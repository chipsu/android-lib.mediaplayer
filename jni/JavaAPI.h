// Copyright (c) 2012 - Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
#pragma once

#include <jni.h>
#include <ffmp/MediaPlayer.h>
#include "Matrix4.h"
#include "GLRenderer.h"

#define JAVA_METHOD(name) Java_me_icechip_android_lib_mediaplayer_MediaPlayer_##name

inline void android_av_log_callback(void *ptr, int level, const char *format, va_list args) {
	char buffer[ICE_LOG_BUFFER_SIZE];
	vsnprintf(buffer, ICE_LOG_BUFFER_SIZE, format, args);
	LOGI(buffer);
}

extern "C" {
    JNIEXPORT int JNICALL JAVA_METHOD(nativeInit)(JNIEnv *env, jobject obj);
    JNIEXPORT int JNICALL JAVA_METHOD(nativeOpen)(JNIEnv *env, jobject obj, jstring path);
    JNIEXPORT int JNICALL JAVA_METHOD(nativePlay)(JNIEnv *env, jobject obj);
    JNIEXPORT int JNICALL JAVA_METHOD(nativeClose)(JNIEnv *env, jobject obj);
    JNIEXPORT int JNICALL JAVA_METHOD(nativeOpenStream)(JNIEnv *env, jobject obj, int index);
    JNIEXPORT int JNICALL JAVA_METHOD(nativeCloseStream)(JNIEnv *env, jobject obj, int index);
    JNIEXPORT int JNICALL JAVA_METHOD(nativeRender)(JNIEnv *env, jobject obj);
    JNIEXPORT int JNICALL JAVA_METHOD(nativeGetAudioStreamInfo)(JNIEnv *env, jobject obj, jobject result, int index);
}

JavaVM *gJVM;

struct Env {
	Env() {
		m_env = attachCurrentThread();
		ICE_ASSERT(m_env);
	}
	
	explicit Env(JNIEnv *env) {
		m_env = env ? env : attachCurrentThread();
		ICE_ASSERT(m_env);
	}
	
	virtual ~Env() {
	}
	
	JNIEnv *env() const {
		ICE_ASSERT(m_env);
		return m_env;
	}
	
	void setIntFieldValue(jobject obj, const char *name, int value) {
		jfieldID id = getField(obj, name, "I");
		m_env->SetIntField(obj, id, value);
	}
	
	jclass getObjectClass(jobject obj) {
		jclass clazz = m_env->GetObjectClass(obj);
		if(!clazz) LOGE("GetObjectClass failed: obj=%d", obj);
		return clazz;
	}
	
	jfieldID getField(jobject obj, const char *name, const char *signature) {
		jclass clazz = getObjectClass(obj);
		jfieldID result = m_env->GetFieldID(clazz, name, signature);
		if(!result) LOGE("GetFieldID failed: obj=%d, name=%s, signature=%s", obj, name, signature);
		m_env->DeleteLocalRef(clazz);
		return result;
	}
		
	jmethodID getMethod(jobject obj, const char *name, const char *signature) {
		jclass clazz = m_env->GetObjectClass(obj);
		jmethodID result = m_env->GetMethodID(clazz, name, signature);
		if(!result) LOGE("GetMethodID failed: obj=%d, name=%s, signature=%s", obj, name, signature);
		m_env->DeleteLocalRef(clazz);
		return result;
	}
	
	static JNIEnv *attachCurrentThread() {
		JNIEnv *env = NULL;
		int status = gJVM->AttachCurrentThread(&env, NULL);
		if(status < 0) {
			LOGE("gJVM->AttachCurrentThread failed");
		}
		return env;
	}
	
protected:
	JNIEnv *m_env;
};

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
	gJVM = jvm;
	JNIEnv *env = NULL;
	int status = gJVM->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status < 0) {
		LOGE("gJVM->GetEnv failed");
		status = gJVM->AttachCurrentThread(&env, NULL);
		if(status < 0) {
			LOGE("AttachCurrentThread failed");
			return JNI_ERR;
		}
	}
	LOGW("JNI_OnLoad OK");
	LOGW(__DATE__ " - "  __TIME__);
	return JNI_VERSION_1_6;
}

struct JavaMediaPlayer : ffmp::MediaPlayer {
	struct Renderer : Env {
		virtual void render(Frame &frame) = 0;
		virtual void finish() { LOGE("Not supported"); }
		virtual ~Renderer() {}
	protected:
		explicit Renderer(JavaMediaPlayer *mp, JNIEnv *env = NULL) : Env(env), m_mp(mp) {
		}
		JavaMediaPlayer *m_mp;
	};
	
	struct JavaAudioRenderer : public Renderer {
		jbyteArray m_data;
		jmethodID m_renderAudioFrame;
		explicit JavaAudioRenderer(JavaMediaPlayer *mp, JNIEnv *env = NULL) : Renderer(mp, env), m_data(NULL) {
			m_renderAudioFrame = getMethod(mp->m_obj, "renderAudioFrame", "([BII)V");
		}
		virtual ~JavaAudioRenderer() {
			if(m_data != NULL) {
				m_env->DeleteLocalRef(m_data);
			}
		}
		virtual void render(Frame &frame) {
			ICE_PROFILE(NULL);
			int size = av_samples_get_buffer_size(NULL, frame.codec->channels, frame.frame->nb_samples, frame.codec->sample_fmt, 1);
			if(m_data == NULL || size > m_env->GetArrayLength(m_data)) {
				if(m_data != NULL) {
					m_env->DeleteLocalRef(m_data);
				}
				LOGD("Resize AudioBuffer => %d", size);
				m_data = m_env->NewByteArray(size);
			}
			jbyte *ptr = m_env->GetByteArrayElements(m_data, NULL);
			memcpy(ptr, frame.frame->data[0], size);
			m_env->ReleaseByteArrayElements(m_data, ptr, 0);
			m_env->CallVoidMethod(m_mp->m_obj, m_renderAudioFrame, m_data, 0, size);
		}
	};
	
	struct GLVideoRenderer : public Renderer {
		RendererGLES20 *m_renderer;
		FrameQueue m_queue;
		jmethodID m_onVideoFrameReady;
		
		explicit GLVideoRenderer(JavaMediaPlayer *mp, JNIEnv *env = NULL) : Renderer(mp, env) {
			m_renderer = NULL;
			m_onVideoFrameReady = getMethod(mp->m_obj, "onVideoFrameReady", "()V");
			m_queue.setMaxSize(mp->m_config.maxRenderQueueSize);
		}
		
		virtual ~GLVideoRenderer() {
			delete m_renderer;
			m_queue.flush();
		}
		
		virtual void render(Frame &frame) {
			m_queue.put(frame);
			m_env->CallVoidMethod(m_mp->m_obj, m_onVideoFrameReady);
			frame.frame = NULL; // FIXME! (frame refcounting?!)
		}
		
		void finish() {
			Frame frame;
			if(m_queue.get(frame, false)) {
				if(m_renderer == NULL) {
					m_renderer = new RendererGLES20(frame.codec);
				}
				m_renderer->render(frame.frame);
				frame.sync();
				frame.free();
			} else {
				LOGW("m_queue.get failed");
			}
		}
	};
	
	static std::vector<JavaMediaPlayer*> s_instances;
	static JavaMediaPlayer *getInstance(JNIEnv *env, jobject obj, bool create = true) {
		for(std::vector<JavaMediaPlayer*>::iterator it = s_instances.begin(); it != s_instances.end(); ++it) {
			if((*it)->m_obj == obj) {
				return *it;
			}
		}
		if(create) {
			JavaMediaPlayer *mp = new JavaMediaPlayer(env, obj);
			s_instances.push_back(mp);
			return mp;
		}
		return NULL;
	}
	
	static void destroyInstance(jobject obj) {
		for(std::vector<JavaMediaPlayer*>::iterator it = s_instances.begin(); it != s_instances.end(); ++it) {
			if((*it)->m_obj == obj) {
				(*it)->close();
				delete *it;
				s_instances.erase(it);
				break;
			}
		}
	}
	
	JavaMediaPlayer(JNIEnv *env, jobject obj) {
		m_env = new Env(env);
		m_obj = env->NewGlobalRef(obj);
		m_threadEnv = NULL;
		m_audioRenderer = NULL;
		m_videoRenderer = NULL;
		m_onStreamOpen = m_env->getMethod(m_obj, "onStreamOpen", "(II)V");
		m_onStreamClose = m_env->getMethod(m_obj, "onStreamClose", "(II)V");
		m_config.asyncDecode[AVMEDIA_TYPE_VIDEO] = true;
		m_config.asyncDecode[AVMEDIA_TYPE_AUDIO] = true;
		m_config.asyncDecode[AVMEDIA_TYPE_SUBTITLE] = true;
		m_config.asyncRender[AVMEDIA_TYPE_VIDEO] = false; // OpenGL already has its own thread, use that
		m_config.asyncRender[AVMEDIA_TYPE_AUDIO] = true;
		m_config.asyncRender[AVMEDIA_TYPE_SUBTITLE] = true;
	}
	
	virtual ~JavaMediaPlayer() {
		m_env->env()->DeleteGlobalRef(m_obj);
		delete m_env;
		ICE_ASSERT(m_audioRenderer == NULL);
		ICE_ASSERT(m_videoRenderer == NULL);
		ICE_ASSERT(m_threadEnv == NULL);
	}
	
	virtual void close() {
		icecore::Profiler::reportAll();
		ffmp::MediaPlayer::close();
	}
	
	
	bool drawGL() {
		icecore::AutoLock<> lock(m_videoLock);
		if(m_videoRenderer) {
			m_videoRenderer->finish();
			return true;
		}
		return false;
	}
	
	bool getAudioStreamInfo(jobject result, int index) {
		icecore::AutoLock<> lock(m_lock);
		AVCodecContext *codec = getCodec(index);
		if(codec != NULL && codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			LOGD("Stream %d: channels=%d, format=%d, rate=%d", index, codec->channels, codec->sample_fmt, codec->sample_rate);
			m_env->setIntFieldValue(result, "index", index);
			m_env->setIntFieldValue(result, "open", findStreamByIndex(index) ? 1 : 0);
			m_env->setIntFieldValue(result, "channels", codec->channels);
			m_env->setIntFieldValue(result, "format", codec->sample_fmt);
			m_env->setIntFieldValue(result, "rate", codec->sample_rate);
			return true;
		} else if(codec) {
			LOGE("Stream %d is not an audio stream! codec_type=%d", index, codec->codec_type);
		} else {
			LOGW("Stream %d does not exist", index);
		}
		return false;
	}
	
protected:
	virtual void onStreamOpen(Stream &stream) {
		LOGI("type=%d, index=%d", stream.type(), stream.index());
		m_env->env()->CallVoidMethod(m_obj, m_onStreamOpen, stream.type(), stream.index());
		ffmp::MediaPlayer::onStreamOpen(stream);
	}
	
	virtual void onStreamClose(Stream &stream) {
		LOGI("type=%d, index=%d", stream.type(), stream.index());
		m_env->env()->CallVoidMethod(m_obj, m_onStreamClose, stream.type(), stream.index());
		ffmp::MediaPlayer::onStreamClose(stream);
	}
	
	virtual void onFrameRender(Stream &stream, Frame &frame) {
		switch(stream.type()) {
		case AVMEDIA_TYPE_AUDIO: {
				icecore::AutoLock<> lock(m_audioLock);
				if(m_audioRenderer) {
					m_audioRenderer->render(frame);
				}
			}
			break;
		case AVMEDIA_TYPE_VIDEO: {
				//LOGW("Render: t=%lld, pts=%lld, delay=%lld", ffmp::ffmp_time(), frame.pts, frame.pts - ffmp::ffmp_time());
				icecore::AutoLock<> lock(m_videoLock);
				if(m_videoRenderer) {
					m_videoRenderer->render(frame);
				}
			}
			break;
		}
		frame.free();
		if(m_config.asyncRender[stream.type()]) {
			frame.sync();
		}
	}
	
	virtual void onDecodeThreadStart(Stream &stream) {
		if(!m_config.asyncRender[stream.type()]) createRenderer(stream.type());
		ffmp::MediaPlayer::onDecodeThreadStart(stream);
	}
	
	virtual void onDecodeThreadStop(Stream &stream) {
		if(!m_config.asyncRender[stream.type()]) destroyRenderer(stream.type());
		ffmp::MediaPlayer::onDecodeThreadStop(stream);
	}
	
	virtual void onRenderThreadStart(Stream &stream) {
		if(m_config.asyncRender[stream.type()]) createRenderer(stream.type());
		ffmp::MediaPlayer::onRenderThreadStart(stream);
	}
	
	virtual void onRenderThreadStop(Stream &stream) {
		if(m_config.asyncRender[stream.type()]) destroyRenderer(stream.type());
		ffmp::MediaPlayer::onRenderThreadStop(stream);
	}
	
	virtual void main() {
		ICE_ASSERT(m_threadEnv == NULL);
		m_threadEnv = new Env();
		if(!m_config.asyncDecode[AVMEDIA_TYPE_VIDEO] && !m_config.asyncRender[AVMEDIA_TYPE_VIDEO]) createRenderer(AVMEDIA_TYPE_VIDEO);
		if(!m_config.asyncDecode[AVMEDIA_TYPE_AUDIO] && !m_config.asyncRender[AVMEDIA_TYPE_AUDIO]) createRenderer(AVMEDIA_TYPE_AUDIO);
		if(!m_config.asyncDecode[AVMEDIA_TYPE_VIDEO] && !m_config.asyncRender[AVMEDIA_TYPE_SUBTITLE]) createRenderer(AVMEDIA_TYPE_SUBTITLE);
		ffmp::MediaPlayer::main();
		if(!m_config.asyncDecode[AVMEDIA_TYPE_VIDEO] && !m_config.asyncRender[AVMEDIA_TYPE_VIDEO]) destroyRenderer(AVMEDIA_TYPE_VIDEO);
		if(!m_config.asyncDecode[AVMEDIA_TYPE_AUDIO] && !m_config.asyncRender[AVMEDIA_TYPE_AUDIO]) destroyRenderer(AVMEDIA_TYPE_AUDIO);
		if(!m_config.asyncDecode[AVMEDIA_TYPE_VIDEO] && !m_config.asyncRender[AVMEDIA_TYPE_SUBTITLE]) destroyRenderer(AVMEDIA_TYPE_SUBTITLE);
		icecore::safe_delete(m_threadEnv);
	}

	bool openStreamIndex(int index);

	void createRenderer(int type) {
		LOGI("type=%d", type);
		switch(type) {
		case AVMEDIA_TYPE_VIDEO: {
				icecore::AutoLock<> lock(m_videoLock);
				ICE_ASSERT(m_videoRenderer == NULL);
				m_videoRenderer = new GLVideoRenderer(this);
			}
			break;
		case AVMEDIA_TYPE_AUDIO: {
				icecore::AutoLock<> lock(m_audioLock);
				ICE_ASSERT(m_audioRenderer == NULL);
				m_audioRenderer = new JavaAudioRenderer(this);
			}
			break;
		}
	}
	
	void destroyRenderer(int type) {
		LOGI("type=%d", type);
		switch(type) {
		case AVMEDIA_TYPE_VIDEO: {
				icecore::AutoLock<> lock(m_videoLock);
				icecore::safe_delete(m_videoRenderer);
			}
			break;
		case AVMEDIA_TYPE_AUDIO: {
				icecore::AutoLock<> lock(m_audioLock);
				icecore::safe_delete(m_audioRenderer);
			}
			break;
		}
	}

	icecore::Mutex m_audioLock;
	icecore::Mutex m_videoLock;
	jobject m_obj;
	Env *m_env;
	Env *m_threadEnv;
	Renderer *m_audioRenderer;
	Renderer *m_videoRenderer;
	jmethodID m_onStreamOpen;
	jmethodID m_onStreamClose;
};

