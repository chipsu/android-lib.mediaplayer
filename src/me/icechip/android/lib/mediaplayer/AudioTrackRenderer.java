package me.icechip.android.lib.mediaplayer;

import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Handler;
import android.util.Log;

/**
 * AudioRenderer using Android's AudioTrack class.
 */
public class AudioTrackRenderer implements AudioRenderer {
	public static final String TAG = AudioTrackRenderer.class.getName();
	private AudioTrack mAudioTrack;
	private Handler mHandler = new Handler();
	
	AudioTrackRenderer(int rate, int channels, int format) {
		int bufferSize = AudioTrack.getMinBufferSize(rate, channels, format);
		mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, rate, channels, format, bufferSize * 2, AudioTrack.MODE_STREAM);
	}
	
	/**
	 * @note We assume that MediaPlayer.m_asyncStream is true here, otherwise this would block the main worker too long.
	 */
	@Override
	public void render(byte[] f, final int offset, final int length) {
		mAudioTrack.write(f, offset, length);
		if(mAudioTrack.getPlayState() != AudioTrack.PLAYSTATE_PLAYING) {
			mAudioTrack.play();
		}
		// TODO: if(MediaPlayer.m_asyncStream) { post(..) } else { write }
		/*final byte[] frame = f.clone(); // FIXME!
		mHandler.post(new Runnable() {
			public void run() {
				Log.d(TAG, "pos=" + mAudioTrack.getPlaybackHeadPosition());
				mAudioTrack.write(frame, offset, length);

				if(mAudioTrack.getPlayState() != AudioTrack.PLAYSTATE_PLAYING) {
					mAudioTrack.play();
				}
			}
		});*/
		/*mAudioTrack.write(f, offset, length);
		if(mAudioTrack.getPlayState() != AudioTrack.PLAYSTATE_PLAYING) {
			mAudioTrack.play();
		}*/
	}
	
	@Override
	public void close() {
		mAudioTrack.stop();
	}
	
	protected void finalize() {
		if(mAudioTrack != null) {
			mAudioTrack.stop();
			mAudioTrack.flush();
			mAudioTrack.release();
		}
	}
}
