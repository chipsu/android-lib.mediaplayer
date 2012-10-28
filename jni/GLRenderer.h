// Copyright (c) 2012 - Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
#pragma once

#include <icecore/Profiler.h>
#include <icegl/Texture.h>
#include <icegl/Program.h>
#include <icegl/Shader.h>
#include "Matrix4.h"

extern "C" {
	#include <libswscale/swscale.h>
}

#ifndef ICE_SWS_FORMAT
	#define ICE_SWS_FORMAT PIX_FMT_RGBA
#endif

#ifndef ICE_SWS_SCALE
	//#define ICE_SWS_SCALE SWS_FAST_BILINEAR
	#define ICE_SWS_SCALE SWS_AREA
#endif

// -100 to 100
inline void MakeColorMatrix(Matrix4 &matrix, int brightness, int contrast, int hue, int saturation, bool yuv) {
	const float b = brightness / 200.0;
	const float c = contrast / 200.0 + 1.0;
	const float h = hue / 200.0;
	const float s = saturation / 200.0 + 1.0;
	
	const float cosH = cos(M_PI * h);
	const float sinH = sin(M_PI * h);
	
	const float h11 = -0.4728 * cosH + 0.7954 * sinH + 1.4728;
	const float h21 = -0.9253 * cosH - 0.0118 * sinH + 0.9523;
	const float h31 =  0.4525 * cosH + 0.8072 * sinH - 0.4524;
	
	const float h12 =  1.4728 * cosH - 1.3728 * sinH - 1.4728;
	const float h22 =  1.9253 * cosH + 0.5891 * sinH - 0.9253;
	const float h32 = -0.4525 * cosH - 1.9619 * sinH + 0.4525;
	
	const float h13 =  1.4728 * cosH - 0.2181 * sinH - 1.4728;
	const float h23 =  0.9253 * cosH + 1.1665 * sinH - 0.9253;
	const float h33 =  0.5475 * cosH - 1.3846 * sinH + 0.4525;
	
	const float sr = (1.0 - s) * 0.3086;
	const float sg = (1.0 - s) * 0.6094;
	const float sb = (1.0 - s) * 0.0820;
	
	const float sr_s = sr + s;
	const float sg_s = sg + s;
	const float sb_s = sr + s;
	
	const float m4 = (s + sr + sg + sb) * (0.5 - 0.5 * c + b);
	
	matrix(0, 0) = c * (sr_s * h11 + sg * h21 + sb * h31);
	matrix(0, 1) = c * (sr_s * h12 + sg * h22 + sb * h32);
	matrix(0, 2) = c * (sr_s * h13 + sg * h23 + sb * h33);
	matrix(0, 3) = m4;
	
	matrix(1, 0) = c * (sr * h11 + sg_s * h21 + sb * h31);
	matrix(1, 1) = c * (sr * h12 + sg_s * h22 + sb * h32);
	matrix(1, 2) = c * (sr * h13 + sg_s * h23 + sb * h33);
	matrix(1, 3) = m4;
	
	matrix(2, 0) = c * (sr * h11 + sg * h21 + sb_s * h31);
	matrix(2, 1) = c * (sr * h12 + sg * h22 + sb_s * h32);
	matrix(2, 2) = c * (sr * h13 + sg * h23 + sb_s * h33);
	matrix(2, 3) = m4;
	
	matrix(3, 0) = 0.0;
	matrix(3, 1) = 0.0;
	matrix(3, 2) = 0.0;
	matrix(3, 3) = 1.0;
	
	if(yuv)  {
		matrix = matrix * Matrix4(
			1.0,  0.000,  1.140, -0.5700,
			1.0, -0.394, -0.581,  0.4875,
			1.0,  2.028,  0.000, -1.0140,
			0.0,  0.000,  0.000,  1.0000
		);
	}
}

struct RendererGLES20 {
    enum RenderMode {
        RM_YUVRGBA,
        RM_YUV,
        RM_FORCE_SWS,
    };
    
    typedef icegl::Program GLProgram;
    typedef icegl::Texture GLTexture;
    typedef icegl::Shader GLShader;
	
	explicit RendererGLES20(AVCodecContext *codec) {
		m_codec = codec;
		m_sws = NULL;
		m_frame = NULL;
		m_frameBuffer = NULL;
		m_program = NULL;
		m_dirty = true;
		m_yuvRgbaMap = NULL;
        m_renderMode = RM_YUVRGBA;
	
		static const float vertices[] = {
			-1, -1,
			+1, -1,
			+1, +1,
			-1, +1,
		};
		static const int vertexType = GL_TRIANGLES;
		static const float texCoords[] = {
			0, 1,
			1, 1,
			1, 0,
			0, 0
		};
		static const int numIndices = 6;
		static const unsigned short indices[] = { 0, 1, 2, 0, 2, 3 };

		memcpy(m_vertices, vertices, sizeof(vertices));
		m_vertexType = vertexType;
		memcpy(m_texCoords, texCoords, sizeof(texCoords));
		
		memcpy(m_indices, indices, sizeof(indices));
		m_numIndices = numIndices;
		
		_init();
	}
	
	virtual ~RendererGLES20() {
		GLTexture::unbindAll();
		for(int i = 0; i < m_textures.size(); ++i) {
			delete m_textures[i];
		}
		delete m_yuvRgbaMap;
		m_textures.clear();
		GLProgram::bind(NULL);
		delete m_program;
		_deleteFrame();
		LOGI("Renderer destroyed");
	}
	
	void render(AVFrame *frame) {
		ICE_PROFILE(NULL);
		
		if(m_frame != NULL) {
			_swscale(frame);
			_renderRGBA(m_frame);
		} else {
			switch(m_codec->pix_fmt) {
			case PIX_FMT_YUV420P:
                switch(m_renderMode) {
                case RM_YUVRGBA:
				    _renderYUVRGBA(frame);
                    break;
                case RM_YUV:
				    _renderYUV(frame);
                    break;
                default:
				    LOGE("Unsupported RenderMode 0x%x!", m_renderMode);
                    break;
                }
				break;
			default:
				LOGE("Unsupported direct format %d!", m_codec->pix_fmt);
				break;
			}
		}
		
		if(m_dirty) {
			_bind(frame);
		}
		
		_render();
		
        // FIXME: AVFrames might be freed before glTexSubImage2D has completed...
        glFinish();
	}
	
	void invalidate() {
		m_dirty = true;
	}
	
	void _swscale(AVFrame *frame) {
		ICE_PROFILE(NULL);
		sws_scale(m_sws, frame->data, frame->linesize, 0, m_codec->height, m_frame->data, m_frame->linesize);
	}
	
	void _renderYUV(AVFrame *frame) {
		ICE_PROFILE(NULL);
		for(int i = 0; i < 3; ++i) {
			int w = frame->linesize[i];
			int h = i ? m_codec->height / 2 : m_codec->height;
			GLTexture::bind(m_textures[i], i);
			m_textures[i]->upload(w, h, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[i]);
		}
	}

    void _renderYUVRGBA(AVFrame *frame) {
		ICE_PROFILE(NULL);
		for(int i = 0; i < 3; ++i) {
			int w = frame->linesize[i];
			int h = i ? m_codec->height / 2 : m_codec->height;
			GLTexture::bind(m_textures[i], i);
			m_textures[i]->upload(w / 4, h, GL_RGBA, GL_UNSIGNED_BYTE, frame->data[i]);
		}
		if(m_yuvRgbaMap == NULL) {
			_createRGBAYUVMask(frame);
		}
    }
	
	void _renderRGBA(AVFrame *frame) {
		ICE_PROFILE(NULL);
		int format = GL_RGBA;
		switch(ICE_SWS_FORMAT) {
		case PIX_FMT_RGB24:
			format = GL_RGB;
			break;
		case PIX_FMT_RGBA:
			format = GL_RGBA;
			break;
		default:
			LOGE("Unknown sws format: %d!", ICE_SWS_FORMAT);
		}
		GLTexture::bind(m_textures[0], 0);
		m_textures[0]->upload(m_codec->width, m_codec->height, format, GL_UNSIGNED_BYTE, frame->data[0]);
	}
	
	void _render() {
		ICE_PROFILE(NULL);
		for(int i = 0; i < m_textures.size(); ++i) {
			GLTexture::bind(m_textures[i], i);
		}
		if(m_yuvRgbaMap != NULL) {
			GLTexture::bind(m_yuvRgbaMap, 3);
		}
		glDrawElements(m_vertexType, m_numIndices, GL_UNSIGNED_SHORT, m_indices);
		checkGlError();
	}
	
	void _bind(AVFrame *frame) {
		ICE_PROFILE(NULL);
		GLProgram::bind(m_program);
		
		m_texCoords[2] = m_texCoords[4] = (float)m_codec->width / (float)frame->linesize[0];
		
		glVertexAttribPointer(m_aPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, m_vertices);
		glEnableVertexAttribArray(m_aPositionHandle);

		if(m_aTexCoordHandle != -1) {
			glVertexAttribPointer(m_aTexCoordHandle, 2, GL_FLOAT, GL_FALSE, 0, m_texCoords);
			glEnableVertexAttribArray(m_aTexCoordHandle);
		}
		
		if(m_uColorMatrixHandle != -1) {
			glUniformMatrix4fv(m_uColorMatrixHandle, 1, GL_FALSE, m_colorMatrix.ptr());
		}
		
		for(int i = 0; i < 4; ++i) {
			if(m_uTextureHandle[i] != -1) {
				glUniform1i(m_uTextureHandle[i], i);
			}
		}
		
		m_dirty = false;
	}
	
	void _init() {
		ICE_PROFILE(NULL);
		LOGI("Initializing renderer, pix_fmt=0x%x", m_codec->pix_fmt);
		m_program = new GLProgram();
		m_program->vert(icegl::g_shader_vert);
		
		int textureCount = 1;
		
		if(m_renderMode != RM_FORCE_SWS) {
			switch(m_codec->pix_fmt) {
			case PIX_FMT_YUV420P:
                switch(m_renderMode) {
                case RM_YUVRGBA:
					m_program->frag(icegl::g_shader_rgba_yuv_frag);
    				textureCount = 3;
                    break;
                case RM_YUV:
					m_program->frag(icegl::g_shader_yuv_frag);
    				textureCount = 3;
				    break;
                default:
				    LOGE("Unsupported RenderMode 0x%x!", m_renderMode);
                    break;
                }
				break;
			case PIX_FMT_RGB24:
			case PIX_FMT_RGBA:
				m_program->frag(icegl::g_shader_rgb_frag);
				break;
			default:
				LOGW("No shader for pix_fmt 0x%x", m_codec->pix_fmt);
			}
		}
		
		if(!m_program->link()) {
			LOGW("Using SWS! RenderMode=0x%x", m_renderMode);
			m_program->clear();
			m_program->vert(icegl::g_shader_vert);
			m_program->frag(icegl::g_shader_rgb_frag);
			if(!m_program->link()) {
				LOGE("FATAL ERROR: Could not link fallback shader!");
			}
			_createFrame(ICE_SWS_FORMAT);
			m_sws = sws_getContext(m_codec->width, m_codec->height, m_codec->pix_fmt, m_codec->width, m_codec->height, ICE_SWS_FORMAT, ICE_SWS_SCALE, NULL, NULL, NULL);
		}
		
		m_aPositionHandle = m_program->getAttribLocation("aPosition");
		m_aTexCoordHandle = m_program->getAttribLocation("aTexCoord");
		m_uColorMatrixHandle = m_program->getUniformLocation("uColorMatrix");
		m_uTextureHandle[0] = m_program->getUniformLocation("sTexture0");
		m_uTextureHandle[1] = m_program->getUniformLocation("sTexture1");
		m_uTextureHandle[2] = m_program->getUniformLocation("sTexture2");
		m_uTextureHandle[3] = m_program->getUniformLocation("sTexture3");

		for(int i = 0; i < textureCount; ++i) {
			GLTexture *texture = new GLTexture();
			texture->create();
			m_textures.push_back(texture);
		}
		
		MakeColorMatrix(m_colorMatrix, 0, 0, 0, 0, true);
		LOGI("Success!");
	}
	
	void _createFrame(PixelFormat format) {
		m_frame = avcodec_alloc_frame();		
		size_t size = avpicture_get_size(format, m_codec->width, m_codec->height);
		m_frameBuffer = (uint8_t*)av_malloc(size);
		m_frameFormat = format;
		avpicture_fill((AVPicture*)m_frame, m_frameBuffer, format, m_codec->width, m_codec->height);
	}
	
	void _deleteFrame() {
		if(m_frameBuffer != NULL) {
			av_free(m_frameBuffer);
			m_frameBuffer = NULL;
		}
		if(m_frame != NULL) {
			av_free(m_frame);
			m_frame = NULL;
		}
		m_frameFormat = PIX_FMT_NONE;
	}
	
	void _createRGBAYUVMask(AVFrame *frame) {
		int w = frame->linesize[0];
		int h = m_codec->height;
		int maskSize = 4 * w * h;
		uint8_t *mask = (uint8_t*)malloc(maskSize);
		for(int i = 0; i < maskSize;) {
			// first pixel should only read from the R component (YUV = tex0.R, tex1.R, tex2.R)
			mask[i++] = 255;
			mask[i++] = 0;
			mask[i++] = 0;
			mask[i++] = 0;
			// 2nd from Green (YUV = tex1..3.G)
			mask[i++] = 0;
			mask[i++] = 255;
			mask[i++] = 0;
			mask[i++] = 0;
			// 3rd from Blue (YUV = texN.B
			mask[i++] = 0;
			mask[i++] = 0;
			mask[i++] = 255;
			mask[i++] = 0;
			// 4th from Alpha - texN.A
			mask[i++] = 0;
			mask[i++] = 0;
			mask[i++] = 0;
			mask[i++] = 255;
		}
		if(m_yuvRgbaMap == NULL) {
			m_yuvRgbaMap = new GLTexture();
			m_yuvRgbaMap->create();
		}
		GLTexture::bind(m_yuvRgbaMap, 3);
		m_yuvRgbaMap->upload(w, h, GL_RGBA, GL_UNSIGNED_BYTE, mask);
		free(mask);
	}

    // Config
    RenderMode m_renderMode;
	
	// Temp frame, used for SW scaling
	AVFrame *m_frame;
	PixelFormat m_frameFormat;
	uint8_t *m_frameBuffer;
	
	// GL
	GLProgram *m_program;
	std::vector<GLTexture*> m_textures;
	Matrix4 m_colorMatrix;
	GLuint m_uTextureHandle[4];
	GLuint m_aPositionHandle;
	GLuint m_aTexCoordHandle;
	GLuint m_uColorMatrixHandle; 
	
	// VB
	int m_vertexType;
	int m_numIndices;
	float m_vertices[16];
	float m_texCoords[16];
	unsigned short m_indices[16];
	
	// Stuff
	AVCodecContext *m_codec;
	SwsContext *m_sws;
	bool m_dirty;
	GLTexture *m_yuvRgbaMap;
};
