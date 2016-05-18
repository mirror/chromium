// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.SurfaceTexture;
import android.opengl.GLES20;
import android.util.Log;

import static org.chromium.chrome.browser.CardboardUtil.makeFloatBuffer;
import static org.chromium.chrome.browser.CardboardUtil.makeRectangularTextureBuffer;
import static org.chromium.ui.base.WindowAndroid.setSurfaceTexture;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

/**
 * Chromoting Cardboard activity desktop, which is used to display host desktop.
 */
public class CardboardBrowser implements SurfaceTexture.OnFrameAvailableListener {
    private static final String VERTEX_SHADER = "uniform mat4 u_CombinedMatrix;"
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

    private static final String FRAGMENT_SHADER = "#extension GL_OES_EGL_image_external : require\n"
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

    // Cursor
    private final int mBytesPerFloat = 4;
    private int programHandle;
    /** Allocate storage for the final combined matrix. This will be passed into the shader program. */
    private float[] mMVPMatrix = new float[16];

    /** How many elements per vertex. */
    private final int mStrideBytes = 7 * mBytesPerFloat;

    /** Offset of the position data. */
    private final int mPositionOffset = 0;

    /** Size of the position data in elements. */
    private final int mPositionDataSize = 3;

    /** Offset of the color data. */
    private final int mColorOffset = 3;

    /** Size of the color data in elements. */
    private final int mColorDataSize = 4;
    private int mTriangleMVPMatrixHandle;

    /** This will be used to pass in model position information. */
    private int mTrianglePositionHandle;

    /** This will be used to pass in model color information. */
    private int mTriangleColorHandle;
    final String vertexShader =
        "uniform mat4 u_MVPMatrix;      \n"  // A constant representing the combined model/view/projection matrix
      + "attribute vec4 a_Position;     \n"     // Per-vertex position information we will pass in.
      + "attribute vec4 a_Color;        \n"     // Per-vertex color information we will pass in.

      + "varying vec4 v_Color;          \n"     // This will be passed into the fragment shader.

      + "void main()                    \n"     // The entry point for our vertex shader.
      + "{                              \n"
      + "   v_Color = a_Color;          \n"     // Pass the color through to the fragment shader.
                                            // It will be interpolated across the triangle.
      + "   gl_Position = u_MVPMatrix   \n" // gl_Position is a special variable used to store the final position.
      + "               * a_Position;   \n"     // Multiply the vertex by the matrix to get the final point in
      + "}                              \n";    // normalized screen coordinates.
    final String fragmentShader =
        "precision mediump float;       \n"     // Set the default precision to medium. We don't need as high of a
                                            // precision in the fragment shader.
      + "varying vec4 v_Color;          \n"     // This is the color from the vertex shader interpolated across the
                                            // triangle per fragment.
      + "void main()                    \n"     // The entry point for our fragment shader.
      + "{                              \n"
      + "   gl_FragColor = v_Color;     \n"     // Pass the color directly through the pipeline.
      + "}                              \n";
   private final FloatBuffer mTriangle1Vertices;
   private final FloatBuffer mTriangle2Vertices;
   final float[] triangle1VerticesData = {
            // X, Y, Z,
            // R, G, B, A
            0.0f, 0.0f,  0.0f,
            0.2f, 0.2f, 0.2f, 0.4f,

            0.1f, -0.05f, 0.4f,
            0.5f, 0.5f, 0.5f, 0.4f,

            0.05f, -0.1f, 0.4f,
            0.6f, 0.6f, 0.6f, 0.4f};
   final float[] triangle2VerticesData = {
            // X, Y, Z,
            // R, G, B, A
            0.0f, 0.0f,  0.1f,
            1.0f, 0.6f, 0.3f, 0.4f,

            0.1f, -0.05f, 0.1f,
            1.0f, 0.6f, 0.4f, 0.4f,

            0.05f, -0.1f, 0.1f,
            1.0f, 0.6f, 0.5f, 0.4f};

    private static final FloatBuffer TEXTURE_COORDINATES =
            makeRectangularTextureBuffer(0.0f, 1.0f, 0.0f, 1.0f);

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
    private FloatBuffer mPosition2;
    private float mHalfWidth;

    // Lock to allow multithreaded access to mHalfWidth.
    private final Object mHalfWidthLock = new Object();

    private Bitmap mVideoFrame;

    // Lock to allow multithreaded access to mVideoFrame.
    private final Object mVideoFrameLock = new Object();

    public CardboardBrowser() {
        mVertexShaderHandle = ShaderHelper.compileShader(GLES20.GL_VERTEX_SHADER, VERTEX_SHADER);
        mFragmentShaderHandle =
                ShaderHelper.compileShader(GLES20.GL_FRAGMENT_SHADER, FRAGMENT_SHADER);
        mProgramHandle = ShaderHelper.createAndLinkProgram(mVertexShaderHandle,
                mFragmentShaderHandle,
                new String[] {"a_Position", "a_TexCoordinate", "u_CombinedMatrix", "u_Texture"});
        mCombinedMatrixHandle = GLES20.glGetUniformLocation(mProgramHandle, "u_CombinedMatrix");
        mTextureUniformHandle = GLES20.glGetUniformLocation(mProgramHandle, "u_Texture");
        mPositionHandle = GLES20.glGetAttribLocation(mProgramHandle, "a_Position");
        mTransparentHandle = GLES20.glGetAttribLocation(mProgramHandle, "a_transparency");
        mTextureCoordinateHandle = GLES20.glGetAttribLocation(mProgramHandle, "a_TexCoordinate");
        mTextureDataHandle = TextureHelper.createTextureHandle();
        mTexture = new SurfaceTexture(mTextureDataHandle);
        mTexture.setOnFrameAvailableListener(this);
        Log.d("bshe:log", "In cardboard browser");
        Log.d("bshe:log", mTexture.toString());
        setSurfaceTexture(mTexture);

        mPosition = makeFloatBuffer(new float[] {
            -1.0f, 1.0f, 0.0f,
            -1.0f, -HALF_HEIGHT, 0.0f,
            1.0f, HALF_HEIGHT, 0.0f,
            -1.0f, -HALF_HEIGHT, 0.0f,
            1.0f, -HALF_HEIGHT, 0.0f,
            1.0f, HALF_HEIGHT, 0.0f});
        // Cursor
        // Load in the vertex shader.
        int vertexShaderHandle = ShaderHelper.compileShader(GLES20.GL_VERTEX_SHADER, vertexShader);

        if (vertexShaderHandle != 0) {
            // Pass in the shader source.
            GLES20.glShaderSource(vertexShaderHandle, vertexShader);

            // Compile the shader.
            GLES20.glCompileShader(vertexShaderHandle);

            // Get the compilation status.
            final int[] compileStatus = new int[1];
            GLES20.glGetShaderiv(vertexShaderHandle, GLES20.GL_COMPILE_STATUS, compileStatus, 0);

            // If the compilation failed, delete the shader.
            if (compileStatus[0] == 0) {
                GLES20.glDeleteShader(vertexShaderHandle);
                vertexShaderHandle = 0;
            }
        }

        int fragmentShaderHandle = ShaderHelper.compileShader(GLES20.GL_FRAGMENT_SHADER, fragmentShader);
        if (fragmentShaderHandle != 0) {
            // Pass in the shader source.
            GLES20.glShaderSource(fragmentShaderHandle, fragmentShader);

            // Compile the shader.
            GLES20.glCompileShader(fragmentShaderHandle);

            // Get the compilation status.
            final int[] compileStatus = new int[1];
            GLES20.glGetShaderiv(fragmentShaderHandle, GLES20.GL_COMPILE_STATUS, compileStatus, 0);

            // If the compilation failed, delete the shader.
            if (compileStatus[0] == 0) {
                GLES20.glDeleteShader(fragmentShaderHandle);
                fragmentShaderHandle = 0;
            }
        }

        if (vertexShaderHandle == 0 || fragmentShaderHandle == 0) {
            throw new RuntimeException("Error creating vertex shader or fragment shader.");
        }
        mTriangle1Vertices = ByteBuffer.allocateDirect(triangle1VerticesData.length * mBytesPerFloat)
             .order(ByteOrder.nativeOrder()).asFloatBuffer();
        mTriangle1Vertices.put(triangle1VerticesData).position(0);
        mTriangle2Vertices = ByteBuffer.allocateDirect(triangle2VerticesData.length * mBytesPerFloat)
             .order(ByteOrder.nativeOrder()).asFloatBuffer();
        mTriangle2Vertices.put(triangle2VerticesData).position(0);
        // Create a program object and store the handle to it.
        programHandle = GLES20.glCreateProgram();
        if (programHandle != 0) {
            // Bind the vertex shader to the program.
            GLES20.glAttachShader(programHandle, vertexShaderHandle);
            // Bind the fragment shader to the program.
            GLES20.glAttachShader(programHandle, fragmentShaderHandle);
            // Bind attributes
            GLES20.glBindAttribLocation(programHandle, 0, "a_Position");
            GLES20.glBindAttribLocation(programHandle, 1, "a_Color");
            // Link the two shaders together into a program.
            GLES20.glLinkProgram(programHandle);
            // Get the link status.
            final int[] linkStatus = new int[1];
            GLES20.glGetProgramiv(programHandle, GLES20.GL_LINK_STATUS, linkStatus, 0);
            // If the link failed, delete the program.
            if (linkStatus[0] == 0) {
                GLES20.glDeleteProgram(programHandle);
                programHandle = 0;
            }
        }

        if (programHandle == 0)	{
            throw new RuntimeException("Error creating program.");
        }
        // GL make the program

        // Set program handles. These will later be used to pass in values to the program.
        mTriangleMVPMatrixHandle = GLES20.glGetUniformLocation(programHandle, "u_MVPMatrix");
        mTrianglePositionHandle = GLES20.glGetAttribLocation(programHandle, "a_Position");
        mTriangleColorHandle = GLES20.glGetAttribLocation(programHandle, "a_Color");

        // Tell OpenGL to use this program when rendering.
        GLES20.glUseProgram(programHandle);
    }

    @Override // SurfaceTexture.OnFrameAvailableListener; runs on arbitrary thread
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
    }

    /**
     * Draw the browser. Make sure {@link hasVideoFrame} returns true.
     */
    public void draw(float[] combinedMatrix, boolean isTransparent) {
        GLES20.glUseProgram(mProgramHandle);

        // Pass in model view project matrix.
        GLES20.glUniformMatrix4fv(mCombinedMatrixHandle, 1, false, combinedMatrix, 0);

        // Pass in texture coordinate.
        GLES20.glVertexAttribPointer(mTextureCoordinateHandle, TEXTURE_COORDINATE_DATA_SIZE,
                GLES20.GL_FLOAT, false, 0, TEXTURE_COORDINATES);
        GLES20.glEnableVertexAttribArray(mTextureCoordinateHandle);

        GLES20.glVertexAttribPointer(
                mPositionHandle, POSITION_DATA_SIZE, GLES20.GL_FLOAT, false, 0, mPosition);
        GLES20.glEnableVertexAttribArray(mPositionHandle);

        // Enable transparency.
        GLES20.glEnable(GLES20.GL_BLEND);
        GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);

        // Pass in transparency.
        GLES20.glVertexAttrib1f(mTransparentHandle, isTransparent ? 0.5f : 1.0f);

        // Update texture for drawing(may miss some frames).
        mTexture.updateTexImage();

        // Link texture data with texture unit.
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureDataHandle);
        GLES20.glUniform1i(mTextureUniformHandle, 0);

        GLES20.glDrawArrays(GLES20.GL_TRIANGLES, 0, VERTICES_NUMBER);

        GLES20.glDisable(GLES20.GL_BLEND);
        GLES20.glDisableVertexAttribArray(mPositionHandle);
        GLES20.glDisableVertexAttribArray(mTextureCoordinateHandle);
    }
    public void drawTriangle(float[] combinedMatrix, boolean isTransparent) {
        GLES20.glUseProgram(programHandle);
        // Pass in the position information
        mTriangle1Vertices.position(mPositionOffset);
        GLES20.glVertexAttribPointer(mTrianglePositionHandle, mPositionDataSize, GLES20.GL_FLOAT, false,
                mStrideBytes, mTriangle1Vertices);
        GLES20.glEnableVertexAttribArray(mTrianglePositionHandle);
        // Pass in the color information
        mTriangle1Vertices.position(mColorOffset);
        GLES20.glVertexAttribPointer(mTriangleColorHandle, mColorDataSize, GLES20.GL_FLOAT, false,
                mStrideBytes, mTriangle1Vertices);

        GLES20.glEnable(GLES20.GL_BLEND);
        GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);

        GLES20.glEnableVertexAttribArray(mTriangleColorHandle);

        GLES20.glUniformMatrix4fv(mTriangleMVPMatrixHandle, 1, false, combinedMatrix, 0);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLES, 0, 3);
        GLES20.glDisableVertexAttribArray(mTrianglePositionHandle);
        GLES20.glDisableVertexAttribArray(mTriangleColorHandle);
    }
    public void drawMarker(float[] combinedMatrix, boolean isTransparent) {

        GLES20.glUseProgram(programHandle);
        // Pass in the position information
        mTriangle2Vertices.position(mPositionOffset);
        GLES20.glVertexAttribPointer(mTrianglePositionHandle, mPositionDataSize, GLES20.GL_FLOAT, false,
                mStrideBytes, mTriangle2Vertices);

        GLES20.glEnableVertexAttribArray(mTrianglePositionHandle);

        // Pass in the color information
        mTriangle2Vertices.position(mColorOffset);
        GLES20.glVertexAttribPointer(mTriangleColorHandle, mColorDataSize, GLES20.GL_FLOAT, false,
                mStrideBytes, mTriangle2Vertices);

        GLES20.glEnableVertexAttribArray(mTriangleColorHandle);
 
        GLES20.glUniformMatrix4fv(mTriangleMVPMatrixHandle, 1, false, combinedMatrix, 0);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLES, 0, 3);
        GLES20.glDisableVertexAttribArray(mTrianglePositionHandle);
        GLES20.glDisableVertexAttribArray(mTriangleColorHandle);
    }

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


}
