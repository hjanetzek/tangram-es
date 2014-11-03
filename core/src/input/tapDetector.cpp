#include "tapDetector.h"
#include "platform.h"

bool Tangram::TapDetector::withinDistance(const Tangram::TouchEvent &_newEvent, float _maxDist) {
    //Since a tap it should have only one touchpoint
    float dx = _newEvent.m_points[0].m_pos.x - m_tapEvent.m_points[0].m_pos.x;
    float dy = _newEvent.m_points[0].m_pos.y - m_tapEvent.m_points[0].m_pos.y;
    return ( (dx*dx + dy*dy) - (_maxDist*_maxDist) < 0);
}

bool Tangram::TapDetector::withinTimeout(const Tangram::TouchEvent &_newEvent, int _timeoutMS) {
    std::chrono::milliseconds tDiff = std::chrono::duration_cast<std::chrono::duration<int, std::milli>> (_newEvent.m_timePoint - m_tapEvent.m_timePoint);
    return ( tDiff.count() < _timeoutMS);
}

Tangram::GestureState Tangram::TapDetector::detect(const Tangram::TouchEvent &_newEvent) {
    //Only handle events with one touchpoint
    if( (_newEvent.m_numTouches > 1) || (_newEvent.m_type == TouchEvent::eventType::cancelled)) {
        return GestureState::none;
    }
    
    // store "down event" to be checked with a possible upcoming "up event"
    if(_newEvent.m_type == TouchEvent::eventType::began) {
        //TODO: handle multi taps
        logMsg("touchevent began\n");
        m_tapEvent = _newEvent;
        return GestureState::none;
    }
    
    //Check for an "up event" against a previous "down event"
    if(_newEvent.m_type == TouchEvent::eventType::ended) {
        logMsg("touchEvent end\n");
        //make sure its within a time threshold of the original down event
        //and within a threshold distance of the original down event
        if(withinTimeout(_newEvent, c_touchTimeout)) {
            logMsg("timeout good\n");
            if(withinDistance(_newEvent, c_maxTouchDist)) {
                logMsg("distance good\n");
                m_tapCount++;
                m_tapEvent = _newEvent;
                m_tapPosition = _newEvent.m_points[0].m_pos;
                m_tapped = true;
                return GestureState::action;
            }
        }
    }
    logMsg("nothing\n");
    return GestureState::none;
}

void Tangram::TapDetector::reset() {
    if(m_tapEvent.m_type != TouchEvent::eventType::invalid) {
        m_tapEvent = TouchEvent();
    }
    m_tapCount = 0;
    m_tapped = false;
}

