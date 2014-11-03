#include <cassert>

#include "touchEvent.h"
#include "platform.h"

namespace Tangram {

    const TouchEvent::Point* TouchEvent::getTouchPoint(std::uintptr_t _pointID) const {
        for(unsigned int i = 0; i < m_numTouches; i++) {
            if(m_points[i].m_id == _pointID) {
                return &(m_points[i]);
            }
        }
        return nullptr;
    }

    const glm::vec2& TouchEvent::touchPos(std::uintptr_t _pointID) const {
        const Point* pt = getTouchPoint(_pointID);
        assert(pt!=nullptr);
        return pt->m_pos;
    }

    bool TouchEvent::sameEventCheck(const TouchEvent& _other) const {
        if(_other.m_numTouches == m_numTouches) {
            for(unsigned int i = 0; i < m_numTouches; i++) {
                if(!getTouchPoint(_other.m_points[i].m_id)) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

};

