#include "scroll-layer.hpp"
#include "utils.hpp"

#include <modules/utils/SingletonCache.hpp>

namespace eclipse::gui::cocos {
    bool ScrollLayer::ccTouchBegan(cocos2d::CCTouch *touch, cocos2d::CCEvent *event) {
        if (!geode::cocos::nodeIsVisible(this)) return false;

        if (this->touchInsideChildScroll(touch->getLocation())) {
            return false;
        }

        return geode::ScrollLayer::ccTouchBegan(touch, event);
    }

    bool ScrollLayer::touchInsideChildScroll(cocos2d::CCPoint touchPos) {
        for (auto child : m_contentLayer->getChildrenExt()) {
            if (geode::cast::typeinfo_cast<geode::ScrollLayer*>(child)) {
                if (child->isVisible()) {
                    auto pos = child->convertToNodeSpace(touchPos);
                    if (cocos2d::CCRect{{}, child->getContentSize()}.containsPoint(pos)) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    ScrollLayer::ScrollLayer(cocos2d::CCRect const& rect, bool scrollWheelEnabled, bool vertical) :
        geode::ScrollLayer(rect, scrollWheelEnabled, vertical) {}

    ScrollLayer::~ScrollLayer() = default;

    ScrollLayer* ScrollLayer::create(cocos2d::CCRect const& rect, bool scroll, bool vertical) {
        auto ret = new ScrollLayer(rect, scroll, vertical);
        ret->autorelease();
        return ret;
    }

    ScrollLayer* ScrollLayer::create(cocos2d::CCSize const& size, bool scroll, bool vertical) {
        return ScrollLayer::create({ 0, 0, size.width, size.height }, scroll, vertical);
    }
}
