#pragma once

#include <chrono>

#include "glm/vec2.hpp"

namespace Tangram {

    class TouchEvent {
        public:
        /* an enum describing event type */
        enum class eventType {
            began,
            moved,
            ended,
            cancelled,
            invalid,
        } m_type = eventType::invalid;

        /* time stamp for the event */
        std::chrono::high_resolution_clock::time_point m_timePoint;
        
        /* 
         * represents number of touches for this event 
         * Will add special case handling to this for mouse clicks etc
         *  Example for mouse control, m_numTouches will be set to 1
         */
        unsigned int m_numTouches = 0;

        /* A max allowed touch count */
        static const unsigned int s_maxTouches = 8;

        /* 
         * Platform independent point structure, 
         *  ID: A unique ID to identify the touchpoints
         *      Android NDK provides an API call to extract this
         *      iOS, we will use the pointer address of the touchPoint itself
         *      osx ?? ??
         */
        struct Point {
            std::uintptr_t m_id = 0;
            glm::vec2 m_pos;
            bool m_isChanged = false;
        } m_points[s_maxTouches];

        /* returns the Point structure for a given pointerID */
        const Point* getTouchPoint(std::uintptr_t _pointID) const;

        /* returns the screen position for a given pointerID */
        const glm::vec2& touchPos(std::uintptr_t _pointID) const;

        /* checks if 2 touchevents are same or not */
        bool sameEventCheck(const TouchEvent& _other) const;
    };
} // namespace Tangram

