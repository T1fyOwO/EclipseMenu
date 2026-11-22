#pragma once

namespace eclipse::gui::cocos {
    class ScrollLayer : public geode::ScrollLayer {
    protected:
        bool ccTouchBegan(cocos2d::CCTouch*, cocos2d::CCEvent*) override;

        ScrollLayer(cocos2d::CCRect const& rect, bool scrollWheelEnabled, bool vertical);
        ~ScrollLayer() override;

        bool touchInsideChildScroll(cocos2d::CCPoint touchPos);

    public:
        static ScrollLayer* create(cocos2d::CCRect const& rect, bool scrollWheelEnabled = true, bool vertical = true);
        static ScrollLayer* create(cocos2d::CCSize const& size, bool scrollWheelEnabled = true, bool vertical = true);
    };
}
