// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.media;

import android.media.MediaPlayer;
import android.media.MediaPlayer.TrackInfo;
import android.net.Uri;
import android.os.Build;
import android.text.TextUtils;
import android.view.Surface;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.HashMap;

/**
* A wrapper around android.media.MediaPlayer that allows the native code to use it.
* See media/base/android/media_player_bridge.cc for the corresponding native code.
*/
@JNINamespace("media")
public class MediaPlayerBridge {

    private static final String TAG = "cr.media";

    // Local player to forward this to. We don't initialize it here since the subclass might not
    // want it.
    private MediaPlayer mPlayer;
    private long mNativeMediaPlayerBridge;

    @CalledByNative
    private static MediaPlayerBridge create(long nativeMediaPlayerBridge) {
        return new MediaPlayerBridge(nativeMediaPlayerBridge);
    }

    protected MediaPlayerBridge(long nativeMediaPlayerBridge) {
        mNativeMediaPlayerBridge = nativeMediaPlayerBridge;
    }

    protected MediaPlayerBridge() {
    }

    @CalledByNative
    protected void destroy() {
        mNativeMediaPlayerBridge = 0;
    }

    protected MediaPlayer getLocalPlayer() {
        if (mPlayer == null) {
            mPlayer = new MediaPlayer();
        }
        return mPlayer;
    }

    @CalledByNative
    protected void setSurface(Surface surface) {
        getLocalPlayer().setSurface(surface);
    }

    @CalledByNative
    protected boolean prepareAsync() {
        try {
            getLocalPlayer().prepareAsync();
        } catch (IllegalStateException ise) {
            Log.e(TAG, "Unable to prepare MediaPlayer.", ise);
            return false;
        } catch (Exception e) {
            // Catch IOException thrown by android MediaPlayer native code.
            Log.e(TAG, "Unable to prepare MediaPlayer.", e);
            return false;
        }
        return true;
    }

    @CalledByNative
    protected boolean isPlaying() {
        return getLocalPlayer().isPlaying();
    }

    @CalledByNative
    protected boolean hasVideo() {
        return hasTrack(TrackInfo.MEDIA_TRACK_TYPE_VIDEO);
    }

    @CalledByNative
    protected boolean hasAudio() {
        return hasTrack(TrackInfo.MEDIA_TRACK_TYPE_AUDIO);
    }

    private boolean hasTrack(int trackType) {
        try {
            TrackInfo trackInfo[] = getLocalPlayer().getTrackInfo();

            // HLS media does not have the track info, so we treat them conservatively.
            if (trackInfo.length == 0) return true;

            for (TrackInfo info : trackInfo) {
                // TODO(zqzhang): may be we can have a histogram recording
                // media track types in the future.
                // See http://crbug.com/571411
                if (trackType == info.getTrackType()) return true;
                if (TrackInfo.MEDIA_TRACK_TYPE_UNKNOWN == info.getTrackType()) return true;
            }
        } catch (RuntimeException e) {
            // Exceptions may come from getTrackInfo (IllegalStateException/RuntimeException), or
            // from some customized OS returning null TrackInfos (NullPointerException).
            return true;
        }
        return false;
    }

    @CalledByNative
    protected int getVideoWidth() {
        return getLocalPlayer().getVideoWidth();
    }

    @CalledByNative
    protected int getVideoHeight() {
        return getLocalPlayer().getVideoHeight();
    }

    @CalledByNative
    protected int getCurrentPosition() {
        return getLocalPlayer().getCurrentPosition();
    }

    @CalledByNative
    protected int getDuration() {
        return getLocalPlayer().getDuration();
    }

    @CalledByNative
    protected void release() {
        getLocalPlayer().release();
    }

    @CalledByNative
    protected void setVolume(double volume) {
        getLocalPlayer().setVolume((float) volume, (float) volume);
    }

    @CalledByNative
    protected void start() {
        getLocalPlayer().start();
    }

    @CalledByNative
    protected void pause() {
        getLocalPlayer().pause();
    }

    @CalledByNative
    protected void seekTo(int msec) throws IllegalStateException {
        getLocalPlayer().seekTo(msec);
    }

    @CalledByNative
    protected boolean setDataSource(
            String url, String cookies, String userAgent, boolean hideUrlLog) {
        Uri uri = Uri.parse(url);
        HashMap<String, String> headersMap = new HashMap<String, String>();
        if (hideUrlLog) headersMap.put("x-hide-urls-from-log", "true");
        if (!TextUtils.isEmpty(cookies)) headersMap.put("Cookie", cookies);
        if (!TextUtils.isEmpty(userAgent)) headersMap.put("User-Agent", userAgent);
        // The security origin check is enforced for devices above K. For devices below K,
        // only anonymous media HTTP request (no cookies) may be considered same-origin.
        // Note that if the server rejects the request we must not consider it same-origin.
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.KITKAT) {
            headersMap.put("allow-cross-domain-redirect", "false");
        }
        try {
            getLocalPlayer().setDataSource(ContextUtils.getApplicationContext(), uri, headersMap);
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    protected void setOnBufferingUpdateListener(MediaPlayer.OnBufferingUpdateListener listener) {
        getLocalPlayer().setOnBufferingUpdateListener(listener);
    }

    protected void setOnCompletionListener(MediaPlayer.OnCompletionListener listener) {
        getLocalPlayer().setOnCompletionListener(listener);
    }

    protected void setOnErrorListener(MediaPlayer.OnErrorListener listener) {
        getLocalPlayer().setOnErrorListener(listener);
    }

    protected void setOnPreparedListener(MediaPlayer.OnPreparedListener listener) {
        getLocalPlayer().setOnPreparedListener(listener);
    }

    protected void setOnSeekCompleteListener(MediaPlayer.OnSeekCompleteListener listener) {
        getLocalPlayer().setOnSeekCompleteListener(listener);
    }

    protected void setOnVideoSizeChangedListener(MediaPlayer.OnVideoSizeChangedListener listener) {
        getLocalPlayer().setOnVideoSizeChangedListener(listener);
    }

    protected static class AllowedOperations {
        private final boolean mCanPause;
        private final boolean mCanSeekForward;
        private final boolean mCanSeekBackward;

        public AllowedOperations(boolean canPause, boolean canSeekForward,
                boolean canSeekBackward) {
            mCanPause = canPause;
            mCanSeekForward = canSeekForward;
            mCanSeekBackward = canSeekBackward;
        }

        @CalledByNative("AllowedOperations")
        private boolean canPause() {
            return mCanPause;
        }

        @CalledByNative("AllowedOperations")
        private boolean canSeekForward() {
            return mCanSeekForward;
        }

        @CalledByNative("AllowedOperations")
        private boolean canSeekBackward() {
            return mCanSeekBackward;
        }
    }

    /**
     * Returns an AllowedOperations object to show all the operations that are
     * allowed on the media player.
     */
    @CalledByNative
    protected AllowedOperations getAllowedOperations() {
        MediaPlayer player = getLocalPlayer();
        boolean canPause = true;
        boolean canSeekForward = true;
        boolean canSeekBackward = true;
        try {
            Method getMetadata = player.getClass().getDeclaredMethod(
                    "getMetadata", boolean.class, boolean.class);
            getMetadata.setAccessible(true);
            Object data = getMetadata.invoke(player, false, false);
            if (data != null) {
                Class<?> metadataClass = data.getClass();
                Method hasMethod = metadataClass.getDeclaredMethod("has", int.class);
                Method getBooleanMethod = metadataClass.getDeclaredMethod("getBoolean", int.class);

                int pause = (Integer) metadataClass.getField("PAUSE_AVAILABLE").get(null);
                int seekForward =
                        (Integer) metadataClass.getField("SEEK_FORWARD_AVAILABLE").get(null);
                int seekBackward =
                        (Integer) metadataClass.getField("SEEK_BACKWARD_AVAILABLE").get(null);
                hasMethod.setAccessible(true);
                getBooleanMethod.setAccessible(true);
                canPause = !((Boolean) hasMethod.invoke(data, pause))
                        || ((Boolean) getBooleanMethod.invoke(data, pause));
                canSeekForward = !((Boolean) hasMethod.invoke(data, seekForward))
                        || ((Boolean) getBooleanMethod.invoke(data, seekForward));
                canSeekBackward = !((Boolean) hasMethod.invoke(data, seekBackward))
                        || ((Boolean) getBooleanMethod.invoke(data, seekBackward));
            }
        } catch (NoSuchMethodException e) {
            Log.e(TAG, "Cannot find getMetadata() method: " + e);
        } catch (InvocationTargetException e) {
            Log.e(TAG, "Cannot invoke MediaPlayer.getMetadata() method: " + e);
        } catch (IllegalAccessException e) {
            Log.e(TAG, "Cannot access metadata: " + e);
        } catch (NoSuchFieldException e) {
            Log.e(TAG, "Cannot find matching fields in Metadata class: " + e);
        }
        return new AllowedOperations(canPause, canSeekForward, canSeekBackward);
    }
}
