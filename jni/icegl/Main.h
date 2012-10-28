// Copyright (c) 2012 - Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
#pragma once

#include <icecore/Main.h>
#include <icecore/Log.h>
#include <icecore/Profiler.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#ifndef MAX_TEXTURE_UNITS
	#define MAX_TEXTURE_UNITS 8
#endif

#if ICE_DEBUG
	#define checkGlError() do { \
		for (GLint error = glGetError(); error; error = glGetError()) { \
			LOGE("glError (0x%x)\n", error); \
		} \
	} while(0)
#else
	#define checkGlError() do {} while(0)
#endif

#ifndef ICE_PROFILE
	#define ICE_PROFILE(name) do {} while(0)
#endif

namespace icegl {

struct Shader;
struct Program;

static const char g_shader_vert[] = 
	"precision mediump float;\n"
	"attribute vec4 aPosition;\n"
	"attribute vec2 aTexCoord;\n"
	"varying vec2 textureCoord;\n"
	"void main() {\n"
	"	textureCoord = aTexCoord;\n"
	"	gl_Position = aPosition;\n"
	"}\n";
	
static const char g_shader_frag[]= 
	"precision mediump float;\n"
	"uniform sampler2D sTexture0;\n"
	"varying vec2 textureCoord;\n"
	"void main() {\n"
	"  gl_FragColor = texture2D(sTexture0, textureCoord);\n"
	"}\n";

	
static const char g_shader_rgb_frag[] = 
	"precision mediump float;\n"
	"uniform sampler2D sTexture0;\n"
	"varying vec2 textureCoord;\n"
	"void main() {\n"
	"  gl_FragColor = texture2D(sTexture0, textureCoord);\n"
	"}\n";

/**
 * YUV shader.
 */
static const char g_shader_yuv_frag[] = 
	"precision mediump float;\n"
	"uniform sampler2D sTexture0;\n"
	"uniform sampler2D sTexture1;\n"
	"uniform sampler2D sTexture2;\n"
	"uniform mat4 uColorMatrix;\n"
	"varying vec2 textureCoord;\n"
	"void main() {\n"
	"	vec4 color = vec4(\n"
	"		texture2D(sTexture0, textureCoord).r,\n"
	"		texture2D(sTexture1, textureCoord).r,\n"
	"		texture2D(sTexture2, textureCoord).r,\n"
	"		1.0\n"
	"	);\n"
	"	gl_FragColor = uColorMatrix * color;\n"
	"}\n";

/**
 * YUV in RGBA shader.
 * Many Android SOC's use GL_RGBA internally so glTexSubImage2D with anything else (like GL_LUMINANCE) can be VERY slow.
 * This hack uses a 4th lookup texture to get the YUV data from the RGBA textures.
 * @todo Artifacts... (vertical lines) (red only?)
 *    i dont know...
 *        first lookup (from tex0) is ok, do we need a smaller lookup table for UV?
 */
static const char g_shader_rgba_yuv_frag[] = 
	"precision mediump float;\n"
	"uniform sampler2D sTexture0;\n"
	"uniform sampler2D sTexture1;\n"
	"uniform sampler2D sTexture2;\n"
	"uniform sampler2D sTexture3;\n"
	"uniform mat4 uColorMatrix;\n"
	"varying vec2 textureCoord;\n"
	"void main() {\n"
	"	vec4 mask = texture2D(sTexture3, textureCoord);\n"
	"	vec4 color = vec4(\n"
	"		length(texture2D(sTexture0, textureCoord) * mask),\n" // Looks fine
	"		length(texture2D(sTexture1, textureCoord) * mask),\n" // !!!
	"		length(texture2D(sTexture2, textureCoord) * mask),\n" // !!!
	"		1.0\n"
	"	);\n"
	"	gl_FragColor = vec4((uColorMatrix * color).xyz, 1.0);\n"
	//"   gl_FragColor = texture2D(sTexture0, textureCoord);\n"
	"}\n";

}
