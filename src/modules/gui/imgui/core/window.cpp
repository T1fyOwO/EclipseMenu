#include "window.hpp"

#include <imgui_internal.h>
#include <modules/config/config.hpp>
#include <modules/gui/imgui/imgui.hpp>
#include <modules/gui/theming/manager.hpp>

namespace eclipse::gui::imgui {

    Window::Window(std::string title, Function<void()>&& onDraw) {
        m_title = std::move(title);
        m_drawCallback = std::move(onDraw);
        m_position = ImVec2(0, 0);
        m_drawPosition = m_position;
        m_size = ImVec2(MIN_SIZE);
        m_isOpen = true;
    }

    void Window::draw() {
        if (!isOnScreen())
            return;

        auto tm = ThemeManager::get();
        auto scale = tm->getGlobalScale();

        // Calculate distance to the bottom to make window fit the screen
        auto screenSize = ImGui::GetIO().DisplaySize;
        auto maxSizeH = MAX_SIZE.y * scale;
        auto bottomMargin = tm->getWindowMargin();
        if (m_position.y + m_size.y > screenSize.y - bottomMargin - 1) // -1 to prevent flickering
            maxSizeH = screenSize.y - m_position.y - bottomMargin;

        ImGui::SetNextWindowSizeConstraints({MIN_SIZE.x * scale, MIN_SIZE.y * scale},
                                            {MAX_SIZE.x * scale, maxSizeH});

        ImGui::SetNextWindowCollapsed(!m_isOpen, ImGuiCond_Always);


        if (ImGuiRenderer::get()->beginWindow(m_title)) {
            ImVec2 winPos = ImGui::GetWindowPos();
            ImVec2 winSize = ImGui::GetWindowSize();
            float titleBarHeight = ImGui::GetFrameHeight();

            m_drawCallback();

            // add scroll fade overlays to indicate more content
            float scrollMaxY = ImGui::GetScrollMaxY();
            if (scrollMaxY > 0.5f) {
                float scrollY = ImGui::GetScrollY();
                auto* drawList = ImGui::GetWindowDrawList();

                float innerTop = winPos.y + titleBarHeight;
                float innerBottom = winPos.y + winSize.y;
                float innerLeft = winPos.x;
                float innerRight = winPos.x + winSize.x;

                float innerHeight = innerBottom - innerTop;
                float preferred = 40.0f * scale;
                float gradientHeight = preferred < innerHeight * 0.5f ? preferred : innerHeight * 0.5f;

                ImU32 transparent = IM_COL32(0, 0, 0, 0);
                ImU32 shadow = IM_COL32(0, 0, 0, 150);

                drawList->PushClipRect(
                    ImVec2(innerLeft, innerTop),
                    ImVec2(innerRight, innerBottom),
                    false
                );

                // top fade
                if (scrollY > 0.5f) drawList->AddRectFilledMultiColor(
                    ImVec2(innerLeft, innerTop),
                    ImVec2(innerRight, innerTop + gradientHeight),
                    shadow, shadow, transparent, transparent
                );

                // bottom fade
                if (scrollY < scrollMaxY - 0.5f) {
                    float rounding = ImGui::GetStyle().WindowRounding;
                    float gradientTop = innerBottom - gradientHeight;

                    int vtxStart = drawList->VtxBuffer.Size;
                    drawList->AddRectFilled(
                        ImVec2(innerLeft, gradientTop),
                        ImVec2(innerRight, innerBottom),
                        shadow,
                        rounding,
                        ImDrawFlags_RoundCornersBottom
                    );
                    int vtxEnd = drawList->VtxBuffer.Size;

                    ImDrawVert* verts = drawList->VtxBuffer.Data;
                    ImU32 rgb  = shadow & ~IM_COL32_A_MASK;
                    float maxA = static_cast<float>((shadow >> IM_COL32_A_SHIFT) & 0xFF);
                    float invH = 1.0f / (innerBottom - gradientTop);
                    for (int i = vtxStart; i < vtxEnd; ++i) {
                        float t = ImClamp((verts[i].pos.y - gradientTop) * invH, 0.0f, 1.0f);
                        ImU32 a = static_cast<ImU32>(maxA * t);
                        verts[i].col = rgb | (a << IM_COL32_A_SHIFT);
                    }
                }

                drawList->PopClipRect();
            }
        }

        // Allow moving the window
        if (ImGui::IsMouseDragging(0) && ImGui::IsWindowHovered()) {
            m_position = ImGui::GetWindowPos();

            m_drawPosition = m_position;

            config::setTemp("draggingWindow", true);
        }

        m_size = ImGui::GetWindowSize();
        m_isOpen = !ImGui::IsWindowCollapsed();

        ImGui::SetWindowPos(m_drawPosition);

        ImGuiRenderer::get()->endWindow();
    }

    bool Window::isOpen() const { return m_isOpen; }

    void Window::setOpen(bool open) { m_isOpen = open; }

    bool Window::isOnScreen() const {
        ImVec2 screenSize = ImGui::GetIO().DisplaySize;
        return m_position.x + m_size.x >= 0 &&
               m_position.y + m_size.y >= 0 &&
               m_position.x < screenSize.x &&
               m_position.y < screenSize.y;
    }

    std::string const& Window::getTitle() const { return m_title; }

    void Window::setTitle(std::string title) { m_title = std::move(title); }

    ImVec2 const& Window::getPosition() const { return m_position; }

    void Window::setPosition(ImVec2 const& position) { m_position = position; }

    ImVec2 const& Window::getDrawPosition() const { return m_drawPosition; }

    void Window::setDrawPosition(ImVec2 const& position) { m_drawPosition = position; }

    ImVec2 const& Window::getSize() const { return m_size; }

    void Window::setSize(ImVec2 const& size) { m_size = size; }

    void Window::applyJson(matjson::Value const& json) {
        if (json.contains("pos")) {
            auto& pos = json["pos"];
            setPosition({
                pos["x"].as<float>().unwrapOrDefault(),
                pos["y"].as<float>().unwrapOrDefault()
            });
        }
        if (json.contains("open")) {
            setOpen(json["open"].as<bool>().unwrapOrDefault());
        }
        if (json.contains("title")) {
            setTitle(json["title"].as<std::string>().unwrapOrDefault());
        }
    }

    std::unique_ptr<animation::MoveAction> Window::animateTo(ImVec2 const& target, double duration, animation::EasingFunction easing, bool useRealPosition) {
        auto action = animation::MoveAction::create(duration, &m_drawPosition, target, easing);

        if (useRealPosition)
            m_position = target;

        return std::move(action);
    }
}

matjson::Value matjson::Serialize<eclipse::gui::imgui::Window>::toJson(eclipse::gui::imgui::Window const& window) {
    return makeObject({
        {"pos", makeObject({
            {"x", window.getPosition().x},
            {"y", window.getPosition().y}
        })},
        {"open", window.isOpen()},
        {"title", window.getTitle()}
    });
}

geode::Result<eclipse::gui::imgui::Window> matjson::Serialize<eclipse::gui::imgui::Window>::fromJson(Value const& value) {
    return geode::Err("Use Window::applyJson() instead of deserializing directly");
}
