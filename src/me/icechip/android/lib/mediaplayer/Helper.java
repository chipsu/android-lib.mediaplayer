package me.icechip.android.lib.mediaplayer;

import android.opengl.GLES20;
import android.os.Debug;
import android.util.Log;

public class Helper {
	private static final String TAG = "Helper";
	
	public static void glThrowError() {
		if(Debug.isDebuggerConnected()) {
			int error = GLES20.glGetError();
			if(error != GLES20.GL_NO_ERROR) {
	            Log.e(TAG, "glError: " + error);
	            throw new RuntimeException("glError: " + error);
			}
		} else {
			glCheckError();
		}
	}

    public static void glCheckError() {
		int error = GLES20.glGetError();
		if(error != GLES20.GL_NO_ERROR) {
            Log.w(TAG, "glError: " + error);
		}
    }
}