
#include <jni.h>
#include "tangram.h"
#include "input/touchEvent.h"
#include "platform.h"

extern "C" {
	JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_init(JNIEnv* jniEnv, jobject obj) 
	{
		initializeOpenGL();
	}

	JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_resize(JNIEnv* jniEnv, jobject obj, jint width, jint height) 
	{
		resizeViewport(width, height);
	}

	JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_render(JNIEnv* jniEnv, jobject obj) 
	{
		renderFrame();
	}

    /*
     * Responsible to create native TouchEvent from Android's motion event parameters
     */
    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_createTouchEvent(JNIEnv* jniEnv, jobject obj, jint type, jint numTouches, jfloatArray x_Pos, jfloatArray y_Pos, jintArray pointIDs, jbooleanArray pointsChanged) {
        // Code to create a touchEvent
        Tangram::TouchEvent newEvent;
        jfloat *c_x_Pos = jniEnv->GetFloatArrayElements(x_Pos, NULL);
        jfloat *c_y_Pos = jniEnv->GetFloatArrayElements(y_Pos, NULL);
        jint *c_pointIDs = jniEnv->GetIntArrayElements(pointIDs, NULL);
        jboolean *c_pointsChanged = jniEnv->GetBooleanArrayElements(pointsChanged, NULL);

        //Assign event type: began, moved, ended, cancelled, invalid
        switch(type) {
            case 0: //ACTION_DOWN
            case 5: //ACTION_POINTER_DOWN
                newEvent.m_type = Tangram::TouchEvent::eventType::began;
                break;
            case 1: //ACTION_UP
            case 6: //ACTION_POINTER_UP
                newEvent.m_type = Tangram::TouchEvent::eventType::ended;
                break;
            case 2: //ACTION_MOVE
                newEvent.m_type = Tangram::TouchEvent::eventType::moved;
                break;
            case 3: //ACTION_CANCEL
                newEvent.m_type = Tangram::TouchEvent::eventType::cancelled;
                break;
            default: //OTHERS (TODO: might have to provide others later)
                newEvent.m_type = Tangram::TouchEvent::eventType::invalid;
        }
        newEvent.m_timePoint = std::chrono::high_resolution_clock::now();
        //TODO: Since these touchpoints are not ordered in the index according to primary to secondary, we need to make
        //      sure we grab the correct touchpoints when num of touch points are more than the MAXTOUCH points allowed by our
        //      touchEvent class
        newEvent.m_numTouches = numTouches > Tangram::TouchEvent::s_maxTouches ? Tangram::TouchEvent::s_maxTouches : numTouches;
        for(int i = 0; i < newEvent.m_numTouches; i++) {
            Tangram::TouchEvent::Point& curPoint = newEvent.m_points[i];
            curPoint.m_id = (std::uintptr_t) *(c_pointIDs + i);
            curPoint.m_pos.x = *(c_x_Pos + i);
            curPoint.m_pos.y = *(c_y_Pos + i);
            curPoint.m_isChanged = *(c_pointsChanged + i);
        }
        //Release carrays which were created from jniarray
        jniEnv->ReleaseFloatArrayElements(x_Pos, c_x_Pos, 0); 
        jniEnv->ReleaseFloatArrayElements(y_Pos, c_y_Pos, 0); 
        jniEnv->ReleaseIntArrayElements(pointIDs, c_pointIDs, 0); 
        jniEnv->ReleaseBooleanArrayElements(pointsChanged, c_pointsChanged, 0); 
        handleTouchEvents(newEvent);
    }
}

