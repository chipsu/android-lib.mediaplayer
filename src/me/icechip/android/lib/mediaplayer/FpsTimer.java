package me.icechip.android.lib.mediaplayer;

public class FpsTimer {
	private int mFps;
	private int mFrames;
	private int mRefreshTime = 250;
	private long mFirstTime;
	private static FpsTimer sInstance;
	
	public static FpsTimer getInstance() {
		if(sInstance == null) {
			sInstance = new FpsTimer();
		}
		return sInstance;
	}
	
	public FpsTimer() {
		reset();
	}
	
	public int fps() {
		return mFps;
	}

	public void reset() {
		mFps = 0;
		mFrames = 0;
		mFirstTime = System.currentTimeMillis();
	}
	
	public void frame() {
		final long time = System.currentTimeMillis();
		final int delta = (int)(time - mFirstTime);
		
		mFrames++;
	
		if(delta >= mRefreshTime) {
			mFps = (1000 * mFrames) / delta;
			mFrames = 0;
			mFirstTime = time;
		}
	}
}