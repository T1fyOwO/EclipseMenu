#include <modules/config/config.hpp>
#include <modules/gui/gui.hpp>
#include <modules/gui/components/toggle.hpp>
#include <modules/hack/hack.hpp>

#include <Geode/modify/PlayLayer.hpp>

namespace eclipse::hacks::Level {
    class $hack(ShowTotalAttempts) {
        void init() override {
            auto tab = gui::MenuTab::find("tab.level");
            tab->addToggle("level.showtotalattempts")->handleKeybinds()->setDescription();
        }

        [[nodiscard]] const char* getId() const override { return "Show Total Attempts"; }
    };

    REGISTER_HACK(ShowTotalAttempts)

    class $modify(ShowTotalAttemptsPLHook, PlayLayer) {
        ADD_HOOKS_DELEGATE("level.showtotalattempts")

        void resetLevel() {
            PlayLayer::resetLevel();

            if (!m_level || !m_attemptLabel) return;
            m_attemptLabel->setString(fmt::format("Attempt {}", m_level->m_attempts.value()).c_str());
        }
    };
}
