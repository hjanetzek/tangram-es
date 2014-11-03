#include "touchEventMgr.h"
#include "platform.h"

namespace Tangram {

    void TouchEventMgr::onTouchEvents(const Tangram::TouchEvent& _touchEvent) {
        //Detect single tapping
        m_tapDetector.detect(_touchEvent);
        
        //TODO: detect other gestures
    }

    void TouchEventMgr::clearEvents() {
        //reset all gestures
        m_tapDetector.reset();
    }
    
}
