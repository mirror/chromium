// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_shell_apk;

import static org.chromium.content_shell_apk.CardboardUtil.makeFloatBuffer;
import static org.chromium.content_shell_apk.CardboardUtil.makeRectangularTextureBuffer;

import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.SurfaceTexture;
import android.opengl.GLES20;
import android.util.Log;


import static org.chromium.ui.base.WindowAndroid.setTextureHandle;
import static org.chromium.ui.base.WindowAndroid.getTextureHandle;

import java.nio.FloatBuffer;
import java.io.IOException;
import java.lang.Thread;

/**
 * Chromoting Cardboard activity desktop, which is used to display host desktop.
 */
public class CardboardBrowser implements SurfaceTexture.OnFrameAvailableListener {
    private static final String VERTEX_SHADER =
            "uniform mat4 u_CombinedMatrix;"
            + "attribute vec4 a_Position;"
            + "attribute vec2 a_TexCoordinate;"
            + "varying vec2 v_TexCoordinate;"
            + "attribute float a_transparency;"
            + "varying float v_transparency;"
            + "void main() {"
            + "  v_transparency = a_transparency;"
            + "  v_TexCoordinate = a_TexCoordinate;"
            + "  gl_Position = u_CombinedMatrix * a_Position;"
            + "}";

    private static final String FRAGMENT_SHADER =
            "#extension GL_OES_EGL_image_external : require\n"
            + "precision highp float;"
            + "uniform samplerExternalOES u_Texture;"
            + "varying vec2 v_TexCoordinate;"
            + "const float borderWidth = 0.002;"
            + "varying float v_transparency;"
            + "void main() {"
            + "  if (v_TexCoordinate.x > (1.0 - borderWidth) || v_TexCoordinate.x < borderWidth"
            + "      || v_TexCoordinate.y > (1.0 - borderWidth)"
            + "      || v_TexCoordinate.y < borderWidth) {"
            + "    gl_FragColor = vec4(1.0, 1.0, 1.0, v_transparency);"
            + "  } else {"
            + "    vec4 texture = texture2D(u_Texture, v_TexCoordinate);"
            + "    gl_FragColor = vec4(texture.r, texture.g, texture.b, v_transparency);"
            + "  }"
            + "}";

    private static final FloatBuffer TEXTURE_COORDINATES = makeRectangularTextureBuffer(
            0.0f, 1.0f, 0.0f, 1.0f);

    private static final int POSITION_DATA_SIZE = 3;
    private static final int TEXTURE_COORDINATE_DATA_SIZE = 2;

    // Fix the desktop height and adjust width accordingly.
    private static final float HALF_HEIGHT = 1.0f;

    // Number of vertices passed to glDrawArrays().
    private static final int VERTICES_NUMBER = 6;

    private int mVertexShaderHandle;
    private int mFragmentShaderHandle;
    private int mProgramHandle;
    private int mCombinedMatrixHandle;
    private int mTextureUniformHandle;
    private int mPositionHandle;
    private int mTransparentHandle;
    private int mTextureDataHandle;
    private SurfaceTexture mTexture;

    private int mTextureCoordinateHandle;
    private FloatBuffer mPosition;
    private float mHalfWidth;

    // Lock to allow multithreaded access to mHalfWidth.
    private final Object mHalfWidthLock = new Object();

    private Bitmap mVideoFrame;

    // Lock to allow multithreaded access to mVideoFrame.
    private final Object mVideoFrameLock = new Object();

    // Flag to indicate whether to reload the desktop texture.
    private boolean mReloadTexture;

    // Lock to allow multithreaded access to mReloadTexture.
    private final Object mReloadTextureLock = new Object();
    private ContentCardboardRenderer.MainHandler mHandler;

    public CardboardBrowser(ContentCardboardRenderer.MainHandler h) {
        long id = Thread.currentThread().getId();
        Log.d("bshe:log", "thread id when create texture: " + id);
        mVertexShaderHandle =
                ShaderHelper.compileShader(GLES20.GL_VERTEX_SHADER, VERTEX_SHADER);
        mFragmentShaderHandle =
                ShaderHelper.compileShader(GLES20.GL_FRAGMENT_SHADER, FRAGMENT_SHADER);
        mProgramHandle = ShaderHelper.createAndLinkProgram(mVertexShaderHandle,
                mFragmentShaderHandle, new String[] {"a_Position", "a_TexCoordinate",
                    "u_CombinedMatrix", "u_Texture"});
        mCombinedMatrixHandle =
                GLES20.glGetUniformLocation(mProgramHandle, "u_CombinedMatrix");
        mTextureUniformHandle = GLES20.glGetUniformLocation(mProgramHandle, "u_Texture");
        mPositionHandle = GLES20.glGetAttribLocation(mProgramHandle, "a_Position");
        mTransparentHandle = GLES20.glGetAttribLocation(mProgramHandle, "a_transparency");
        mTextureCoordinateHandle = GLES20.glGetAttribLocation(mProgramHandle, "a_TexCoordinate");
        mTextureDataHandle = TextureHelper.createTextureHandle();
        mTexture = new SurfaceTexture(mTextureDataHandle);
        mTexture.setOnFrameAvailableListener(this);
        setTextureHandle(mTexture);

        mHandler = h;

        mPosition = makeFloatBuffer(new float[] {
                -1.0f, 1.0f, 0.0f,
                -1.0f, -HALF_HEIGHT, 0.0f,
                1.0f, HALF_HEIGHT, 0.0f,
                -1.0f, -HALF_HEIGHT, 0.0f,
                1.0f, -HALF_HEIGHT, 0.0f,
                1.0f, HALF_HEIGHT, 0.0f
        });
    }

    @Override   // SurfaceTexture.OnFrameAvailableListener; runs on arbitrary thread
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        long id = Thread.currentThread().getId();
        Log.d("bshe:log", "thread id when frame availabe: " + id);

        Log.d("bshe:log", "frame available in browser");
        //mHandler.sendEmptyMessage(ContentCardboardRenderer.MainHandler.MSG_FRAME_AVAILABLE);
    }

    /**
     * Draw the browser. Make sure {@link hasVideoFrame} returns true.
     */
    public void draw(float[] combinedMatrix, boolean isTransparent) {
        //long id = Thread.currentThread().getId();
        //Log.d("bshe:log", "thread id when draw" + id);

//        if (getTextureHandle() != null)
//            getTextureHandle().attachToGLContext(mTextureDataHandle);
        GLES20.glUseProgram(mProgramHandle);

        // Pass in model view project matrix.
        GLES20.glUniformMatrix4fv(mCombinedMatrixHandle, 1, false, combinedMatrix, 0);

        // Pass in texture coordinate.
        GLES20.glVertexAttribPointer(mTextureCoordinateHandle, TEXTURE_COORDINATE_DATA_SIZE,
                GLES20.GL_FLOAT, false, 0, TEXTURE_COORDINATES);
        GLES20.glEnableVertexAttribArray(mTextureCoordinateHandle);

        GLES20.glVertexAttribPointer(mPositionHandle, POSITION_DATA_SIZE, GLES20.GL_FLOAT, false,
                0, mPosition);
        GLES20.glEnableVertexAttribArray(mPositionHandle);

        // Enable transparency.
        GLES20.glEnable(GLES20.GL_BLEND);
        GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);

        // Pass in transparency.
        GLES20.glVertexAttrib1f(mTransparentHandle, isTransparent ? 0.5f : 1.0f);

        mTexture.updateTexImage();
        // Link texture data with texture unit.
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureDataHandle);
        GLES20.glUniform1i(mTextureUniformHandle, 0);

        GLES20.glDrawArrays(GLES20.GL_TRIANGLES, 0, VERTICES_NUMBER);

        GLES20.glDisable(GLES20.GL_BLEND);
        GLES20.glDisableVertexAttribArray(mPositionHandle);
        GLES20.glDisableVertexAttribArray(mTextureCoordinateHandle);

//        if (getTextureHandle() != null)
//            getTextureHandle().detachFromGLContext();
    }

    /**
     *  Update the desktop frame data based on the mVideoFrame. Note here we fix the
     *  height of the desktop and vary width accordingly.
     */
//    private void updateVideoFrame(Bitmap videoFrame) {
//        float newHalfDesktopWidth;
//        synchronized (mVideoFrameLock) {
//            mVideoFrame = videoFrame;
//            TextureHelper.linkTexture(mTextureDataHandle, videoFrame);
//            newHalfDesktopWidth = videoFrame.getWidth() * HALF_HEIGHT / videoFrame.getHeight();
//        }
//
//        synchronized (mHalfWidthLock) {
//            if (Math.abs(mHalfWidth - newHalfDesktopWidth) > 0.0001) {
//                mHalfWidth = newHalfDesktopWidth;
//                mPosition = makeFloatBuffer(new float[] {
//                        // Desktop model coordinates.
//                        -mHalfWidth, HALF_HEIGHT, 0.0f,
//                        -mHalfWidth, -HALF_HEIGHT, 0.0f,
//                        mHalfWidth, HALF_HEIGHT, 0.0f,
//                        -mHalfWidth, -HALF_HEIGHT, 0.0f,
//                        mHalfWidth, -HALF_HEIGHT, 0.0f,
//                        mHalfWidth, HALF_HEIGHT, 0.0f
//                });
//            }
//        }
//    }
//
    /**
     * Clean up opengl data.
     */
    public void cleanup() {
        GLES20.glDeleteShader(mVertexShaderHandle);
        GLES20.glDeleteShader(mFragmentShaderHandle);
        GLES20.glDeleteTextures(1, new int[] {mTextureDataHandle}, 0);
    }

    /**
     * Get desktop height and width in pixels.
     */
    public Point getFrameSizePixels() {
        synchronized (mVideoFrameLock) {
            return new Point(mVideoFrame == null ? 0 : mVideoFrame.getWidth(),
                    mVideoFrame == null ? 0 : mVideoFrame.getHeight());
        }
    }

    /**
     * Link desktop texture if {@link reloadTexture} was previously called.
     * Invoked from {@link com.google.vrtoolkit.cardboard.CardboardView.StereoRenderer.onNewFrame}
     * so that both eyes will have the same texture.
     */
    public void maybeLoadDesktopTexture() {
        synchronized (mReloadTextureLock) {
            if (!mReloadTexture) {
                return;
            }
        }

        //updateVideoFrame(bitmap);

        synchronized (mReloadTextureLock) {
            mReloadTexture = false;
        }
    }

    /**
     * Inform this object that a new video frame should be rendered.
     * Called from native display thread.
     */
    public void reloadTexture() {
        synchronized (mReloadTextureLock) {
            mReloadTexture = true;
        }
    }
}
