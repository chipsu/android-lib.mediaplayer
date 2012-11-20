#include "MediaPlayer.h"

namespace ffmp {

static bool seekTo(AVFormatContext *format, int64_t position) {
    if(!av_seek_frame(format, -1, position, AVSEEK_FLAG_ANY)) {
        return av_seek_frame(format, -1, position, 0);
    }
    return true;
}

MediaPlayer::MediaPlayer() {
    m_format = NULL;
    m_status = S_CLOSE;
    m_clock = 0;
}

MediaPlayer::~MediaPlayer() {
}

bool MediaPlayer::open(const char *path) {
    icecore::AutoLock<> lock(m_lock);
        
    if(m_format != NULL) {
        LOGE("Already open!");
        return false;
    }
    
    if(avformat_open_input(&m_format, path, NULL, NULL) != 0) {
        LOGE("av_open_input_file failed");
        close();
        return false;
    }
    
    if(av_find_stream_info(m_format) < 0) {
        LOGE("av_find_stream_info failed");
        close();
        return false;
    }
    
    av_dump_format(m_format, 0, path, 0);

    openStreamType(AVMEDIA_TYPE_AUDIO, -1);
    openStreamType(AVMEDIA_TYPE_VIDEO, -1);
    openStreamType(AVMEDIA_TYPE_SUBTITLE, -1);

    if(m_streams.size() == 0) {
        LOGE("No playable stream found");
        close();
        return false;
    }
    
    m_status = S_PAUSE;
    
    LOGI("Starting main thread...");
    if(!m_thread.start<MediaPlayer, &MediaPlayer::main>(this)) {
        LOGE("Cannot start thread");
        close();
        return false;
    }
    
    LOGI("Success: file '%s' open", path);
    return true;
}

bool MediaPlayer::isOpen() const {
    return m_format != NULL;
}
    
void MediaPlayer::close() {
    LOGI("Closing...");
    m_lock.lock();
    m_status = S_CLOSE;
    while(m_streams.size()) {
        closeStream(m_streams[0]);
    }
    if(m_format != NULL) {
        av_close_input_file(m_format);
        m_format = NULL;
    }
    m_lock.unlock();
    LOGI("Waiting for threads...");
    m_thread.join();
    LOGI("Done!");
}

bool MediaPlayer::openStream(int index) {
    icecore::AutoLock<> lock(m_lock);
    LOGI("Open stream %d...", index);
    AVCodecContext *codec = getCodec(index);
    if(codec != NULL) {
        if(findStreamByIndex(index) == NULL) {
            Stream *stream = findStreamByType(codec->codec_type);
            if(stream) {
                closeStream(stream);
            }
            return openStreamType(codec->codec_type, index);
        } else {
            LOGW("Stream %d is already open", index);
        }
    } else {
        LOGE("Stream %d does not exist", index);
    }
    LOGE("Open stream %d failed", index);
    return false;
}

bool MediaPlayer::closeStream(int index) {
    icecore::AutoLock<> lock(m_lock);
    LOGI("Close stream %d...", index);
    Stream *stream = findStreamByIndex(index);
    if(stream != NULL) {
        return closeStream(stream);
    } else {
        LOGE("Stream %d is not open", index);
    }
    LOGE("Close stream %d failed, count=%d", index, m_streams.size());
    return false;
}

bool MediaPlayer::play() {
    icecore::AutoLock<> lock(m_lock);
    if(!isOpen()) {
        LOGE("Not open");
        return false;
    }
    m_status = S_PLAY;
    return true;
}

bool MediaPlayer::pause() {
    icecore::AutoLock<> lock(m_lock);
    if(!isOpen()) {
        LOGE("Not open");
        return false;
    }
    m_status = S_PAUSE;
    return true;
}

bool MediaPlayer::seek(uint64_t position) {
    icecore::AutoLock<> lock(m_lock);
    if(!isOpen()) {
        LOGE("Not open");
        return false;
    }
    LOGD("Seeking to %lld...", position);
    if(!seekTo(m_format, position)) {
        LOGE("Seek to %lld failed", position);
        return false;
    }
    LOGW("TODO: Flush stuff...");
    return false;
}

uint64_t MediaPlayer::getPosition() {
    icecore::AutoLock<> lock(m_lock);
    return ffmp_time() - m_clock;
}

void MediaPlayer::main() {
    AVPacket packet;
    LOGI("MediaPlayer running...");
    while(1) {
        icecore::AutoLock<> lock(m_lock);
        if(m_status == S_CLOSE) {
            LOGI("Got close signal");
            break;
        }
        if(m_status == S_PAUSE) {
            lock.unlock();
            ffmp_sleep(10000);
            continue;
        }
        if(av_read_frame(m_format, &packet) >= 0) {
            Stream *stream = findStreamByIndex(packet.stream_index);
            if(stream != NULL) {
                stream->put(packet);
            } else {
                LOGW_ONCE("Stream %d is not open", packet.stream_index);
                av_free_packet(&packet);
            }
        } else {
            LOGW_TIMED(5, "av_read_frame failed");
        }
    }
    LOGI("MediaPlayer exiting...");
}

// TODO: Update master clock, drop audio
void MediaPlayer::onStreamDecode(Stream &stream, Frame &frame) {
    if(frame.pts != AV_NOPTS_VALUE && stream.type() == AVMEDIA_TYPE_VIDEO) {
        microsec_t now = ffmp_time();
        if(frame.pts + m_config.maxFrameDelay < now) {
            LOGW_TIMED(10, "Drop frame: type=%d, pkt_pts=%lld, pts=%llu, now=%llu, diff=%lld, %f", stream.type(), frame.frame->pkt_pts, frame.pts, now, frame.pts - now,
                    av_q2d(stream.stream()->time_base) * frame.frame->pkt_pts);
            frame.free();
            return;
        }
    }
    stream.put(frame);
}

void MediaPlayer::onFrameRender(Stream &stream, Frame &frame) {
    frame.free();
}

void MediaPlayer::onStreamOpen(Stream &stream) {
}

void MediaPlayer::onStreamClose(Stream &stream) {
}

bool MediaPlayer::openStreamType(int type, int index) {
    if(findStreamByType(type)) LOGW("Stream of type %d is already open!", type);
    if(index < 0) index = m_config.lastOpenStream[type];
    if(index < 0 || index >= m_format->nb_streams) {
        for(int i = 0; i < m_format->nb_streams; ++i) {
            if(m_format->streams[i]->codec->codec_type == type) {
                index = i;
                break;
            }
        }
        if(index < 0) {
            LOGE("Cannot find a stream of type %d", type);
            return false;
        }
    } else if(m_format->streams[index]->codec->codec_type != type) {
        LOGE("Stream %d is not of type %d", index, type);
        return false;
    }
    return openStreamIndex(index);
}

bool MediaPlayer::openStreamIndex(int index) {
    if(index < 0 || index >= m_format->nb_streams) {
        LOGE("Bad stream index %d", index);
        return false;
    }
    if(findStreamByIndex(index) != NULL) {
        LOGE("Stream %d is already open", index);
        return false;
    }
    LOGI("Opening stream %d...", index);
    Stream *stream = new Stream(this, m_format->streams[index]);
    if(!stream->open()) {
        LOGE("stream->open failed");
        delete stream;
        return false;
    }
    
    m_streams.push_back(stream);
    
    onStreamOpen(*stream);
    
    LOGI("Success");
    return true;
}

bool MediaPlayer::closeStream(Stream *stream) {
    for(std::vector<Stream*>::iterator it = m_streams.begin(); it != m_streams.end(); ++it) {
        if((*it) == stream) {
            onStreamClose(*stream);
            stream->close();
            delete stream;
            m_streams.erase(it);
            return true;
        }
    }
    LOGE("BUG: Stream does not exist, was it never added to m_streams!?");
    return false;
}

bool MediaPlayer::Stream::open() {
    m_codec = m_stream->codec;
    AVCodec *decoder = avcodec_find_decoder(m_codec->codec_id);
    
    if(decoder == NULL) {
        LOGE("Cannot find a suitable decoder for stream %d", index());
        return false;
    }
    
    // Speed hacks
    // TODO: Config
    m_codec->skip_loop_filter = AVDISCARD_ALL;
    m_codec->skip_frame = AVDISCARD_NONREF;
    m_codec->skip_idct = AVDISCARD_NONREF;
    m_codec->flags2 |= CODEC_FLAG2_FAST;
    //m_codec->lowres = 2;
    
    if(avcodec_open(m_codec, decoder) < 0) {
        LOGE("Cannot open decoder for stream %d", index());
        return false;
    }

    m_asyncDecode = m_mp->m_config.asyncDecode[type()];
    if(m_asyncDecode) {
        m_packetQueue.flush();
        m_packetQueue.setMaxSize(m_mp->m_config.maxPacketQueueSize);
        m_packetQueue.resume();
        if(!m_decodeThread.start<Stream, &Stream::decodeMain>(this)) {
            LOGE("Cannot start thread");
            close();
            return false;
        }
    }
    
    m_asyncRender = m_mp->m_config.asyncRender[type()];
    if(m_asyncRender) {
        m_renderQueue.flush();
        m_renderQueue.setMaxSize(m_mp->m_config.maxRenderQueueSize);
        m_renderQueue.resume();
        if(!m_renderThread.start<Stream, &Stream::renderMain>(this)) {
            LOGE("Cannot start thread");
            close();
            return false;
        }
    }
    
    LOGI("Stream started, using decoder '%s'", decoder->name);
    return true;
} 

void MediaPlayer::Stream::close() {
    LOGI("Closing...");
    if(m_asyncDecode) {
        m_packetQueue.abort();
        LOGI("Waiting for decoder thread...");
        m_decodeThread.join();
        m_packetQueue.flush();
        m_asyncDecode = false;
    }
    if(m_asyncRender) {
        m_renderQueue.abort();
        LOGI("Waiting for render thread...");
        m_renderThread.join();
        m_renderQueue.flush();
        m_asyncRender = false;
    }
    if(m_codec != NULL) {
        avcodec_close(m_codec);
        m_codec = NULL;
    }
    LOGI("Done!");
}


void MediaPlayer::Stream::decodeMain() {
    LOGI("Stream decoder thread %d running...", type());
    AVPacket packet;
    m_mp->onDecodeThreadStart(*this);
    while(1) {
        /*FIXME: NOT NEEDED!? (put waits?
         * if(m_asyncFrame && m_frameQueue.size() > 10) {
            ffmp_sleep(10000); // TODO: Config, abort
            continue;
        }*/
        if(m_packetQueue.get(packet, true)) {
            decode(packet);
        } else if(m_packetQueue.aborted()) {
            LOGI("Decoder thread %d aborted", type());
            break;
        } else {
            LOGD("m_queue.get returned, but no packet!?");
        }
    }
    m_mp->onDecodeThreadStop(*this);
    LOGI("Decoder thread %d done", type());
}

void MediaPlayer::Stream::renderMain() {
    LOGI("Stream render thread %d running...", type());
    Frame frame;
    m_mp->onRenderThreadStart(*this);
    while(1) {
        if(m_renderQueue.get(frame, true)) {
            render(frame);
        } else if(m_renderQueue.aborted()) {
            LOGI("Render thread %d aborted", type());
            break;
        } else {
            LOGD("m_frameQueue.get returned, but no frame!?");
        }
    }
    m_mp->onRenderThreadStop(*this);
    LOGI("Render thread %d done", type());
}

}
