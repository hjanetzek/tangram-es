#include "tangram.h"
#include "input/touchEventMgr.h"

// SceneDirector is the primary controller of the map
std::unique_ptr<SceneDirector> sceneDirector;
std::unique_ptr<Tangram::TouchEventMgr> touchEventMgr;

void initializeOpenGL()
{

    sceneDirector.reset(new SceneDirector());
    touchEventMgr.reset(new Tangram::TouchEventMgr());
    sceneDirector->loadStyles();

    logMsg("%s\n", "initialize");

}

void resizeViewport(int newWidth, int newHeight)
{
    glViewport(0, 0, newWidth, newHeight);

    if (sceneDirector) {
        sceneDirector->onResize(newWidth, newHeight);
    }

    logMsg("%s\n", "resizeViewport");
}

void renderFrame()
{

    // TODO: Use dt from client application
    applyGestures();
    sceneDirector->update(0.016);

    sceneDirector->renderFrame();

    GLenum glError = glGetError();
    if (glError) {
        logMsg("GL Error %d!!!\n", glError);
    }
}

void handleTouchEvents(const Tangram::TouchEvent& _touchEvent) {
    touchEventMgr->onTouchEvents(_touchEvent);
}

//Reads Info from TouchEventMgr and does touch event processing
//          has helper functions like: (See /Users/Varun/Development/oryol/code/Samples/Input/TestInput/TestInput.cc for referal)
//                  handleTap
//                  handlePinch
//                  handlePan
void applyGestures() {
    if(touchEventMgr->m_tapDetector.isTap()) {
        logMsg("new tap event detected. Yay!\n");
        touchEventMgr->m_tapDetector.reset();
        //TODO: code to handle tapping
    }
}
