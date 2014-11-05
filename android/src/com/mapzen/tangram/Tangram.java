package com.mapzen.tangram;

import android.opengl.GLSurfaceView;
import android.view.View.OnTouchListener;
import android.view.View;
import android.view.MotionEvent;
import android.opengl.GLES10;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.egl.EGLConfig;

public class Tangram implements GLSurfaceView.Renderer, OnTouchListener {
	
	static {
		System.loadLibrary("c++_shared");
		System.loadLibrary("tangram");
	}

	private static native void init();
	private static native void resize(int width, int height);
	private static native void render();
    private static native void createTouchEvent(int type, int numTouches, float[] x_Pos, float[] y_Pos, int[] pointIDs, boolean[] pointsChanged);

	public void onDrawFrame(GL10 gl) 
	{
		render();
	}

	public void onSurfaceChanged(GL10 gl, int width, int height) 
	{
		resize(width, height);
	}

	public void onSurfaceCreated(GL10 gl, EGLConfig config) 
	{
		init();
	}

    //getAction returns a combination of actual action (least sig 8 bits) and the id(most sig 24 bits): 
    //getActionMasked: returns the actual action
    //getActionIndex: returns the index associated with pointer actions
    public boolean onTouch(View v, MotionEvent event) 
    {
        int pointerCount = event.getPointerCount();
        int action = event.getActionMasked();
        int actionPointerIndex = event.getActionIndex();
        //long timePoint = event.getEventTime();
        float[] x_Pos = new float[pointerCount];
        float[] y_Pos = new float[pointerCount];
        int[] pointIDs = new int[pointerCount];
        boolean[] pointsChanged = new boolean[pointerCount];
        
        for(int i = 0; i < pointerCount; i = i+1) {
            pointIDs[i] = event.getPointerId(i);
            x_Pos[i] = event.getX(i);
            y_Pos[i] = event.getY(i);
            pointsChanged[i] = (i == actionPointerIndex);
        }
        createTouchEvent(action, pointerCount, x_Pos, y_Pos, pointIDs, pointsChanged);
        return true;
    }
}

