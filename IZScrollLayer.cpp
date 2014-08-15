//  IZScrollLayer.cpp
//  OceanusOL
//
//  Created by Ruby on 14-8-13.
//
//
#include "IZScrollLayer.h"
#include "cocos-ext.h"

USING_NS_CC;
USING_NS_CC_EXT;
typedef enum {
    NodeTagInvalid = 0,
    NodeTagClipingNode,
    NodeTagContainer,
    NodeTagLoopScrollAction,
    NodeTagIndicatorContainer
}NodeTag;

bool IZScrollLayer::init(std::vector<CCNode*> nodes)
{
    if (!CCLayer::init() || nodes.empty()) {
        return false;
    }

    // Setup a count of the available screens
    totalScreens = nodes.size();

    // Make sure the layer accepts touches
    this->setTouchEnabled(totalScreens > 1);

    _stealTouches = true;
    _minimumTouchLengthToSlide = 30.0f;

    // Set up the starting variables
    currentScreen = 1;

    // Default support loop scroll
    mSupportLoopScroll = true;

    // Support indicate
    mSupportIndicator = true;

    // Asume all items are the same size
    CCNode *node = nodes[0];
    scrollWidth = (int)node->getContentSize().width;
    scrollHeight = (int)node->getContentSize().height;
    startWidth = scrollWidth;
    startHeight = scrollHeight;

    // Set size
    setContentSize(node->getContentSize());

    CCScale9Sprite *stencil = CCScale9Sprite::createWithSpriteFrameName("StencilRect.png",
                                                                        CCRectMake(upoint(0.5f), upoint(0.5f), upoint(0.5f), upoint(0.5f)));
    stencil->setAnchorPoint(CCPointZero);
    stencil->setContentSize(node->getContentSize());
    CCClippingNode *frame = CCClippingNode::create(stencil);
    frame->setContentSize(node->getContentSize());

    // Loop through the array and add the screens
    CCNode *container = CCNode::create();
    int i = 0;
    for (std::vector<CCNode *>::iterator it = nodes.begin(); it != nodes.end(); it++, i++) {
        CCNode *item = *it;
        item->setTag(i + 1);
        item->setAnchorPoint(CCPointZero);
        item->setPosition(ccp(i * scrollWidth, 0));
        container->addChild(item);
    }

    // Add Header
    CCRenderTexture *header = CCRenderTexture::create(scrollWidth, scrollHeight,
                                                      kCCTexture2DPixelFormat_RGBA8888, GL_DEPTH24_STENCIL8);
    header->setPosition(CCPointZero);
    header->begin();
    nodes[totalScreens - 1]->visit();
    header->end();
    header->setPosition(ccp(-scrollWidth * 0.5f, scrollHeight * 0.5f));
    container->addChild(header);

    // Add Footer
    CCRenderTexture *footer = CCRenderTexture::create(scrollWidth, scrollHeight,
                                                      kCCTexture2DPixelFormat_RGBA8888, GL_DEPTH24_STENCIL8);
    footer->setPosition(CCPointZero);
    footer->begin();
    nodes[0]->visit();
    footer->end();
    footer->setPosition(ccp((totalScreens + 0.5f) * scrollWidth, scrollHeight * 0.5f));
    container->addChild(footer);

    frame->setAnchorPoint(CCPointZero);
    addChild(frame, 0, NodeTagClipingNode);
    frame->addChild(container, 0, NodeTagContainer);

    // Show indicators
    showIndicator();

    return true;
}

IZScrollLayer* IZScrollLayer::create(std::vector<CCNode*> nodes)
{
	IZScrollLayer *pRet = new IZScrollLayer();
	if (pRet && pRet->init(nodes)) {
		pRet->autorelease();
		return pRet;
	} else {
    	CC_SAFE_DELETE(pRet);
        return NULL;
    }
}

void IZScrollLayer::moveToPage(int page)
{
    currentScreen = page;
	CCEaseBounce* changePage = CCEaseBounce::create(CCMoveTo::create(0.3f, ccp(-((page-1)*scrollWidth),0)));
    CCNode *container = getChildByTag(NodeTagClipingNode)->getChildByTag(NodeTagContainer);
    container->runAction(changePage);
}

void IZScrollLayer::moveToNextPage()
{
	CCEaseBounce* changePage = CCEaseBounce::create(CCMoveTo::create(0.3f, ccp(-(((currentScreen+1)-1)*scrollWidth),0)));
    CCNode *container = getChildByTag(NodeTagClipingNode)->getChildByTag(NodeTagContainer);
    currentScreen++;
    container->runAction(CCSequence::create(changePage,
                                            CCCallFunc::create(this, callfunc_selector(IZScrollLayer::resetItemsOrder)),
                                            NULL));
}

void IZScrollLayer::moveToPreviousPage()
{
	CCEaseBounce* changePage =CCEaseBounce::create(CCMoveTo::create(0.3f, ccp(-(((currentScreen-1)-1)*scrollWidth),0)));
    CCNode *container = getChildByTag(NodeTagClipingNode)->getChildByTag(NodeTagContainer);
    currentScreen--;
    container->runAction(CCSequence::create(changePage,
                                            CCCallFunc::create(this, callfunc_selector(IZScrollLayer::resetItemsOrder)),
                                            NULL));
}

void IZScrollLayer::resetItemsOrder()
{
    if (!mSupportLoopScroll || totalScreens == 1) {
        return;
    }

    CCNode *container = getChildByTag(NodeTagClipingNode)->getChildByTag(NodeTagContainer);
    if (currentScreen == 0) {
        container->setPositionX(-(totalScreens - 1) * scrollWidth);
        currentScreen = totalScreens;
    }

    if (currentScreen == totalScreens + 1) {
        container->setPositionX(0);
        currentScreen = 1;
    }

    refreshHighLightedIndictor();
}

void IZScrollLayer::fireAutoScroll()
{
    CCAction *action = (CCAction *)getActionByTag(NodeTagLoopScrollAction);
    if (action && !action->isDone() || totalScreens <= 1) {
        return;
    }
    CCAction *loopScroll =  CCRepeatForever::create(CCSequence::create(CCDelayTime::create(3.f),
                                                                       CCCallFunc::create(this, callfunc_selector(IZScrollLayer::moveToNextPage)),
                                                                       NULL));
    loopScroll->setTag(NodeTagLoopScrollAction);
    runAction(loopScroll);
}

void IZScrollLayer::onExit()
{
    CCLayer::onExit();
	CCDirector::sharedDirector()->getTouchDispatcher()->removeDelegate(this);
}

void IZScrollLayer::onEnterTransitionDidFinish()
{
    CCLayer::onEnterTransitionDidFinish();
    fireAutoScroll();
}

bool IZScrollLayer::ccTouchBegan(CCTouch *touch, CCEvent *withEvent)
{
    // If touch others return, else this will scroll
    if (!checkClaim(touch)) {
        return false;
    }

    stopActionByTag(NodeTagLoopScrollAction);
    touchLength_ = 0;
	CCPoint touchPoint = touch->getLocationInView();
	touchPoint = CCDirector::sharedDirector()->convertToGL(touchPoint);
	startSwipe = (int)touchPoint.x;
	return true;
}

void IZScrollLayer::ccTouchMoved(CCTouch *touch, CCEvent *withEvent)
{
    CCPoint point = touch->getLocation();
	CCPoint prevPoint = touch->getPreviousLocation();
    CCPoint delta = ccpSub(point, prevPoint);
	
    touchLength_ += ccpLength(delta);
    
	// If finger is dragged for more distance then minimum - start sliding and cancel pressed buttons.
	// Of course only if we not already in sliding mode
	if (touchLength_ < getMinimumTouchLengthToSlide()) {
        return;
	}
    
    if (getStealTouches() && checkClaim(touch)) {
        claimTouch(touch);
    }

	CCPoint touchPoint = touch->getLocationInView();
	touchPoint = CCDirector::sharedDirector()->convertToGL(touchPoint);
    CCNode *container = getChildByTag(NodeTagClipingNode)->getChildByTag(NodeTagContainer);
	container->setPosition(ccp((-(currentScreen-1)*scrollWidth)+(touchPoint.x-startSwipe),0));
}

void IZScrollLayer::ccTouchEnded(CCTouch *touch, CCEvent *withEvent)
{
	CCPoint touchPoint = touch->getLocationInView();
	touchPoint = CCDirector::sharedDirector()->convertToGL(touchPoint);
    fireAutoScroll();

	int newX = (int)touchPoint.x;
	if (newX - startSwipe < -scrollWidth / 3) {
		this->moveToNextPage();
	} else if ( (newX - startSwipe) > scrollWidth / 3) {
		this->moveToPreviousPage();
	} else {
		this->moveToPage(currentScreen);		
	}
}

void IZScrollLayer::claimTouch(cocos2d::CCTouch *aTouch)
{
    CCTouchDispatcher *dispatcher = CCDirector::sharedDirector()->getTouchDispatcher();
	// Enumerate through all targeted handlers.
	CCArray *handlers = dispatcher->getTargetedHandlers();
	int count = handlers->count();
	for (int i = 0; i < count; i++ ) {
		CCTargetedTouchHandler *handler = (CCTargetedTouchHandler *)handlers->objectAtIndex(i);
		// Only our handler should claim the touch.
		if (handler->getDelegate() == this) {
			if (!handler->getClaimedTouches()->containsObject(aTouch)) {
				handler->getClaimedTouches()->addObject(aTouch);
			}
        } else {
            // Steal touch from other targeted delegates, if they claimed it.
            if (handler->getClaimedTouches()->containsObject(aTouch)) {
				handler->getDelegate()->ccTouchCancelled(aTouch, NULL);
                handler->getClaimedTouches()->removeObject(aTouch);
            }
        }
	}
}

bool IZScrollLayer::checkClaim(cocos2d::CCTouch *aTouch)
{
    CCPoint touchPoint = aTouch->getLocationInView();
    touchPoint = CCDirector::sharedDirector()->convertToGL(touchPoint);

    CCNode *frame = getChildByTag(NodeTagClipingNode);
    CCRect rect = frame->boundingBox();
    rect.origin = getParent()->convertToWorldSpace(rect.origin);

    return rect.containsPoint(touchPoint);
}

void IZScrollLayer::registerWithTouchDispatcher()
{
    CCTouchDispatcher *dispatcher = CCDirector::sharedDirector()->getTouchDispatcher();
    int priority = kCCMenuHandlerPriority - 1;
    dispatcher->addTargetedDelegate(this, priority, false);
}

void IZScrollLayer::showIndicator()
{
    if (!mSupportIndicator) {
        return;
    }

    CCNode *iContainer = CCNode::create();
    static const int gap = upoint(2);
    int itemWidth, itemHeight = 0;
    for (int i = 0; i < totalScreens; i++) {
        CCSprite *indicator = CCSprite::createWithSpriteFrameName("PageIndicator.png");
        CCSprite *highIndicator = CCSprite::createWithSpriteFrameName("PageIndicatorHighlighted.png");

        itemWidth = indicator->getContentSize().width;
        itemHeight = indicator->getContentSize().height;

        indicator->setAnchorPoint(CCPointZero);
        indicator->setPosition(ccp(i * (itemWidth + gap), 0));

        highIndicator->setAnchorPoint(CCPointZero);
        highIndicator->setPosition(ccp(i * (itemWidth + gap), 0));
        if (i != 0) {
            highIndicator->setVisible(false);
        }

        iContainer->addChild(indicator);
        iContainer->addChild(highIndicator, 1, i + 1);
    }

    CCNode *frame = getChildByTag(NodeTagClipingNode);

    iContainer->setContentSize(ccp(totalScreens * itemWidth + gap * (totalScreens - 1), itemHeight));
    iContainer->setAnchorPoint(ccp(0.5f, 1));
    iContainer->setPosition(ccp(frame->boundingBox().getMidX(), frame->boundingBox().getMinY() - upoint(5)));
    addChild(iContainer, 0, NodeTagIndicatorContainer);
}

void IZScrollLayer::refreshHighLightedIndictor()
{
    if (!mSupportLoopScroll) {
        return;
    }
    CCNode *indicatorContainer = getChildByTag(NodeTagIndicatorContainer);
    for (int i = 1 ; i <= totalScreens; i++) {
        indicatorContainer->getChildByTag(i)->setVisible(i == currentScreen);
    }
}