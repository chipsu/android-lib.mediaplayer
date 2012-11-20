#pragma once

#include "Main.h"
#include "Queue.h"

namespace ffmp {

struct MediaPlayer {
    MediaPlayer();
    virtual ~MediaPlayer();
    // TODO: openOpts VIDEO|AUDIO|...streamIndex
    virtual bool open(const char *path);
    bool isOpen() const;
    virtual void close();
    virtual bool openStream(int index);
    virtual bool closeStream(int index);
    bool play();
    bool pause();
    
    /**
     * Seek to position (AV_TIME_BASE, microseconds)
     */
    bool seek(uint64_t position);
    uint64_t getPosition();
    
    uint64_t getClock() {
        icecore::AutoLock<> lock(m_timeLock);
        if(m_clock == 0) m_clock = ffmp_time();
        return m_clock;
    }
    
    void setClock(uint64_t value) {
        icecore::AutoLock<> lock(m_timeLock);
        m_clock = value;
    }
    
protected:
    enum Status {
        S_PLAY,
        S_PAUSE,
        S_CLOSE,
    };
    
    struct Frame {
        Frame() {
            frame = NULL;
            codec = NULL;
            pts = AV_NOPTS_VALUE;
        }
        ~Frame() {}
        void free() {
            if(frame) {
                av_free(frame);
                frame = NULL;
            }
            codec = NULL;
        }
        void sync() {
            if(pts != AV_NOPTS_VALUE) {
                int64_t delay = pts - ffmp_time();
                if(delay > 0) {
                    // TODO: Compensate when usleep oversleeps?
                    // TODO: abort() long sleeps...
                    //LOGD(" -- Sleep type=%d, delay=%lld", stream.type(), delay);
                    ffmp_sleep(delay);
                    //LOGD(" -- Done, remaining=%lld", frame.pts - ffmp::ffmp_time());
                }
            }
        }
        AVCodecContext *codec;
        AVFrame *frame;
        microsec_t pts;
    };

    struct AVPacketManager {
        static void free(AVPacket &pkt) { av_free_packet(&pkt); }
    };
    typedef Queue<AVPacket, AVPacketManager> PacketQueue;

    struct FrameManager {
        static void free(Frame &f) { f.free(); }
    };
    typedef Queue<Frame, FrameManager> FrameQueue;
    
    struct Stream {
        explicit Stream(MediaPlayer *mp, AVStream *stream) : m_mp(mp), m_stream(stream) {
            m_codec = NULL;
        }
        ~Stream() {}
        bool open();
        void close();
        void decodeMain();
        void renderMain();
        int index() const { return m_stream->index; }
        int type() const { return m_stream->codec->codec_type; }
        AVStream *stream() const { return m_stream; }
        
        void put(AVPacket &packet) {
            if(m_asyncDecode) {
                m_packetQueue.put(packet);
            } else {
                decode(packet);
            }
        }
        
        void put(Frame &frame) {
            if(m_asyncRender) {
                m_renderQueue.put(frame);
            } else {
                render(frame);
            }
        }
        
        void render(Frame &frame) {
            m_mp->onFrameRender(*this, frame);
        }
        
        void decode(AVPacket &packet) {
            Frame frame;
            if(decode(frame, packet) && frame.frame != NULL) {
                frame.codec = m_codec; // FIXME
                m_mp->onStreamDecode(*this, frame);
            }
            av_free_packet(&packet);
        }
        
        bool decode(Frame &frame, AVPacket &packet) {
            frame.frame = avcodec_alloc_frame(); // TODO: m_frame?
            int done = 0;
            int result = 0;
            switch(type()) {
            case AVMEDIA_TYPE_VIDEO:
                result = avcodec_decode_video2(m_codec, frame.frame, &done, &packet);
                break;
            case AVMEDIA_TYPE_AUDIO:
                result = avcodec_decode_audio4(m_codec, frame.frame, &done, &packet);
                break;
            default:
                LOGE("Unsupported media type %d", type());
                result = -1;
                break;
            }
            if(done) {
                frame.pts = av_frame_get_best_effort_timestamp(frame.frame);
                //frame.pts = frame.frame->pkt_dts;
                if(frame.pts != AV_NOPTS_VALUE) {
                    //LOGD("PTS=%f", frame.pts * av_q2d(m_stream->time_base));
                    frame.pts *= av_q2d(m_stream->time_base) * 1000000.0;
                    frame.pts += m_mp->getClock();
                } else {
                    //frame.pts = m_clock + av_q2d(m_stream->time_base) * 1000000.0;
                }
                //LOGD("clock=%lld, PTS=%lld, pkt_pts=%lld, delay=%lld", m_clock, frame.pts, frame.frame->pkt_pts, frame.pts - av_gettime());
                return true;
            }
            frame.free();
            if(result < 0) {
                LOGE("decode frame failed: %d", result);
                return false;
            }
            return true;
        }
        
    protected:
        MediaPlayer *m_mp;
        AVStream *m_stream;
        AVCodecContext *m_codec;
        PacketQueue m_packetQueue;
        FrameQueue m_renderQueue;
        icecore::Thread m_decodeThread;
        icecore::Thread m_renderThread;
        bool m_asyncDecode;
        bool m_asyncRender;
    };
    
    typedef std::vector<Stream*> StreamList;
    
    virtual void onStreamOpen(Stream &stream);
    virtual void onStreamClose(Stream &stream);
    virtual void onStreamDecode(Stream &stream, Frame &frame);
    virtual void onFrameRender(Stream &stream, Frame &frame);
    virtual void onDecodeThreadStart(Stream &stream) {}
    virtual void onDecodeThreadStop(Stream &stream) {}
    virtual void onRenderThreadStart(Stream &stream) {}
    virtual void onRenderThreadStop(Stream &stream) {}
    
    virtual bool openStreamType(int type, int index);
    virtual bool openStreamIndex(int index);
    virtual bool closeStream(Stream *stream);
    
    virtual void main();
    
    Stream *findStreamByType(int type) const {
        for(StreamList::const_iterator it = m_streams.begin(); it != m_streams.end(); ++it) {
            if((*it)->type() == type) {
                return *it;
            }
        }
        return NULL;
    }
    
    Stream *findStreamByIndex(int index) const {
        for(StreamList::const_iterator it = m_streams.begin(); it != m_streams.end(); ++it) {
            if((*it)->index() == index) {
                return *it;
            }
        }
        return NULL;
    }
    
    AVCodecContext *getCodec(int index) const {
        return m_format != NULL && index >= 0 && index < m_format->nb_streams ? m_format->streams[index]->codec : NULL;
    }
    
    Config m_config;
    icecore::Mutex m_timeLock;
    icecore::RMutex m_lock;
    icecore::Thread m_thread;
    AVFormatContext *m_format;
    StreamList m_streams;
    Status m_status;
    uint64_t m_clock;
};
    
}
