#pragma once

#include "touchEvent.h"
#include "gestureState.h"

namespace Tangram {

    /* 
     * Helper to construct different gestures based on the types/sequences of taps detected
     */
    class TapDetector {
        /* Required to save the previous event for tap detection */
        TouchEvent m_tapEvent;
        unsigned int m_tapCount;

        //TODO: Only handles single tap right now
        //      Need functionality for multiple taps
        /* 
         * Timeouts for tap action (millisecond)
         * c_touchTimeout: to detect timeout for a single touch event
         * c_tapTimeout: to detect timeout for multiple taps
         */
        const unsigned int c_touchTimeout = 250;
        const unsigned int c_tapTimeout = 750;
    
        //TODO: Only handles single tap right now
        //      Need functionality for multiple taps
        /*
         * Proximity distance constants for tap(up event) and multiple taps
         * c_maxTouchDist: maximum allowed pixel distance between a down and an up event (for a tap)
         * c_maxTapDist: maximum allowed pixel distance for a multiple tap sequence
         */
        const float c_maxTouchDist = 10;
        const float c_maxTapDist = 30;
        
        /* set when a tap gesture is detected */
        bool m_tapped = false;
        glm::vec2 m_tapPosition;
        
        /*
         * checks if the new touchevent is within a specific threshhold of the old event stored in this obj
         */
        bool withinDistance(const TouchEvent& _touchEvent, float _maxDist);

        /*
         * checks if the new touchevent is within a timeout of the old event stored in this obj
         */
        bool withinTimeout(const TouchEvent& _touchEvent, int _timeoutMS);


    public:
        
        /* Defines the logic to detect a tap using this event and a previous event */
        GestureState detect(const TouchEvent& _touchEvent);

        /* resets the detector*/
        void reset();
        
        /* getters */
        bool isTap() {return m_tapped;}
        glm::vec2 tapPosition() {return m_tapPosition;}
    };

};

