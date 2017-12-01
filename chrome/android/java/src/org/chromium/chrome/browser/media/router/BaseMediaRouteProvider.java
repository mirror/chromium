// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.media.router;

import android.os.Handler;
import android.support.v7.media.MediaRouteSelector;
import android.support.v7.media.MediaRouter;
import android.support.v7.media.MediaRouter.RouteInfo;

import org.chromium.base.Log;
import org.chromium.chrome.browser.media.router.cast.CastSession;
import org.chromium.chrome.browser.media.router.cast.ChromeCastSessionManager;
import org.chromium.chrome.browser.media.router.cast.CreateRouteRequest;
import org.chromium.chrome.browser.media.router.cast.DiscoveryCallback;
import org.chromium.chrome.browser.media.router.cast.MediaSink;
import org.chromium.chrome.browser.media.router.cast.MediaSource;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A {@link BaseMediaRouteProvider} common implementation for MediaRouteProviders.
 */
public abstract class BaseMediaRouteProvider
        implements MediaRouteProvider, DiscoveryDelegate,
                   ChromeCastSessionManager.ChromeCastSessionObserver {
    private static final String TAG = "MediaRouter";

    protected static final List<MediaSink> NO_SINKS = Collections.emptyList();

    protected final MediaRouter mAndroidMediaRouter;
    protected final MediaRouteManager mManager;
    protected final Map<String, DiscoveryCallback> mDiscoveryCallbacks =
            new HashMap<String, DiscoveryCallback>();
    protected final Map<String, MediaRoute> mRoutes = new HashMap<String, MediaRoute>();
    protected Handler mHandler = new Handler();

    // There can be only one Cast session at the same time on Android.
    protected CastSession mSession;

    protected static class OnSinksReceivedRunnable implements Runnable {
        private final WeakReference<MediaRouteManager> mRouteManager;
        private final MediaRouteProvider mRouteProvider;
        private final String mSourceId;
        private final List<MediaSink> mSinks;

        public OnSinksReceivedRunnable(MediaRouteManager manager, MediaRouteProvider routeProvider,
                String sourceId, List<MediaSink> sinks) {
            mRouteManager = new WeakReference<MediaRouteManager>(manager);
            mRouteProvider = routeProvider;
            mSourceId = sourceId;
            mSinks = sinks;
        }

        @Override
        public void run() {
            MediaRouteManager manager = mRouteManager.get();
            if (manager != null) {
                Log.d(TAG, "Reporting %d sinks for source: %s", mSinks.size(), mSourceId);
                manager.onSinksReceived(mSourceId, mRouteProvider, mSinks);
            }
        }
    }

    protected BaseMediaRouteProvider(MediaRouter androidMediaRouter, MediaRouteManager manager) {
        mAndroidMediaRouter = androidMediaRouter;
        mManager = manager;
    }

    /**
     * {@link DiscoveryDelegate} implementation.
     */
    @Override
    public void onSinksReceived(String sourceId, List<MediaSink> sinks) {
        Log.d(TAG, "Received %d sinks for sourceId: %s", sinks.size(), sourceId);
        mHandler.post(new OnSinksReceivedRunnable(mManager, this, sourceId, sinks));
    }

    protected abstract MediaSource getSourceFromId(String sourceId);

    @Override
    public void startObservingMediaSinks(String sourceId) {
        Log.d(TAG, "startObservingMediaSinks: " + sourceId);

        if (mAndroidMediaRouter == null) {
            // If the MediaRouter API is not available, report no devices so the page doesn't even
            // try to cast.
            onSinksReceived(sourceId, NO_SINKS);
            return;
        }

        MediaSource source = getSourceFromId(sourceId);
        if (source == null) {
            // If the source is invalid or not supported by this provider, report no devices
            // available.
            onSinksReceived(sourceId, NO_SINKS);
            return;
        }

        MediaRouteSelector routeSelector = source.buildRouteSelector();
        if (routeSelector == null) {
            // If the application invalid, report no devices available.
            onSinksReceived(sourceId, NO_SINKS);
            return;
        }

        // No-op, if already monitoring the application for this source.
        String applicationId = source.getApplicationId();
        DiscoveryCallback callback = mDiscoveryCallbacks.get(applicationId);
        if (callback != null) {
            callback.addSourceUrn(sourceId);
            return;
        }

        List<MediaSink> knownSinks = new ArrayList<MediaSink>();
        for (RouteInfo route : mAndroidMediaRouter.getRoutes()) {
            if (route.matchesSelector(routeSelector)) {
                knownSinks.add(MediaSink.fromRoute(route));
            }
        }

        callback = new DiscoveryCallback(sourceId, knownSinks, this, routeSelector);
        mAndroidMediaRouter.addCallback(
                routeSelector, callback, MediaRouter.CALLBACK_FLAG_REQUEST_DISCOVERY);
        mDiscoveryCallbacks.put(applicationId, callback);
    }

    @Override
    public void stopObservingMediaSinks(String sourceId) {
        Log.d(TAG, "stopObservingMediaSinks: " + sourceId);
        if (mAndroidMediaRouter == null) return;

        MediaSource source = getSourceFromId(sourceId);
        if (source == null) return;

        String applicationId = source.getApplicationId();
        DiscoveryCallback callback = mDiscoveryCallbacks.get(applicationId);
        if (callback == null) return;

        callback.removeSourceUrn(sourceId);

        if (callback.isEmpty()) {
            mAndroidMediaRouter.removeCallback(callback);
            mDiscoveryCallbacks.remove(applicationId);
        }
    }

    @Override
    public void createRoute(String sourceId, String sinkId, String presentationId, String origin,
            int tabId, boolean isIncognito, int nativeRequestId) {}

    @Override
    public void joinRoute(String sourceId, String presentationId, String origin, int tabId,
            int nativeRequestId) {}

    @Override
    public void closeRoute(String routeId) {}

    @Override
    public void detachRoute(String routeId) {}

    @Override
    public void sendStringMessage(String routeId, String message, int nativeCallbackId) {}

    // ChromeCastSessionObserver implementation
    @Override
    public abstract void onSessionLaunching(CreateRouteRequest issuingRequest);

    @Override
    public abstract void onSessionClosed();

    @Override
    public void onSessionLaunchError() {
        for (String routeId : mRoutes.keySet()) {
            mManager.onRouteClosedWithError(routeId, "Launch error");
        }
        mRoutes.clear();
    };

    @Override
    public void onSessionCreated(CastSession session) {
        mSession = session;
    }

    @Override
    public void onSessionStopAction() {
        if (mSession == null) return;

        for (String routeId : mRoutes.keySet()) closeRoute(routeId);
    }
}
