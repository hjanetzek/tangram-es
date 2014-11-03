#pragma once

#include "touchEvent.h"
#include "tapDetector.h"
#include "panDetector.h"
#include "pinchDetector.h"

namespace Tangram {

    /* Takes the TouchEvents from platform specific code
     * Responsibe to process touch events using gesture specific detectors:
     *          tapDetector
     *          pinchDetector
     *          panDetector
     */
    class TouchEventMgr {
    public:
        TapDetector m_tapDetector;
        PanDetector m_panDetector;
        PinchDetector m_pinchDetector;
        void onTouchEvents(const TouchEvent& _touchEvent);
        void clearEvents();
    };

};
