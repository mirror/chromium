// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.router.cast;

import android.annotation.SuppressLint;

import com.google.android.gms.cast.ApplicationMetadata;
import com.google.android.gms.cast.Cast;
import com.google.android.gms.cast.CastStatusCodes;

import org.chromium.base.Log;

/**
 * Manages the lifetime of the active CastSessions and broadcasts state changes to observers.
 */
public class ChromeCastSessionManager {
    private static final String TAG = "cr_CastSessionMgr";

    private static ChromeCastSessionManager sInstance;

    /**
     * Defines the events relevant to CastSession state changes.
     */
    public interface ChromeCastSessionObserver {
        // Called right before a session launch is started.
        public void onSessionLaunching(CreateRouteRequest originalRequest);

        // Called after a successful session launch.
        public void onSessionCreated(CastSession session);

        // Called after a failed session launch.
        public void onSessionLaunchError();

        // Called when a session is shutting down (as a result of user action, or when a new
        // session is being launched)
        public void onSessionStopAction();

        // Called after a session has shut down.
        public void onSessionClosed();
    }

    private class CastListener extends Cast.Listener {
        private CastSession mSession;

        CastListener() {}

        void setSession(CastSession session) {
            mSession = session;
        }

        @Override
        public void onApplicationStatusChanged() {
            if (mSession == null) return;

            mSession.updateSessionStatus();
        }

        @Override
        public void onApplicationMetadataChanged(ApplicationMetadata metadata) {
            if (mSession == null) return;

            mSession.updateSessionStatus();
        }

        @Override
        // TODO(crbug.com/635567): Fix this properly.
        @SuppressLint("DefaultLocale")
        public void onApplicationDisconnected(int errorCode) {
            if (errorCode != CastStatusCodes.SUCCESS) {
                Log.e(TAG, String.format("Application disconnected with: %d", errorCode));
            }

            // This callback can be called more than once if the application is stopped from Chrome.
            if (mSession == null) return;

            mSession.stopApplication();
            mSession = null;
        }

        @Override
        public void onVolumeChanged() {
            if (mSession == null) return;

            mSession.onVolumeChanged();
        }
    }

    // The current active session. There can only be one active session on Android.
    // NOTE: This is a Chromium abstraction, different from the Cast SDK CastSession.
    private CastSession mSession;

    // Listener tied to |mSession|.
    private CastListener mListener;

    // Route request to be started once |mSession| is stopped.
    private CreateRouteRequest mPendingRouteRequest;

    // Object that initiated the current cast session and that should be notified
    // of changes to the session.
    private ChromeCastSessionObserver mCurrentSessionObserver;

    public static ChromeCastSessionManager get() {
        if (sInstance == null) sInstance = new ChromeCastSessionManager();

        return sInstance;
    }

    private ChromeCastSessionManager() {}

    public void onSessionCreated(CastSession session, ChromeCastSessionObserver sessionObserver) {
        assert mSession == null;

        mSession = session;
        mCurrentSessionObserver = sessionObserver;

        mCurrentSessionObserver.onSessionCreated(session);
    }

    public void requestSessionLaunch(CreateRouteRequest request) {
        // Since we only can only have one session, close it before starting a new one.
        if (mSession != null) {
            mPendingRouteRequest = request;
            mSession.stopApplication();
            return;
        }

        launchSession(request);
    }

    public void stopApplication() {
        mSession.stopApplication();
    }

    public void launchSession(CreateRouteRequest request) {
        request.getSessionObserver().onSessionLaunching(request);

        mListener = new CastListener();

        request.start(mListener);
    }

    public void onSessionStopAction() {
        mCurrentSessionObserver.onSessionStopAction();
    }

    public void onSessionClosed() {
        mCurrentSessionObserver.onSessionClosed();

        mSession = null;
        mCurrentSessionObserver = null;
        mListener = null;

        if (mPendingRouteRequest != null) {
            launchSession(mPendingRouteRequest);
            mPendingRouteRequest = null;
        }
    }

    public void onSessionLaunchError() {
        mCurrentSessionObserver.onSessionLaunchError();
    }
}
