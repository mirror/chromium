// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.SurfaceTexture;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.TextureView;
import android.view.TextureView.SurfaceTextureListener;
import android.util.Log;
import android.widget.FrameLayout;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.ThreadUtils;
import org.chromium.ui.base.WindowAndroid;

import static org.chromium.ui.base.WindowAndroid.getSurfaceTexture;
import static org.chromium.ui.base.WindowAndroid.addListener;

/***
 * This view is used by a ContentView to render its content.
 * Call {@link #setCurrentContentViewCore(ContentViewCore)} with the contentViewCore that should be
 * managing the view.
 * Note that only one ContentViewCore can be shown at a time.
 */
@JNINamespace("content")
public class ContentViewRenderView
        extends FrameLayout implements WindowAndroid.SurfaceListener {
    // The native side of this object.
    private long mNativeContentViewRenderView;

    private SurfaceHolder.Callback mSurfaceCallback;
    private final SurfaceView mSurfaceView;
    //private final TextureView mSurfaceView;

    private Surface mOldSurface = null;
    private Surface mSurface = null;
    private boolean mUseOldSurface = true;
    protected ContentViewCore mContentViewCore;

    /**
     * Constructs a new ContentViewRenderView.
     * This should be called and the {@link ContentViewRenderView} should be added to the view
     * hierarchy before the first draw to avoid a black flash that is seen every time a
     * {@link SurfaceView} is added.
     * @param context The context used to create this.
     */
    public ContentViewRenderView(Context context) {
        super(context);

        mSurfaceView = new SurfaceView(getContext());
        mSurfaceView.setZOrderMediaOverlay(true);

        setSurfaceViewBackgroundColor(Color.WHITE);
        addView(mSurfaceView,
                new FrameLayout.LayoutParams(
                        FrameLayout.LayoutParams.MATCH_PARENT,
                        FrameLayout.LayoutParams.MATCH_PARENT));
        mSurfaceView.setVisibility(GONE);
        addListener(this);
     }

    @Override
    public void OnNewSurface(SurfaceTexture surface) {
      final int width = getWidth() > getHeight() ? getWidth() : getHeight();
      final int height = getWidth() < getHeight() ? getWidth() : getHeight();
      surface.setDefaultBufferSize(width, height);
      mSurface = new Surface(surface);
      mUseOldSurface = false;
      ThreadUtils.postOnUiThread(new Runnable(){
        @Override
        public void run() {
          Log.d("bshe:log", "First set new surface texture");
          nativeSurfaceCreated(mNativeContentViewRenderView);
          mSurfaceView.setVisibility(GONE);
          mSurfaceCallback.surfaceChanged(null, 4, width, height);
        }
      });
    }

    public void useSurface(boolean old) {
        // hack to allow surface update. If the format of surface is the same, compositor wont
        // try to update the surface in onSurfaceTextureSizeChanged.
        mUseOldSurface = old;
        nativeSurfaceCreated(mNativeContentViewRenderView);
        if (mUseOldSurface) {
            mSurfaceView.setVisibility(VISIBLE);
        } else {
            mSurfaceView.setVisibility(GONE);
            int width = getWidth() > getHeight() ? getWidth() : getHeight();
            int height = getWidth() < getHeight() ? getWidth() : getHeight();
            mSurfaceCallback.surfaceChanged(null, 4, width, height);
        }
    }


    /**
     * Initialization that requires native libraries should be done here.
     * Native code should add/remove the layers to be rendered through the ContentViewLayerRenderer.
     * @param rootWindow The {@link WindowAndroid} this render view should be linked to.
     */
    public void onNativeLibraryLoaded(WindowAndroid rootWindow) {
        assert !mSurfaceView.getHolder().getSurface().isValid() :
                        "Surface created before native library loaded.";
        Log.d("bshe:log", "onNativeLibaryLoaded. TextureView should be visible.");
        assert rootWindow != null;
        mNativeContentViewRenderView = nativeInit(rootWindow.getNativePointer());
        assert mNativeContentViewRenderView != 0;
        mSurfaceCallback = new SurfaceHolder.Callback() {
            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                assert mNativeContentViewRenderView != 0;

                if (mUseOldSurface) {
                    Log.d("bshe:log", "use old surface");
                    Log.d("bshe:log", "format = " + format);
                    Log.d("bshe:log", "width = " + width + " height = " + height);
                    Log.d("bshe:log", "getWidth = " + getWidth() + " height = " + getHeight());
                    nativeSurfaceChanged(
                        mNativeContentViewRenderView, format, width, height, mOldSurface);
                } else if (!mUseOldSurface && mSurface != null) {
                    Log.d("bshe:log", "use new surface");
                    Log.d("bshe:log", "format = " + format);
                    Log.d("bshe:log", "width = " + width + " height = " + height);
                    Log.d("bshe:log", "getWidth = " + getWidth() + " height = " + getHeight());
                    nativeSurfaceChanged(
                        mNativeContentViewRenderView, format, width, height, mSurface);
                }
                if (mContentViewCore != null) {
                    mContentViewCore.onPhysicalBackingSizeChanged(width, height);
                }

            }

            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                assert mNativeContentViewRenderView != 0;
                mOldSurface = holder.getSurface();
                nativeSurfaceCreated(mNativeContentViewRenderView);

                onReadyToRender();
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                assert mNativeContentViewRenderView != 0;
                nativeSurfaceDestroyed(mNativeContentViewRenderView);
            }
        };
        mSurfaceView.getHolder().addCallback(mSurfaceCallback);
        mSurfaceView.setVisibility(VISIBLE);
    }

    /**
     * Sets the background color of the surface view.  This method is necessary because the
     * background color of ContentViewRenderView itself is covered by the background of
     * SurfaceView.
     * @param color The color of the background.
     */
    public void setSurfaceViewBackgroundColor(int color) {
        if (mSurfaceView != null) {
            mSurfaceView.setBackgroundColor(color);
        }
    }

    /**
     * Gets the SurfaceView for this ContentViewRenderView
     */
    public View getSurfaceView() {
        return mSurfaceView;
    }

    /**
     * Should be called when the ContentViewRenderView is not needed anymore so its associated
     * native resource can be freed.
     */
    public void destroy() {
        mSurfaceView.getHolder().removeCallback(mSurfaceCallback);
        nativeDestroy(mNativeContentViewRenderView);
        mNativeContentViewRenderView = 0;
    }

    public void setCurrentContentViewCore(ContentViewCore contentViewCore) {
        assert mNativeContentViewRenderView != 0;
        mContentViewCore = contentViewCore;

        if (mContentViewCore != null) {
            mContentViewCore.onPhysicalBackingSizeChanged(getWidth(), getHeight());
            nativeSetCurrentContentViewCore(mNativeContentViewRenderView,
                                            mContentViewCore.getNativeContentViewCore());
        } else {
            nativeSetCurrentContentViewCore(mNativeContentViewRenderView, 0);
        }
    }

    /**
     * This method should be subclassed to provide actions to be performed once the view is ready to
     * render.
     */
    protected void onReadyToRender() {
    }

    /**
     * This method could be subclassed optionally to provide a custom SurfaceView object to
     * this ContentViewRenderView.
     * @param context The context used to create the SurfaceView object.
     * @return The created SurfaceView object.
     */
    protected SurfaceView createSurfaceView(Context context) {
        return new SurfaceView(context);
    }

    /**
     * @return whether the surface view is initialized and ready to render.
     */
    public boolean isInitialized() {
        return mSurfaceView.getHolder().getSurface() != null;
//        return mSurfaceView.getSurfaceTexture() != null;
    }

    /**
     * Enter or leave overlay video mode.
     * @param enabled Whether overlay mode is enabled.
     */
    public void setOverlayVideoMode(boolean enabled) {
        int format = enabled ? PixelFormat.TRANSLUCENT : PixelFormat.OPAQUE;
        // mSurfaceView.getHolder().setFormat(format);
        nativeSetOverlayVideoMode(mNativeContentViewRenderView, enabled);
    }

    @CalledByNative
    private void onSwapBuffersCompleted() {
        if (mSurfaceView.getBackground() != null) {
            post(new Runnable() {
                @Override public void run() {
                    mSurfaceView.setBackgroundResource(0);
                }
            });
        }
    }

    private native long nativeInit(long rootWindowNativePointer);
    private native void nativeDestroy(long nativeContentViewRenderView);
    private native void nativeSetCurrentContentViewCore(long nativeContentViewRenderView,
            long nativeContentViewCore);
    private native void nativeSurfaceCreated(long nativeContentViewRenderView);
    private native void nativeSurfaceDestroyed(long nativeContentViewRenderView);
    private native void nativeSurfaceChanged(long nativeContentViewRenderView,
            int format, int width, int height, Surface surface);
    private native void nativeSetOverlayVideoMode(long nativeContentViewRenderView,
            boolean enabled);
}
