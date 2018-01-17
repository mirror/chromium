// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.router.cast;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.support.v7.media.MediaRouter;

import com.google.android.gms.cast.Cast;
import com.google.android.gms.cast.LaunchOptions;
import com.google.android.gms.cast.framework.CastContext;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.browser.media.router.ChromeMediaRouter;
import org.chromium.chrome.browser.media.router.MediaRoute;
import org.chromium.chrome.browser.media.router.cast.remoting.RemotingCastSession;

import javax.annotation.Nullable;

/**
 * Establishes a {@link MediaRoute} by starting a Cast application represented by the given
 * presentation URL. Reports success or failure to {@link ChromeMediaRouter}.
 * Since there're numerous asynchronous calls involved in getting the application to launch
 * the class is implemented as a state machine.
 */
public class CreateRouteRequest implements ChromeCastSessionManager.CastSessionLaunchRequest {
    private static final String TAG = "MediaRouter";

    private static final int STATE_IDLE = 0;
    private static final int STATE_CONNECTING_TO_API = 1;
    private static final int STATE_API_CONNECTION_SUSPENDED = 2;
    private static final int STATE_LAUNCHING_APPLICATION = 3;
    private static final int STATE_LAUNCH_SUCCEEDED = 4;
    private static final int STATE_TERMINATED = 5;

    private final MediaSource mSource;
    private final MediaSink mSink;
    private final String mPresentationId;
    private final String mOrigin;
    private final int mTabId;
    private final boolean mIsIncognito;
    private final int mRequestId;
    private final CastMessageHandler mMessageHandler;
    private final ChromeCastSessionManager.CastSessionManagerListener mSessionListener;
    private final RequestedCastSessionType mSessionType;

    private int mState = STATE_IDLE;

    // Used to identify whether the request should launch a CastSessionImpl or a RemotingCastSession
    // (based off of wheter the route creation was requested by a RemotingMediaRouteProvider or a
    // CastMediaRouteProvider).
    public enum RequestedCastSessionType { CAST, REMOTE }

    /**
     * Initializes the request.
     * @param source The {@link MediaSource} defining the application to launch on the Cast device.
     * @param sink The {@link MediaSink} identifying the selected Cast device.
     * @param presentationId The presentation id assigned to the route by {@link ChromeMediaRouter}.
     * @param origin The origin of the frame requesting the route.
     * @param tabId The id of the tab containing the frame requesting the route.
     * @param isIncognito Whether the route is being requested from an Incognito profile.
     * @param requestId The id of the route creation request for tracking by
     * {@link ChromeMediaRouter}.
     * @param listener The listener that should be notified of session/route creation changes.
     * @param messageHandler A message handler (used to create CastSessionImpl instances).
     */
    public CreateRouteRequest(MediaSource source, MediaSink sink, String presentationId,
            String origin, int tabId, boolean isIncognito, int requestId,
            ChromeCastSessionManager.CastSessionManagerListener listener,
            RequestedCastSessionType sessionType, @Nullable CastMessageHandler messageHandler) {
        assert source != null;
        assert sink != null;

        mSource = source;
        mSink = sink;
        mPresentationId = presentationId;
        mOrigin = origin;
        mTabId = tabId;
        mIsIncognito = isIncognito;
        mRequestId = requestId;
        mSessionListener = listener;
        mSessionType = sessionType;
        mMessageHandler = messageHandler;
    }

    public MediaSource getSource() {
        return mSource;
    }

    public MediaSink getSink() {
        return mSink;
    }

    public String getPresentationId() {
        return mPresentationId;
    }

    public String getOrigin() {
        return mOrigin;
    }

    public int getTabId() {
        return mTabId;
    }

    public boolean isIncognito() {
        return mIsIncognito;
    }

    public int getNativeRequestId() {
        return mRequestId;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////
    // ChromeCastSessionManager.CastSessionLaunchRequest implementation.

    /**
     * Returns the object that should be notified of session changes that result
     * from this route creation request.
     */
    @Override
    public ChromeCastSessionManager.CastSessionManagerListener getSessionListener() {
        return mSessionListener;
    }

    /**
     * Starts the process of launching the application on the Cast device.
     */
    @Override
    public void start(Cast.Listener castListener) {
        if (mState != STATE_IDLE) throwInvalidState();

        CastUtils.getCastContext().setReceiverApplicationId(mSource.getApplicationId());

        MediaRouter mediaRouter = ChromeMediaRouter.getAndroidMediaRouter();
        for (MediaRouter.RouteInfo routeInfo : mediaRouter.getRoutes()) {
            if (routeInfo.getId().equals(mSink.getId())) {
                // Unselect and then select so that CAF will get notified of the selection.
                mediaRouter.unselect(0);
                routeInfo.select();
                break;
            }
        }
    }

    // TODO(crbug.com/635567): Fix this properly.
    @SuppressLint("DefaultLocale")
    private void throwInvalidState() {
        throw new RuntimeException(String.format("Invalid state: %d", mState));
    }
}
