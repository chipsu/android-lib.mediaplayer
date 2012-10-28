package me.icechip.android.lib.mediaplayer;

import java.io.File;

import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.view.Window;
import android.view.WindowManager;
import android.widget.LinearLayout;

//Process.setThreadPriority(Process.THREAD_PRIORITY_LESS_FAVORABLE);
public class MediaActivity extends Activity {
    public static final String TAG = MediaActivity.class.getName();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        init();
    }
    
    @Override
    protected void onResume() {
        super.onResume();
        if(mGLSurfaceView != null) {
        	mGLSurfaceView.onResume();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if(mGLSurfaceView != null) {
        	mGLSurfaceView.onPause();
        }
        MediaPlayer.getInstance().close();
    }
    
    @Override
    public void onDestroy() {
    	MediaPlayer.getInstance().close();
    	super.onDestroy();
    }
    
    private void init() {
    	requestWindowFeature(Window.FEATURE_NO_TITLE);
    	setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
    	getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
    	
    	if(mUseGL) {
            mGLSurfaceView = new GLView(this);
    		mVideoView = mGLSurfaceView;
    		MediaPlayer.getInstance().mGLSurfaceView = mGLSurfaceView;
    	} else {
    		Log.w(TAG, "Using software VideoView!");
    		Log.wtf(TAG, "FIXME");
    		//mVideoView = new SWView(this);
    	}

    	mVideoView.setVisibility(View.INVISIBLE);
    	addContentView(mVideoView, new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));

        /*mOverlay = new OverlayView(this);
        mOverlay.setVisibility(View.INVISIBLE);
        addContentView(mOverlay, new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));*/

        mFileBrowser = new FileBrowser(this) {
        	protected void onOpenFile(File file) {
        		MediaPlayer.getInstance().open(file.getAbsolutePath());
        		// FIXME: VideoContext.onStateChange, each of these views should hook that and show itself when needed
        		MediaActivity.this.mFileBrowser.setVisibility(INVISIBLE);
        		MediaActivity.this.mVideoView.setVisibility(VISIBLE);
        		//MediaActivity.this.mOverlay.setVisibility(VISIBLE);
        		MediaPlayer.getInstance().play();
        	}
        };
        //File videoDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MOVIES);
        File videoDir = Environment.getExternalStoragePublicDirectory("video");
        mFileBrowser.trySetPath(videoDir.getAbsolutePath()); // TODO: Use favs from config
        addContentView(mFileBrowser, new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
    }

    protected boolean mUseGL = true;
    protected View mVideoView;
    protected GLSurfaceView mGLSurfaceView;
    protected FileBrowser mFileBrowser;
}
