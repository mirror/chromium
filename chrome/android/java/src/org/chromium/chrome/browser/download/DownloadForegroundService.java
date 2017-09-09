// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.app.Notification;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.support.annotation.Nullable;

import org.chromium.base.ObserverList;
import org.chromium.chrome.browser.AppHooks;

/**
 * Keep-alive foreground service for downloads.
 */
public class DownloadForegroundService extends Service {
    private final IBinder mBinder = new LocalBinder();
    private final ObserverList<Observer> mObservers = new ObserverList<>();

    /**
     * An Observer interfaces that allows other classes to know when this service is shutting down.
     */
    public interface Observer {
        void onForegroundServiceRestarted();
        void onForegroundServiceTaskRemoved();
        void onForegroundServiceDestroyed();
    }

    public void addObserver(Observer observer) {
        mObservers.addObserver(observer);
    }

    public void removeObserver(Observer observer) {
        mObservers.removeObserver(observer);
    }

    /**
     * Start the foreground service with this given context.
     * @param context The context used to start service.
     */
    public static void startDownloadForegroundService(Context context) {
        AppHooks.get().startForegroundService(new Intent(context, DownloadForegroundService.class));
    }

    /**
     * Update the foreground service to be pinned to a different notification.
     * @param notificationId The id of the new notification to be pinned to.
     * @param notification The new notification to be pinned to.
     */
    public void startOrUpdateForegroundService(int notificationId, Notification notification) {
        startForeground(notificationId, notification);
    }

    /**
     * Stop the foreground service that is running.
     */
    public void stopDownloadForegroundService(boolean isCancelled) {
        stopForeground(isCancelled /* kill notification if cancelled */);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // In the case the service was restarted when the intent is null.
        if (intent == null) {
            // TODO(jming): make this more generalized eventually (probably don't need observer?)
            DownloadNotificationService2.getInstance().onForegroundServiceRestarted();
            for (Observer observer : mObservers) observer.onForegroundServiceRestarted();
        }

        // This should restart service after Chrome gets killed (except for Android 4.4.2).
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        for (Observer observer : mObservers) observer.onForegroundServiceDestroyed();
        super.onDestroy();
    }

    @Override
    public void onTaskRemoved(Intent rootIntent) {
        for (Observer observer : mObservers) observer.onForegroundServiceTaskRemoved();
        super.onTaskRemoved(rootIntent);
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    /**
     * Class for clients to access.
     */
    public class LocalBinder extends Binder {
        DownloadForegroundService getService() {
            return DownloadForegroundService.this;
        }
    }
}
