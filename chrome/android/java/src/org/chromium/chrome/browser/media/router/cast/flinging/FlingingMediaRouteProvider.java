// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.router.cast.flinging;

import android.net.Uri;
import android.os.Handler;
import android.support.v7.media.MediaRouteSelector;
import android.support.v7.media.MediaRouter;
import android.support.v7.media.MediaRouter.RouteInfo;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.ChromeApplication;
import org.chromium.chrome.browser.media.MediaUrlResolver;
import org.chromium.chrome.browser.media.router.ChromeMediaRouter;
import org.chromium.chrome.browser.media.router.DiscoveryDelegate;
import org.chromium.chrome.browser.media.router.MediaRouteManager;
import org.chromium.chrome.browser.media.router.MediaRouteProvider;
import org.chromium.chrome.browser.media.router.cast.DiscoveryCallback;
import org.chromium.chrome.browser.media.router.cast.MediaSink;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;

/**
 * A {@link MediaRouteProvider} implementation for media flinging.
 */
public class FlingingMediaRouteProvider implements MediaRouteProvider, DiscoveryDelegate {
    private static final String TAG = "MediaFlinging";

    private final MediaRouter mAndroidMediaRouter;
    private final MediaRouteManager mManager;
    private final Map<String, MediaUrlResolverDelegate> mUrlResolvers =
            new HashMap<String, MediaUrlResolverDelegate>();
    private final Map<String, DiscoveryCallback> mDiscoveryCallbacks =
            new HashMap<String, DiscoveryCallback>();
    private Handler mHandler = new Handler();

    private static class OnSinksReceivedRunnable implements Runnable {
        private final WeakReference<MediaRouteManager> mRouteManager;
        private final MediaRouteProvider mRouteProvider;
        private final String mSourceId;
        private final List<MediaSink> mSinks;

        OnSinksReceivedRunnable(MediaRouteManager manager, MediaRouteProvider routeProvider,
                String sourceId, List<MediaSink> sinks) {
            mRouteManager = new WeakReference<MediaRouteManager>(manager);
            mRouteProvider = routeProvider;
            mSourceId = sourceId;
            mSinks = sinks;
        }

        @Override
        public void run() {
            MediaRouteManager manager = mRouteManager.get();
            if (manager != null) manager.onSinksReceived(mSourceId, mRouteProvider, mSinks);
        }
    }

    private static class MediaUrlResolverDelegate implements MediaUrlResolver.Delegate {
        private final WeakReference<FlingingMediaRouteProvider> mProvider;
        private final FlingingMediaSource mSource;
        private MediaUrlResolver mResolver;

        MediaUrlResolverDelegate(FlingingMediaRouteProvider provider, FlingingMediaSource source) {
            mProvider = new WeakReference<FlingingMediaRouteProvider>(provider);
            mSource = source;
        }

        public void setResolver(MediaUrlResolver resolver) {
            mResolver = resolver;
        }

        public void cancel() {
            mProvider.clear();
            mResolver.cancel(true);
            mResolver = null;
        }

        @Override
        public Uri getUri() {
            return Uri.parse(mSource.getContentURI());
        }

        @Override
        public String getCookies() {
            // TODO(avayvod): add cookies support (see MediaResourceGetterImpl).
            return "";
        }

        @Override
        public void deliverResult(Uri uri, boolean playable) {
            FlingingMediaRouteProvider provider = mProvider.get();
            if (provider == null) return;

            if (playable) {
                provider.startDiscoveryForSource(mSource);
            } else {
                provider.onSinksReceived(mSource.getUrn(), new ArrayList<MediaSink>());
            }
        }
    }

    /**
     * @return Initialized {@link FlingingMediaRouteProvider} object or null if it's not supported.
     */
    @Nullable
    public static FlingingMediaRouteProvider create(MediaRouteManager manager) {
        MediaRouter androidMediaRouter = ChromeMediaRouter.getAndroidMediaRouter();

        return new FlingingMediaRouteProvider(androidMediaRouter, manager);
    }

    /**
     * {@link DiscoveryDelegate} implementation.
     */
    @Override
    public void onSinksReceived(String sourceId, List<MediaSink> sinks) {
        mHandler.post(new OnSinksReceivedRunnable(mManager, this, sourceId, sinks));
    }

    @Override
    public boolean supportsSource(String sourceId) {
        return FlingingMediaSource.from(sourceId) != null;
    }

    @Override
    public void startObservingMediaSinks(String sourceId) {
        // No-op if already started observing this source.
        MediaUrlResolverDelegate delegate = mUrlResolvers.get(sourceId);
        if (delegate != null) return;

        FlingingMediaSource source = FlingingMediaSource.from(sourceId);
        if (source == null) {
            // If the source is invalid, report no devices available.
            onSinksReceived(sourceId, new ArrayList<MediaSink>());
            return;
        }

        // No-op, if already monitoring the application for this source.
        DiscoveryCallback callback = mDiscoveryCallbacks.get(source.getApplicationId());
        if (callback != null && callback.hasSourceUrn(sourceId)) return;

        if (mAndroidMediaRouter == null) {
            // If the MediaRouter API is not available, report no devices so the page doesn't even
            // try to cast.
            onSinksReceived(sourceId, new ArrayList<MediaSink>());
            return;
        }

        if (source.buildRouteSelector() == null) {
            // If the application invalid, report no devices available.
            onSinksReceived(sourceId, new ArrayList<MediaSink>());
            return;
        }

        if (!source.needsToBeResolved()) {
            startDiscoveryForSource(source);
            return;
        }

        // Resolve the media element's source and check if it can be played remotely. Only then
        // start looking for the remote playback devices.
        delegate = new MediaUrlResolverDelegate(this, source);
        MediaUrlResolver resolver =
                new MediaUrlResolver(delegate, ChromeApplication.getBrowserUserAgent());
        delegate.setResolver(resolver);
        mUrlResolvers.put(sourceId, delegate);
        resolver.execute();
    }

    private void startDiscoveryForSource(FlingingMediaSource source) {
        String sourceId = source.getUrn();
        mUrlResolvers.remove(sourceId);

        String applicationId = source.getApplicationId();
        DiscoveryCallback callback = mDiscoveryCallbacks.get(applicationId);
        if (callback != null) {
            callback.addSourceUrn(sourceId);
            return;
        }

        MediaRouteSelector routeSelector = source.buildRouteSelector();
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
        MediaUrlResolverDelegate delegate = mUrlResolvers.get(sourceId);
        if (delegate != null) {
            delegate.cancel();
            mUrlResolvers.remove(sourceId);
        }

        if (mAndroidMediaRouter == null) return;

        FlingingMediaSource source = FlingingMediaSource.from(sourceId);
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

    @VisibleForTesting
    FlingingMediaRouteProvider(MediaRouter androidMediaRouter, MediaRouteManager manager) {
        mAndroidMediaRouter = androidMediaRouter;
        mManager = manager;
    }
}
