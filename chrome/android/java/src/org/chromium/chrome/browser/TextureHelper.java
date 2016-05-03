// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.opengl.GLES11Ext;
import static android.opengl.GLES20.GL_LINEAR;
import static android.opengl.GLES20.GL_NEAREST;
import static android.opengl.GLES20.GL_TEXTURE_2D;
import static android.opengl.GLES20.GL_TEXTURE_CUBE_MAP;
import static android.opengl.GLES20.GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
import static android.opengl.GLES20.GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
import static android.opengl.GLES20.GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
import static android.opengl.GLES20.GL_TEXTURE_CUBE_MAP_POSITIVE_X;
import static android.opengl.GLES20.GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
import static android.opengl.GLES20.GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
import static android.opengl.GLES20.GL_TEXTURE_MAG_FILTER;
import static android.opengl.GLES20.GL_TEXTURE_MIN_FILTER;
import static android.opengl.GLES20.glBindTexture;
import static android.opengl.GLES20.glDeleteTextures;
import static android.opengl.GLES20.glGenTextures;
import static android.opengl.GLES20.glTexParameteri;
import static android.opengl.GLUtils.texImage2D;

import android.graphics.Bitmap;

/**
 * Helper class for working with OpenGL textures.
 */
public class TextureHelper {
    /**
     * Create new texture handle.
     * @return New texture handle.
     */
    public static int createTextureHandle() {
        int[] textureDataHandle = new int[1];
        glGenTextures(1, textureDataHandle, 0);

        if (textureDataHandle[0] != 0) {
            // Bind to the texture in OpenGL.
            glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textureDataHandle[0]);

            glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            return textureDataHandle[0];
        } else {
            throw new RuntimeException("Error generating texture handle.");
        }
    }

    /**
     * Link desktop texture with a handle.
     * @param textureDataHandle the handle to attach texture to
     */
    public static void linkTexture(int textureDataHandle, Bitmap bitmap) {
        // Delete previously attached texture.
        glDeleteTextures(1, new int[] {textureDataHandle}, 0);

        // Bind to the texture in OpenGL.
        glBindTexture(GL_TEXTURE_2D, textureDataHandle);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Load the bitmap into the bound texture.
        texImage2D(GL_TEXTURE_2D, 0, bitmap, 0);
    }

}
