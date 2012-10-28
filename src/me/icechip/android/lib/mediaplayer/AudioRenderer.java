package me.icechip.android.lib.mediaplayer;

public interface AudioRenderer {
	void render(byte[] frame, final int offset, final int length);
	void close();
}
