// Copyright (c) 2012 - Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
#pragma once

#include "Main.h"

namespace icegl {

struct Shader {
	
	Shader() : m_handle(0), m_type(0) {}
	
	Shader(const char *source, int type) : m_handle(0), m_type(0) {
		load(source, type);
	}
	
	~Shader() {
		unload();
	}
	
	bool isValid() const {
		return m_handle != 0;
	}
	
	GLuint getType() const {	
		return m_type;
	}
	
	GLuint getHandle() const {
		return m_handle;
	}
	
	bool load(const char *source, int type) {
		GLint status = GL_FALSE;
		
		if(isValid()) {
			LOGE("Shader is already loaded!");
			return false;
		}
		m_handle = glCreateShader(type);
		if(m_handle == 0) {
			LOGE("glCreateShader(%s) failed!", getTypeStr(type));
			return false;
		}
		glShaderSource(m_handle, 1, &source, NULL);
		glCompileShader(m_handle);
		checkGlError();
		
		glGetShaderiv(m_handle, GL_COMPILE_STATUS, &status);
		if(status == GL_FALSE) {
			GLint bufferSize = 0;
			glGetShaderiv(m_handle, GL_INFO_LOG_LENGTH, &bufferSize);
			LOGE("Could not compile shader %d: %s!", getTypeStr(type));
			if(bufferSize) {
				char *buffer = (char*)malloc(bufferSize);
				if(buffer) {
					glGetShaderInfoLog(m_handle, bufferSize, NULL, buffer);
					LOGE("Shader log:\n");
					LOGE(buffer);
					free(buffer);
				}
			}
			LOGE("Shader source:\n");
			LOGE(source);
			unload();
			return false;
		}
		m_type = type;
		return true;
	}
	
	void unload() {
		if(m_handle != 0) {
			glDeleteShader(m_handle);
			m_handle = 0;
			m_type = 0;
		}
	}
	
	const char *getTypeStr() const {
		return getTypeStr(m_type);
	}
	
	static const char *getTypeStr(int type) {
		if(type == GL_VERTEX_SHADER) {
			return "GL_VERTEX_SHADER";
		} else if(type == GL_FRAGMENT_SHADER) {
			return "GL_FRAGMENT_SHADER";
		}
		return "UNKNOWN";
	}
	
	GLuint m_handle;
	GLuint m_type;
};

}
