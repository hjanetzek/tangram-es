//
//  ViewController.m
//  TangramiOS
//
//  Created by Matt Blair on 8/25/14.
//  Copyright (c) 2014 Mapzen. All rights reserved.
//

#import "ViewController.h"

@interface ViewController () {

}
@property (strong, nonatomic) EAGLContext *context;

- (void)setupGL;
- (void)tearDownGL;
- (void)onTouchEvent:(Tangram::TouchEvent::eventType)type withTouches:(NSSet*)touches withEvent:(UIEvent*)event;

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    view.multipleTouchEnabled = YES;
    
    [self setupGL];
}

- (void)dealloc
{    
    [self tearDownGL];
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil)) {
        self.view = nil;
        
        [self tearDownGL];
        
        if ([EAGLContext currentContext] == self.context) {
            [EAGLContext setCurrentContext:nil];
        }
        self.context = nil;
    }

    // Dispose of any resources that can be recreated.
}

- (void) touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event {
    [self onTouchEvent:Tangram::TouchEvent::eventType::began withTouches:touches withEvent:event];
}

- (void) touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event {
    [self onTouchEvent:Tangram::TouchEvent::eventType::moved withTouches:touches withEvent:event];
}

- (void) touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event {
    [self onTouchEvent:Tangram::TouchEvent::eventType::ended withTouches:touches withEvent:event];
}

- (void) touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event {
    [self onTouchEvent:Tangram::TouchEvent::eventType::cancelled withTouches:touches withEvent:event];
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];
    
    initializeOpenGL();
    
    int width = self.view.bounds.size.width;
    int height = self.view.bounds.size.height;
    resizeViewport(width, height);
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
    
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    applyGestures();
    clearGestures();
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    renderFrame();
}


- (void) onTouchEvent:(Tangram::TouchEvent::eventType)type withTouches:(NSSet*)touches withEvent:(UIEvent*)event {
    Tangram::TouchEvent newEvent;
    newEvent.m_type = type;
    newEvent.m_timePoint = std::chrono::high_resolution_clock::now();
    NSEnumerator* enumerator = [[event allTouches] objectEnumerator];
    UITouch* curTouch;
    while ((curTouch = [enumerator nextObject])) {
        if ((newEvent.m_numTouches + 1) < Tangram::TouchEvent::s_maxTouches) {
            CGPoint pos = [curTouch locationInView:curTouch.view];
            Tangram::TouchEvent::Point& curPoint = newEvent.m_points[newEvent.m_numTouches++];
            curPoint.m_id = (std::uintptr_t) curTouch;
            curPoint.m_pos.x = pos.x;
            curPoint.m_pos.y = pos.y;
            curPoint.m_isChanged = [touches containsObject:curTouch];
        }
    }
    handleTouchEvents(newEvent);
}

@end
