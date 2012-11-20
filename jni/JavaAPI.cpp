#include "JavaAPI.h"

std::vector<JavaMediaPlayer*> JavaMediaPlayer::s_instances;

#ifdef ICE_OMX

extern AVCodec ff_h264_decoder;
extern AVCodec ff_libstagefright_h264_decoder;

#endif
   
static void global_exit() {
    icecore::Profiler::reportAll();
}
    
static void global_init() {
    static bool done = false;
    if(!done) {
        atexit(global_exit);
        //av_log_set_level(AV_LOG_DEBUG);
        av_log_set_level(AV_LOG_ERROR);
        av_log_set_callback(android_av_log_callback);
        av_register_all();
#ifdef ICE_OMX
        LOGI("ff_libstagefright_h264_decoder=%s", ff_libstagefright_h264_decoder.name);
        avcodec_register(&ff_libstagefright_h264_decoder);
#endif
        done = true;
    }
}

bool JavaMediaPlayer::openStreamIndex(int index) {
#ifdef ICE_OMX
    if(index >= 0 && index < m_format->nb_streams && m_format->nb_streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
        LOGI("Attempting hardware decoding...");
        LOGI("ff_libstagefright_h264_decoder=%s", ff_libstagefright_h264_decoder.name);
        ff_h264_decoder.id = AV_CODEC_ID_NONE;
        if(!ffmp::MediaPlayer::openStreamIndex(index)) {
            LOGI("No hardware decoder found!");
            ff_h264_decoder.id = AV_CODEC_ID_H264;
        }
    }
#endif
    return ffmp::MediaPlayer::openStreamIndex(index);
}

static JavaMediaPlayer *mp(JNIEnv *env, jobject obj) {
    return JavaMediaPlayer::getInstance(env, obj);
}

int JAVA_METHOD(nativeInit)(JNIEnv *env, jobject obj) {
    global_init();
    return 0;
}

int JAVA_METHOD(nativeOpen)(JNIEnv *env, jobject obj, jstring path) {
    const char *str = env->GetStringUTFChars(path, NULL);
    return mp(env, obj)->open(str) ? 0 : -1;
}

int JAVA_METHOD(nativePlay)(JNIEnv *env, jobject obj) {
    return mp(env, obj)->play() ? 0 : -1;
}

int JAVA_METHOD(nativeClose)(JNIEnv *env, jobject obj) {
    JavaMediaPlayer::destroyInstance(obj);
    return 0;
}

int JAVA_METHOD(nativeOpenStream)(JNIEnv *env, jobject obj, int index) {
    return mp(env, obj)->openStream(index) ? 0 : -1;
}

int JAVA_METHOD(nativeCloseStream)(JNIEnv *env, jobject obj, int index) {
    return mp(env, obj)->closeStream(index) ? 0 : -1;
}

int JAVA_METHOD(nativeRender)(JNIEnv *env, jobject obj) {
    return mp(env, obj)->drawGL() ? 0 : -1;
}

int JAVA_METHOD(nativeGetAudioStreamInfo)(JNIEnv *env, jobject obj, jobject result, int index) {
    return mp(env, obj)->getAudioStreamInfo(result, index) ? 0 : -1;
}
