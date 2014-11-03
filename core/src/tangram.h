#pragma once

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <memory>

#include "platform.h"
#include "sceneDirector/sceneDirector.h"
#include "util/shaderProgram.h"
#include "util/vertexLayout.h"
#include "util/vboMesh.h"
#include "input/touchEventMgr.h"

/* Tangram C API
 *
 * Primary interface for controlling and managing the lifecycle of a Tangram map surface
 * 
 * TODO: More complete lifecycle management such as onPause, onResume
 * TODO: Input functions will go here too, things like onTouchDown, onTouchMove, etc.
 */

// Create resources and initialize the map view
void initializeOpenGL();

// Resize the map view to a new width and height (in pixels)
void resizeViewport(int newWidth, int newHeight);

// Render a new frame of the map view (if needed)
// TODO: Pass in the time step from the client application
void renderFrame();

/*
 * Passes the platform independent TouchEvent to touchEventMgr for processing
 */
void handleTouchEvents(const Tangram::TouchEvent& _touchEvent);

/*
 * Checks against various gestures and applies the corresponding gesture to the application
 */
void applyGestures();

/*
 * Clears the state of all gestures detected in the last update call!
 */
void clearGestures();
