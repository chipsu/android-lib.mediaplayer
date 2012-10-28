package me.icechip.android.lib.mediaplayer;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;

public class GLView extends GLSurfaceView {
	private static final String TAG = GLView.class.getName();

	public GLView(Context context) {
		super(context);
		init(context);
	}
	
	public GLView(Context context, AttributeSet attrs) {
		super(context, attrs);
		init(context);
	}
	
	private void init(Context context) {
		int version = getGLESVersion(context);
		if(version >= 0x20000) {
        	GLRenderer renderer = new GLRenderer(context);
			setEGLContextClientVersion(2);
			setRenderer(renderer);
			setRenderMode(RENDERMODE_WHEN_DIRTY);
			//setRenderMode(RENDERMODE_CONTINUOUSLY);
		} else {
        	Log.wtf(TAG, "Sorry, this system does not support OpenGL ES 2.0 :(\n");
		}
	}

    public static int getGLESVersion(Context context) {
        ActivityManager am = (ActivityManager)context.getSystemService(Context.ACTIVITY_SERVICE);
        ConfigurationInfo info = am.getDeviceConfigurationInfo();
        return info.reqGlEsVersion;
    }
}