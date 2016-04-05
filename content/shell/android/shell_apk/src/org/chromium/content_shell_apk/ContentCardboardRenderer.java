// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_shell_apk;

import android.app.Activity;
import android.graphics.Point;
import android.graphics.PointF;
import android.opengl.GLES20;
import android.opengl.Matrix;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

import com.google.vrtoolkit.cardboard.CardboardView;
import com.google.vrtoolkit.cardboard.Eye;
import com.google.vrtoolkit.cardboard.HeadTransform;
import com.google.vrtoolkit.cardboard.Viewport;

import javax.microedition.khronos.egl.EGLConfig;
import java.lang.ref.WeakReference;

import static org.chromium.ui.base.WindowAndroid.getTextureHandle;
import java.lang.Thread;

/**
 * Renderer for Cardboard view.
 */
public class ContentCardboardRenderer implements CardboardView.StereoRenderer {
    private static final String TAG = "cr.CardboardRenderer";

    private static final int BYTE_PER_FLOAT = 4;
    private static final int POSITION_DATA_SIZE = 3;
    private static final int TEXTURE_COORDINATE_DATA_SIZE = 2;
    private static final float Z_NEAR = 0.1f;
    private static final float Z_FAR = 1000.0f;

    // Desktop position is fixed in world coordinates.
    private static final float DESKTOP_POSITION_X = 0.0f;
    private static final float DESKTOP_POSITION_Y = 0.0f;
    private static final float DESKTOP_POSITION_Z = -2.0f;

    // Menu bar position is relative to the view point.
    private static final float MENU_BAR_POSITION_X = 0.0f;
    private static final float MENU_BAR_POSITION_Y = 0.7f;
    private static final float MENU_BAR_POSITION_Z = -0.9f;

    // Distance to move camera each time.
    private static final float CAMERA_MOTION_STEP = 0.5f;

    // Small number used to avoid division-overflow or other problems with
    // floating-point imprecision.
    private static final float EPSILON = 1e-5f;

    private final Activity mActivity;

    private CardboardBrowser mCardboardBrowser;
    private MenuBar mMenuBar;

    private float mCameraPosition;
    // Lock to allow multithreaded access to mCameraPosition.
    private final Object mCameraPositionLock = new Object();

    private float[] mCameraMatrix;
    private float[] mViewMatrix;
    private float[] mProjectionMatrix;

    // Make matrix member variable to avoid unnecessary initialization.
    private float[] mDesktopModelMatrix;
    private float[] mDesktopCombinedMatrix;
    private float[] mEyePointModelMatrix;
    private float[] mEyePointCombinedMatrix;

    // Direction that user is looking towards.
    private float[] mForwardVector;

    // Eye position at the menu bar distance;
    private PointF mEyeMenuBarPosition;

    private MainHandler mHandler;
    public static class MainHandler extends Handler {
        public static final int MSG_FRAME_AVAILABLE = 0;

        private WeakReference<ContentCardboardRenderer> mWeakContentCardboardRenderer;

        public MainHandler(ContentCardboardRenderer browser) {
            mWeakContentCardboardRenderer = new WeakReference<ContentCardboardRenderer>(browser);
        }

        @Override
        public void handleMessage(Message msg) {
            ContentCardboardRenderer browser = mWeakContentCardboardRenderer.get();
            if (browser == null) {
                Log.d("bshe:log", "Got message for dead browser");
                return;
            }

            switch (msg.what) {
              case MSG_FRAME_AVAILABLE: {
                    browser.uploadTexImage();
                    break;
              }
              default:
                  throw new RuntimeException("Unknown message " + msg.what);
            }
        }
    }

    public ContentCardboardRenderer(Activity activity) {
        long id = Thread.currentThread().getId();
        Log.d("bshe:log", "thread id when create cardboad renderer: " + id);

        mActivity = activity;
        mCameraPosition = 0.0f;

        mCameraMatrix = new float[16];
        mViewMatrix = new float[16];
        mProjectionMatrix = new float[16];
        mDesktopModelMatrix = new float[16];
        mDesktopCombinedMatrix = new float[16];
        mEyePointModelMatrix = new float[16];
        mEyePointCombinedMatrix = new float[16];

        mForwardVector = new float[3];

        mHandler = new MainHandler(this);
    }

    public void uploadTexImage() {
        getTextureHandle().updateTexImage();
    }

    @Override
    public void onSurfaceCreated(EGLConfig config) {
      // on Render thread
        // Set the background clear color to black.
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        // Use culling to remove back faces.
        GLES20.glEnable(GLES20.GL_CULL_FACE);

        // Enable depth testing.
        GLES20.glEnable(GLES20.GL_DEPTH_TEST);

        mCardboardBrowser = new CardboardBrowser(mHandler);
        mMenuBar = new MenuBar(mActivity);

//      initializeRedrawCallback();
    }

    @Override
    public void onSurfaceChanged(int width, int height) {
    }

    @Override
    public void onNewFrame(HeadTransform headTransform) {
        // Position the eye at the origin.
        float eyeX = 0.0f;
        float eyeY = 0.0f;
        float eyeZ;
        synchronized (mCameraPositionLock) {
            eyeZ = mCameraPosition;
        }

        // We are looking toward the negative Z direction.
        float lookX = DESKTOP_POSITION_X;
        float lookY = DESKTOP_POSITION_Y;
        float lookZ = DESKTOP_POSITION_Z;

        // Set our up vector. This is where our head would be pointing were we holding the camera.
        float upX = 0.0f;
        float upY = 1.0f;
        float upZ = 0.0f;

        Matrix.setLookAtM(mCameraMatrix, 0, eyeX, eyeY, eyeZ, lookX, lookY, lookZ, upX, upY, upZ);

        headTransform.getForwardVector(mForwardVector, 0);
        mEyeMenuBarPosition = getLookingPosition(Math.abs(MENU_BAR_POSITION_Z));

        mCardboardBrowser.maybeLoadDesktopTexture();
    }

    @Override
    public void onDrawEye(Eye eye) {
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);

        // Apply the eye transformation to the camera.
        Matrix.multiplyMM(mViewMatrix, 0, eye.getEyeView(), 0, mCameraMatrix, 0);

        mProjectionMatrix = eye.getPerspective(Z_NEAR, Z_FAR);

        drawDesktop();
        drawMenuBar();
    }

    @Override
    public void onRendererShutdown() {
        mCardboardBrowser.cleanup();
        mMenuBar.cleanup();
    }

    @Override
    public void onFinishFrame(Viewport viewport) {
    }

    private void drawDesktop() {
        Matrix.setIdentityM(mDesktopModelMatrix, 0);
        Matrix.translateM(mDesktopModelMatrix, 0, DESKTOP_POSITION_X,
                DESKTOP_POSITION_Y, DESKTOP_POSITION_Z);

        // Pass in Model View Matrix and Model View Project Matrix.
        Matrix.multiplyMM(mDesktopCombinedMatrix, 0, mViewMatrix, 0, mDesktopModelMatrix, 0);
        Matrix.multiplyMM(mDesktopCombinedMatrix, 0, mProjectionMatrix,
                0, mDesktopCombinedMatrix, 0);

        mCardboardBrowser.draw(mDesktopCombinedMatrix, false);
    }


    private void drawMenuBar() {
        float menuBarZ;
        synchronized (mCameraPositionLock) {
            menuBarZ = mCameraPosition + MENU_BAR_POSITION_Z;
        }

        mMenuBar.draw(mViewMatrix, mProjectionMatrix, mEyeMenuBarPosition, MENU_BAR_POSITION_X,
                MENU_BAR_POSITION_Y, menuBarZ);
    }

    /**
     * Get eye position at the given distance.
     */
    private PointF getLookingPosition(float distance) {
        if (Math.abs(mForwardVector[2]) < EPSILON) {
            return new PointF(Math.copySign(Float.MAX_VALUE, mForwardVector[0]),
                    Math.copySign(Float.MAX_VALUE, mForwardVector[1]));
        } else {
            return new PointF(mForwardVector[0] * distance / mForwardVector[2],
                    mForwardVector[1] * distance / mForwardVector[2]);
        }
    }


}
