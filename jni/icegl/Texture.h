// Copyright (c) 2012 - Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
#pragma once

#include "Main.h"

namespace icegl {

struct Texture {
    
    static Texture *s_active[MAX_TEXTURE_UNITS];
    
    static void bindAny(Texture *texture) {
        ICE_PROFILE(NULL);
        for(int i = 0; i < MAX_TEXTURE_UNITS; ++i) {
            if(s_active[i] == texture) {
                glActiveTexture(GL_TEXTURE0 + i);
                return;
            }
        }
        bind(texture, MAX_TEXTURE_UNITS - 1);
    }
    
    static void bind(Texture *texture, int index) {
        ICE_PROFILE(NULL);
        if(index < 0 || index > MAX_TEXTURE_UNITS) {
            LOGE("Texture index out of range!");
            return;
        }
        if(texture != s_active[index]) {
            glActiveTexture(GL_TEXTURE0 + index);
            if(s_active[index] != NULL) {
                s_active[index]->unbind();
            }
            if(texture != NULL) {
                texture->bind();
            }
            s_active[index] = texture;
        }
    }
    
    static void bind(std::vector<Texture*> textures) {
        for(int i = 0; i < textures.size(); ++i) {
            bind(textures[i], i);
        }
        unbindFrom(textures.size() + 1);
    }
    
    static void unbind(Texture *texture) {
        for(int i = 0; i < MAX_TEXTURE_UNITS; ++i) {
            if(s_active[i] == texture) {
                bind(NULL, i);
            }
        }
    }
    
    static void unbind(int unit, int count) {
        for(int i = unit; i < count; ++i) {
            bind(NULL, i);
        }
    }
    
    static void unbindFrom(int unit) {
        unbind(unit, MAX_TEXTURE_UNITS - unit);
    }
    
    static void unbindAll() {
        unbindFrom(0);
    }
    
    Texture() : m_handle(0), m_target(0), m_width(0), m_height(0), m_internalFormat(0), m_pixelType(0) {
    }
    
    ~Texture() {
        unload();
    }
    
    GLuint isValid() const {
        return m_handle != 0;
    }
    
    GLuint getHandle() const {
        return m_handle;
    }
    
    int getWidth() const {
        return m_width;
    }
    
    int getHeight() const {
        return m_height;
    }
    
    int getTarget() const {
        return m_target;
    }
    
    void bind() {
        glBindTexture(m_target, m_handle);
        checkGlError();
    }
    
    void unbind() {
        glBindTexture(m_target, 0);
        checkGlError();
    }
    
    bool create(int width = 0, int height = 0, int target = GL_TEXTURE_2D) {
        ICE_PROFILE(NULL);
        if(isValid()) {
            LOGE("Texture is already initialized!");
            return false;
        }
        m_width = width;
        m_height = height;
        m_target = target;
        glGenTextures(1, &m_handle);
        bindAny(this);
        if(target == GL_TEXTURE_2D) {
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        checkGlError();
        return true;
    }
    
    /*
eglGetCurrentContext()
EGLAPI EGLImageKHR EGLAPIENTRY eglCreateImageKHR (EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
EGLAPI EGLBoolean EGLAPIENTRY eglDestroyImageKHR (EGLDisplay dpy, EGLImageKHR image);
GL_APICALL void GL_APIENTRY glEGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image);
GL_APICALL void GL_APIENTRY glEGLImageTargetRenderbufferStorageOES (GLenum target, GLeglImageOES image);
     */
    // TODO: Move glTexImage => create() and only use TexSubImage here.
    bool upload(int width, int height, int format, int type, const void *data) {
        ICE_PROFILE(NULL);
        if(!isValid()) {
            LOGE("Texture has not been initialized!");
            return false;
        }
        int level = 0;
        int internalFormat = format;
        bindAny(this);
        if(m_width != width
        || m_height != height
        || m_format != format
        || m_internalFormat != internalFormat
        || m_pixelType != type) {
            // FIXME: This is terribly slow... 
            glTexImage2D(m_target, level, internalFormat, width, height, 0, format, type, data);
            checkGlError();
            m_width = width;
            m_height = height;
            m_format = format;
            m_internalFormat = internalFormat;
            m_pixelType = type;
        } else {
            glTexSubImage2D(m_target, level, 0, 0, width, height, format, type, data);
            checkGlError();
        }
        return true;
    }
    
    bool uploadPart(int x, int y, int width, int height, int format, int type, const void *data) {
        ICE_PROFILE(NULL);
        if(!isValid()) {
            LOGE("Texture has not been initialized!");
            return false;
        }
        int level = 0;
        glTexSubImage2D(m_target, level, x, y, width, height, format, type, data);
        checkGlError();
        return true;
    }
    
    bool load(...) {
        if(isValid()) {
            LOGE("Texture is already initialized!");
            return false;
        }
        LOGE("Not implemented!");
        return false;
        /*create(bitmap->width, bitmap->height, target);
        bindAny(this);
                
        switch(type) {
        case GL_TEXTURE_2D: {
                Bitmap bitmap = bitmaps[0];
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmap, 0);
                Helper.glThrowError();
            }
            break;
        case GL_TEXTURE_CUBE_MAP: {
                glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                for(int i = 0; i < 6; ++i) {
                    Bitmap bitmap = bitmaps[i];
                    GLUtils.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, bitmap, 0);
                    Helper.glThrowError();
                }
            }
            break;
        default:
            assert(false);
        }
        */
        return true;
    }
    
    void unload() {
        if(m_handle != 0) {
            unbind(this);
            glDeleteTextures(1, &m_handle);
            m_handle = 0;
        }
    }
    
    GLuint m_handle;
    int m_target;
    int m_width;
    int m_height;
    int m_format;
    int m_internalFormat;
    int m_pixelType;
};

}
