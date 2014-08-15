//  IZScrollLayer.h
//  OceanusOL
//
//  Created by Ruby on 14-8-13.
//
//
#ifndef __OceanusOL__IZScrollLayer__
#define __OceanusOL__IZScrollLayer__

#include "OceanusOL.h"
#include "IZScrollView.h"

class IZScrollLayer : public cocos2d::CCLayer
{
    CC_SYNTHESIZE(float, _minimumTouchLengthToSlide, MinimumTouchLengthToSlide);
    CC_SYNTHESIZE(bool, _stealTouches, StealTouches);
protected:
	// Holds the current height and width of the screen
	int scrollHeight;
	int scrollWidth;
	
	// Holds the height and width of the screen when the class was inited
	int startHeight;
	int startWidth;

    float touchLength_;
	
	// Holds the current page being displayed
	int currentScreen;
	
	// A count of the total screens available
	int totalScreens;
	
	// The initial point the user starts their swipe
	int startSwipe;

    // Support loop scroll
    bool mSupportLoopScroll;
    bool mSupportIndicator;

	void moveToPage(int page);
	void moveToNextPage();
	void moveToPreviousPage();

    void resetItemsOrder();
    void fireAutoScroll();

    virtual void onExit();
    virtual void onEnterTransitionDidFinish();
    void registerWithTouchDispatcher();
    void claimTouch(cocos2d::CCTouch *aTouch);
    bool checkClaim(cocos2d::CCTouch *aTouch);

    void showIndicator();
    void refreshHighLightedIndictor();

	virtual bool ccTouchBegan(cocos2d::CCTouch *touch, cocos2d::CCEvent *withEvent);
	virtual void ccTouchMoved(cocos2d::CCTouch *touch, cocos2d::CCEvent *withEvent);
	virtual void ccTouchEnded(cocos2d::CCTouch *touch, cocos2d::CCEvent *withEvent);
public:
	static IZScrollLayer* create(std::vector<cocos2d::CCNode*> nodes);
	bool init(std::vector<CCNode*> nodes);
};

#endif