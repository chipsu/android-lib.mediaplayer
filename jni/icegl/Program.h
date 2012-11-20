// Copyright (c) 2012 - Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
#pragma once

#include "Main.h"
#include "Shader.h"

namespace icegl {

struct Program {
    static Program *s_active;
    static Program *s_default;
    
    static void bind(Program *program) {
        ICE_PROFILE(NULL);
        if(program == NULL) {
            if(s_default == NULL) {
                s_default = new Program(g_shader_vert, g_shader_frag);
            }
            program = s_default;
        }
        if(s_active != program) {
            if(s_active != NULL) {
                s_active->unbind();
            }
            program->bind();
            s_active = program;
        }
    }
    
    Program() : m_handle(0) {}
    
    Program(const char *vertexSource, const char *fragmentSource) : m_handle(0) {
        attach(new Shader(vertexSource, GL_VERTEX_SHADER));
        attach(new Shader(fragmentSource, GL_FRAGMENT_SHADER));
        link();
    }
    
    ~Program() {
        clear();
    }
    
    bool isValid() const {
        return m_handle != 0;
    }
    
    GLuint getHandle() const {
        return m_handle;
    }
    
    bool bind() {
        if(!isValid()) {
            LOGE("Program is not valid!");
            return false;
        }
        glUseProgram(m_handle);
        checkGlError();
        return true;
    }
    
    void unbind() {
    }
    
    bool attach(Shader *shader) {
        if(shader == NULL) {
            LOGE("NULL pointer!");
            return false;
        }
        if(!shader->isValid()) {
            LOGE("Shader is not valid!");
            return false;
        }
        m_shaders.push_back(shader);
        return true;
    }
    
    bool attach(const char *source, int type) {
        Shader *shader = new Shader(source, type);
        if(!attach(shader)) {
            delete shader;
            return false;
        }
        return true;
    }
    
    bool vert(const char *source) {
        return attach(source, GL_VERTEX_SHADER);
    }
    
    bool frag(const char *source) {
        return attach(source, GL_FRAGMENT_SHADER);
    }
    
    bool link() {
        GLint status = GL_FALSE;
            
        if(isValid()) {
            LOGE("GLProgram is already linked!");
            return false;
        }
        if(m_shaders.size() < 1) {
            LOGE("GLProgram has no shaders attached!");
            return false;
        }
        m_handle = glCreateProgram();
        if(m_handle == 0) {
            LOGE("glCreateProgram failed!");
            return false;
        }
        
        for(std::vector<Shader*>::iterator it = m_shaders.begin(); it != m_shaders.end(); ++it) {
            LOGI("glAttachShader, shader type = %s", (*it)->getTypeStr());
            glAttachShader(m_handle, (*it)->getHandle());
            checkGlError();
        }
        
        glLinkProgram(m_handle);
        glGetProgramiv(m_handle, GL_LINK_STATUS, &status);
        checkGlError();
        
        if(status == GL_FALSE) {
            LOGE("glLinkProgram failed, %d shaders attached", m_shaders.size());
            GLint bufferSize = 0;
            glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &bufferSize);
            if(bufferSize) {
                char *buffer = (char*)malloc(bufferSize);
                if(buffer) {
                    glGetProgramInfoLog(m_handle, bufferSize, NULL, buffer);
                    LOGE("glLinkProgram error: %s", buffer);
                    free(buffer);
                }
            }
            glDeleteProgram(m_handle);
            m_handle = 0;
            return false;
        }
        return true;
    }
    
    void clear() {
        for(std::vector<Shader*>::iterator it = m_shaders.begin(); it != m_shaders.end(); ++it) {
            delete *it;
        }
        m_shaders.clear();
        if(m_handle) {
            glDeleteProgram(m_handle);
            m_handle = 0;
        }
    }
    
    GLuint getAttribLocation(const char *name) const {
        GLuint handle = glGetAttribLocation(m_handle, name);
        if(handle == -1) {
            LOGW("glGetAttribLocation(%d, %s) failed!", m_handle, name);
        }
        return handle;
    }
    
    GLuint getUniformLocation(const char *name) const {
        GLuint handle = glGetUniformLocation(m_handle, name);
        if(handle == -1) {
            LOGW("glGetUniformLocation(%d, %s) failed!", m_handle, name);
        }
        return handle;
    }
    
    GLuint m_handle;
    std::vector<Shader*> m_shaders;
};

}
