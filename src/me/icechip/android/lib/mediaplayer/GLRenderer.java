package me.icechip.android.lib.mediaplayer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.opengl.GLES10;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.util.Log;

public class GLRenderer implements GLSurfaceView.Renderer {
	
    public GLRenderer(Context context) {
        mContext = context;
    }

    public void onDrawFrame(GL10 glUnused) {
		
    	MediaPlayer mMediaPlayer = MediaPlayer.getInstance();

		if(mMediaPlayer != null /*&& mMediaPlayer.isOpen()*/) {
			mMediaPlayer.nativeRender();
		} else {
	    	GLES20.glClearColor(mClearColor[0], mClearColor[1], mClearColor[2], mClearColor[3]);
			GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);
			mClearColor[0] += 0.01;
			if(mClearColor[0] > 1.0) mClearColor[0] = 0;
		}

        mFpsTimer.frame();
        
        //GLES20.glFinish();
    }

    public void onSurfaceChanged(GL10 glUnused, int width, int height) {
	    try {
	    	//float ratio = (float)width / height;
	        GLES20.glViewport(0, 0, width, height);
	        //Camera.frustum(-ratio, ratio, -1, 1);
	        mWidth = width;
	        mHeight = height;
	        Helper.glThrowError();
		} catch(Exception ex) {
			Log.e(TAG, ex.toString());
			Log.e(TAG, Log.getStackTraceString(ex));
            throw new RuntimeException(ex);
		}
    }

    public void onSurfaceCreated(GL10 glUnused, EGLConfig config) {
		try {
			Log.d(TAG, "GL_EXTENSIONS:\n");
			Log.d(TAG, GLES10.glGetString(GLES10.GL_EXTENSIONS));
	        onInit();
	        Helper.glThrowError();
		} catch(Exception ex) {
			Log.e(TAG, ex.toString());
			Log.e(TAG, Log.getStackTraceString(ex));
            throw new RuntimeException(ex);
		}
    }
    
    public void onInit() {
    	GLES20.glDisable(GLES20.GL_DEPTH_TEST);
    	GLES20.glCullFace(GLES20.GL_BACK);
		GLES20.glHint(GLES20.GL_GENERATE_MIPMAP_HINT, GLES20.GL_FASTEST);
        Helper.glThrowError();
    }
    
    public Context context() {
    	return mContext;
    }
    
    public int width() {
    	return mWidth;
    }
    
    public int height() {
    	return mHeight;
    }

    private FpsTimer mFpsTimer = FpsTimer.getInstance();
    private int mWidth;
    private int mHeight;
    private float[] mClearColor = { 1, 0, 1, 1 };
    private Context mContext;
    private static final String TAG = GLRenderer.class.getName();
}