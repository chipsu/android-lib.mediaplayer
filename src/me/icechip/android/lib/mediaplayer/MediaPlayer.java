package me.icechip.android.lib.mediaplayer;

import android.media.AudioFormat;
import android.opengl.GLSurfaceView;
import android.util.Log;

/**
 * @todo cleanup rendering options..
 * 
 * @author mu
 *
 */
public class MediaPlayer {
	public static final String TAG = MediaPlayer.class.getName();
	public static final int STATUS_PLAYING = 1;
	public static final int STATUS_PAUSED = 2;
	public static final int STATUS_STOPPED = 3;
	public static final int VIDEO_RENDERER_NONE = 0;
	public static final int VIDEO_RENDERER_JAVA = 1; // Call java code for video rendering (mVideoRenderer)
	public static final int VIDEO_RENDERER_NATIVE = 2; // Use native renderer
	public static final int NATIVE_VIDEO_RENDERER_OPENGL = 1; // OpenGL ES 2.0
	public static final int NATIVE_VIDEO_RENDERER_STAGEFRIGHT = 2; // SF direct rendering, if available...
	public static final int AUDIO_RENDERER_NONE = 0;
	public static final int AUDIO_RENDERER_JAVA = 1; // Call java code for audio rendering (mAudioRenderer)
	public static final int AUDIO_RENDERER_NATIVE = 2; // Use native code
	public static final int NATIVE_AUDIO_RENDERER_OPENSL = 1; // OpenSL
	public static final int NATIVE_AUDIO_RENDERER_STAGEFRIGHT = 2; // SF hw decoder, if available

	// FFMpeg AVMediaType
	public static final int MEDIA_TYPE_UNKNOWN = -1;
	public static final int MEDIA_TYPE_VIDEO = 0;
	public static final int MEDIA_TYPE_AUDIO = 1;
	public static final int MEDIA_TYPE_DATA = 2;
	public static final int MEDIA_TYPE_SUBTITLE = 3;
	public static final int MEDIA_TYPE_ATTACHMENT = 4;
	
	protected Object mVideoRenderer;
	protected AudioRenderer mAudioRenderer;
	protected int mAudioRenderMode = AUDIO_RENDERER_JAVA;
	protected int mVideoRenderMode = VIDEO_RENDERER_NATIVE;
	protected int mNativeVideoRenderMode = NATIVE_VIDEO_RENDERER_OPENGL;
	
	// TODO: NO SINGLETON!!!
	static MediaPlayer sInstance;
	public static MediaPlayer getInstance() {
		if(sInstance == null) {
			sInstance = new MediaPlayer();
		}
		return sInstance;
	}
	
	protected MediaPlayer() {
	}
	
	public void open(String path) {
		int result = nativeOpen(path);
	}
	
	public void play() {
		int result = nativePlay();
	}
	
	public void close() {
		int result = nativeClose();
		mAudioRenderer = null;
	}
	
	protected void onStatusChanged(int status) {
		Log.e(TAG, "xxxxxxxx onStatusChanged");
	}
	
	public GLSurfaceView mGLSurfaceView;
	
	public int getAudioRenderMode() {
		return mAudioRenderMode;
	}
	
	public void setAudioRenderMode(int mode) {
		mAudioRenderMode = mode;
	}
	
	protected AudioRenderer createAudioRenderer(int hz, int ch, int enc) {
		return new AudioTrackRenderer(hz, ch, enc);
	}

	// TODO: ON FAIL: disable stream! nativeCloseStream...
	protected void onStreamOpen(int type, int index) {
		Log.d(TAG, "onStreamOpen: type=" + type + ", index=" + index);
		switch(type) {
		case MEDIA_TYPE_VIDEO:
			if(mVideoRenderMode == VIDEO_RENDERER_JAVA) {
				// TODO: mVideoRender = createVideoRenderer(...);
				Log.w(TAG, "FIXME");
			}
			break;
		case MEDIA_TYPE_AUDIO:
			if(mAudioRenderMode == AUDIO_RENDERER_JAVA) {
				try {
					AudioStreamInfo info = new AudioStreamInfo();
					if(nativeGetAudioStreamInfo(info, index) != 0) {
						throw new Exception("nativeGetAudioStreamInfo failed for stream " + index);
					}
					Log.d(TAG, "Audio: format=" + info.format + ", ch=" + info.channels + ", rate=" + info.rate);
					int format = info.getAudioFormat();
					int channels = info.getAudioFormatChannels();
					mAudioRenderer = createAudioRenderer(info.rate, channels, format);
					Log.i(TAG, "mAudioRenderer = " + mAudioRenderer.toString());
				} catch(Exception ex) {
					// TODO:: if(hasOtherStreams) tryAnyStream(type)
					Log.e(TAG, ex.getMessage());
					nativeCloseStream(index);
					mAudioRenderer = null;
				}
			}
			break;
		}
	}

	protected void onStreamClose(int type, int index) {
		Log.d(TAG, "onStreamClose: type=" + type + ", index=" + index);
		switch(type) {
		case MEDIA_TYPE_VIDEO:
			if(mVideoRenderer != null) {
				// TODO: mVideoRender = createVideoRenderer(...);
				Log.w(TAG, "FIXME");
			}
			break;
		case MEDIA_TYPE_AUDIO:
			if(mAudioRenderer != null) {
				mAudioRenderer.close();
				mAudioRenderer = null;
				Log.i(TAG, "mAudioRenderer = NULL");
			}
			break;
		}
	}
	
	/**
	 * Called by native code when a video frame is ready to render
	 * @note Not all renderers will trigger this event
	 */
	protected void onVideoFrameReady() {
		if(mNativeVideoRenderMode == NATIVE_VIDEO_RENDERER_OPENGL) {
			mGLSurfaceView.requestRender();
		}
	}
	
	/**
	 * NOT_USED
	 */
	protected void DO_NOT_USE_onAudioFrameReady() {
		Log.wtf(TAG, "FIXME");
	}
	
	/**
	 * This is called from Native code when an Audio frame is ready to render.
	 * The frame should be processed ASAP!
	 */
	protected void renderAudioFrame(byte[] frame, int offset, int length) {
		if(mAudioRenderer != null) {
			mAudioRenderer.render(frame, offset, length);
		} else {
			Log.e(TAG, "mAudioRenderer == NULL, missing AUDIO_RENDERER_NONE?");
		}
	}
	
	/**
	 * Called by native code when...
	 * The difference from onVideoFrameReady is that onVideoFrameReady is called only if the render call has to be called from Java-code.
	 * whereas renderVideoFrame assumes that the Java-code renders the frame.
	 */
	protected void renderVideoFrame(byte[] frame, int formatAndStuff) {
		if(mVideoRenderer != null) {
			//mVideoRenderer.render(...);
			Log.wtf(TAG, "fixme");
		} else {
			Log.e(TAG, "mVideoRenderer == NULL, missing AUDIO_RENDERER_NONE?");
		}
	}

	static protected native int nativeInit();
	protected native int nativeOpen(String path/*, TODO: options: streamIndex, types... disable video... */);
	protected native int nativePlay();
	protected native int nativeClose();
	protected native int nativeOpenStream(int index);
	protected native int nativeCloseStream(int index);
	//protected native int nativeSeek(long position);
	//protected native long nativePosition();
	
	class Config {
		public final int THREAD_DISABLE = 0;
		public final int THREAD_AUDIO = 0x01;
		public final int THREAD_VIDEO = 0x02;
		public final int THREAD_SUBTITLES = 0x04;
		public final int THREAD_ALL = THREAD_AUDIO | THREAD_VIDEO | THREAD_SUBTITLES;
		int threadMode = THREAD_DISABLE;
	}
	protected native int nativeConfig(Config config);
	
	/**
	 * Perform native rendering, this method should only be called after the onVideoFrameReady event.
	 */
	public native int nativeRender();
	protected native int nativeSetVideoRenderer(int renderer);
	
	class StreamInfo {
		public int index;
		public int open;
		//public String name;
		//public String codec;
	}

	class AudioStreamInfo extends StreamInfo {
		public static final int AV_SAMPLE_FMT_NONE = -1;
		public static final int AV_SAMPLE_FMT_U8 = 0;
		public static final int AV_SAMPLE_FMT_S16 = 1;
		public static final int AV_SAMPLE_FMT_S32 = 2;
		public static final int AV_SAMPLE_FMT_FLT = 3;
		public static final int AV_SAMPLE_FMT_DBL = 4;
		
		public int channels;
		public int format;
		public int rate;

		public int getAudioFormat() {
			switch(format) {
			case AV_SAMPLE_FMT_U8:
				return AudioFormat.ENCODING_PCM_8BIT;
			case AV_SAMPLE_FMT_S16:
				return AudioFormat.ENCODING_PCM_16BIT;
			}
			Log.wtf(TAG, "Unsupported sample format");
			return AudioFormat.ENCODING_INVALID;
		}
		
		public int getAudioFormatChannels() {
			switch(channels) {
			case 1:
				return AudioFormat.CHANNEL_OUT_MONO;
			case 2:
				return AudioFormat.CHANNEL_OUT_STEREO;
			case 4:
				return AudioFormat.CHANNEL_OUT_QUAD;
			}
			Log.e(TAG, "Unsupported channel number: " + channels);
			return AudioFormat.CHANNEL_INVALID;
		}
	}
	protected native int nativeGetAudioStreamCount();
	protected native int nativeGetAudioStreamInfo(AudioStreamInfo result, int index);
	
    static {
    	try {
	        System.loadLibrary("ffmpeg");
	        System.loadLibrary("mediaplayer");
	        nativeInit();
    	} catch(UnsatisfiedLinkError ex) {
    		Log.e(TAG, "Could not load native library: " + ex.getMessage());
    		throw ex;
    	}
    }
}
