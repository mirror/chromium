// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.media.router.cast.remoting;

import android.support.v7.media.MediaRouter;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.media.router.BaseMediaRouteProvider;
import org.chromium.chrome.browser.media.router.ChromeMediaRouter;
import org.chromium.chrome.browser.media.router.MediaRoute;
import org.chromium.chrome.browser.media.router.MediaRouteManager;
import org.chromium.chrome.browser.media.router.MediaRouteProvider;
import org.chromium.chrome.browser.media.router.cast.ChromeCastSessionManager;
import org.chromium.chrome.browser.media.router.cast.CreateRouteRequest;
import org.chromium.chrome.browser.media.router.cast.MediaSink;
import org.chromium.chrome.browser.media.router.cast.MediaSource;

/**
 * A {@link MediaRouteProvider} implementation for media remote playback.
 */
public class RemotingMediaRouteProvider extends BaseMediaRouteProvider {
    private static final String TAG = "MediaRemoting";

    /**
     * @return Initialized {@link RemotingMediaRouteProvider} object.
     */
    public static RemotingMediaRouteProvider create(MediaRouteManager manager) {
        MediaRouter androidMediaRouter = ChromeMediaRouter.getAndroidMediaRouter();
        return new RemotingMediaRouteProvider(androidMediaRouter, manager);
    }

    @Override
    public boolean supportsSource(String sourceId) {
        return getSourceFromId(sourceId) != null;
    }

    @Override
    protected MediaSource getSourceFromId(String sourceId) {
        return RemotingMediaSource.from(sourceId);
    }

    @Override
    public void createRoute(String sourceId, String sinkId, String presentationId, String origin,
            int tabId, boolean isIncognito, int nativeRequestId) {
        if (mAndroidMediaRouter == null) {
            mManager.onRouteRequestError("Not supported", nativeRequestId);
            return;
        }

        MediaSink sink = MediaSink.fromSinkId(sinkId, mAndroidMediaRouter);
        if (sink == null) {
            mManager.onRouteRequestError("No sink", nativeRequestId);
            return;
        }

        MediaSource source = getSourceFromId(sourceId);
        if (source == null) {
            mManager.onRouteRequestError("Unsupported presentation URL", nativeRequestId);
            return;
        }

        CreateRouteRequest createRouteRequest = new CreateRouteRequest(source, sink, presentationId,
                origin, tabId, isIncognito, nativeRequestId, this,
                CreateRouteRequest.RequestedCastSessionType.REMOTE, null);

        ChromeCastSessionManager.get().requestSessionLaunch(createRouteRequest);
    }

    @Override
    public void joinRoute(
            String sourceId, String presentationId, String origin, int tabId, int nativeRequestId) {
        // Remote playback does not support rejoining routes.
    }

    @Override
    public void closeRoute(String routeId) {
        MediaRoute route = mRoutes.get(routeId);
        if (route == null) return;

        if (mSession == null) {
            mRoutes.remove(routeId);
            mManager.onRouteClosed(routeId);
            return;
        }

        ChromeCastSessionManager.get().stopApplication();
    }

    @Override
    public void detachRoute(String routeId) {
        mRoutes.remove(routeId);
    }

    @Override
    public void sendStringMessage(String routeId, String message, int nativeCallbackId) {
        // Remote playback does not support sending string messages.
    }

    @VisibleForTesting
    RemotingMediaRouteProvider(MediaRouter androidMediaRouter, MediaRouteManager manager) {
        super(androidMediaRouter, manager);
    }

    @Override
    public void onSessionClosed() {
        if (mSession == null) return;

        for (String routeId : mRoutes.keySet()) mManager.onRouteClosed(routeId);
        mRoutes.clear();

        mSession = null;

        if (mAndroidMediaRouter != null) {
            mAndroidMediaRouter.selectRoute(mAndroidMediaRouter.getDefaultRoute());
        }
    }

    @Override
    public void onSessionLaunching(CreateRouteRequest request) {
        MediaSink sink = request.getSink();
        MediaSource source = request.getSource();

        MediaRoute route =
                new MediaRoute(sink.getId(), source.getSourceId(), request.getPresentationId());
        mRoutes.put(route.id, route);
        mManager.onRouteCreated(route.id, route.sinkId, request.getNativeRequestId(), this, true);
    }
}
