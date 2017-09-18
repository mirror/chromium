// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.download;

import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;

import java.util.HashSet;
import java.util.Set;

import javax.annotation.Nullable;

/**
 * A class that handles logic related to observers that are waiting to see when the
 * DownloadsForegroundService is shutting down or starting back up.
 */
public final class DownloadForegroundServiceObservers {
    private static final String TAG = "DownloadFgServiceObs";
    private static final String KEY_FOREGROUND_SERVICE_OBSERVERS = "ForegroundServiceObservers";

    /**
     * An Observer interfaces that allows other classes to know when this service is shutting down.
     */
    public interface Observer {
        /**
         * Called when the foreground service was automatically restarted because of START_STICKY.
         */
        void onForegroundServiceRestarted();

        /**
         * Called when any task (service or activity) is removed from the service's application.
         */
        void onForegroundServiceTaskRemoved();

        /**
         * Called when the foreground service is destroyed.
         */
        void onForegroundServiceDestroyed();
    }

    /**
     * Adds an observer, which will be notified when this service attempts to start stopping itself.
     * @param observer to be added to the list of observers in SharedPrefs.
     */
    public static void addObserver(String observer) {
        SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
        Set<String> observers = getAllObservers();

        if (observers.contains(observer)) return;

        // Set returned from getStringSet() should not be modified.
        observers = new HashSet<>(observers);
        observers.add(observer);
        prefs.edit().putStringSet(KEY_FOREGROUND_SERVICE_OBSERVERS, observers).apply();
    }

    /**
     * Removes observer, which will no longer be notified when this class starts stopping itself.
     * @param observer to be removed from the list of observers in SharedPrefs.
     */
    public static void removeObserver(String observer) {
        SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
        Set<String> observers = getAllObservers();

        String observerToRemove = null;
        for (String observerEntry : observers) {
            if (observerEntry.equals(observer)) {
                observerToRemove = observerEntry;
                break;
            }
        }

        // Entry matching observer was not found.
        if (observerToRemove == null) return;

        // Set returned from getStringSet() should not be modified.
        observers = new HashSet<>(observers);
        observers.remove(observerToRemove);

        if (observers.size() == 0) {
            removeAllObservers();
            return;
        }

        prefs.edit().putStringSet(KEY_FOREGROUND_SERVICE_OBSERVERS, observers).apply();
    }

    static void alertObserversServiceRestarted() {
        Set<String> observers = getAllObservers();
        removeAllObservers();

        for (String observerString : observers) {
            DownloadForegroundServiceObservers.Observer observer =
                    DownloadForegroundServiceObservers.getObserverFromString(observerString);
            if (observer != null) observer.onForegroundServiceRestarted();
        }
    }

    static void alertObserversServiceDestroyed() {
        Set<String> observers = getAllObservers();

        for (String observerString : observers) {
            DownloadForegroundServiceObservers.Observer observer =
                    DownloadForegroundServiceObservers.getObserverFromString(observerString);
            if (observer != null) observer.onForegroundServiceDestroyed();
        }
    }

    static void alertObserversTaskRemoved() {
        Set<String> observers = getAllObservers();

        for (String observerString : observers) {
            Observer observer = getObserverFromString(observerString);
            if (observer != null) observer.onForegroundServiceTaskRemoved();
        }
    }

    private static Set<String> getAllObservers() {
        SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
        return prefs.getStringSet(KEY_FOREGROUND_SERVICE_OBSERVERS, new HashSet<String>(1));
    }

    private static void removeAllObservers() {
        ContextUtils.getAppSharedPreferences()
                .edit()
                .remove(KEY_FOREGROUND_SERVICE_OBSERVERS)
                .apply();
    }

    @Nullable
    private static Observer getObserverFromString(String observerString) {
        if (observerString == null) return null;

        Class<?> observerClass;
        try {
            observerClass = Class.forName(observerString);
        } catch (ClassNotFoundException e) {
            Log.w(TAG, "Unable to find observer class with name " + observerString);
            return null;
        }

        if (!Observer.class.isAssignableFrom(observerClass)) {
            Log.w(TAG, "Class " + observerClass + " is not an observer");
            return null;
        }

        try {
            return (Observer) observerClass.newInstance();
        } catch (InstantiationException e) {
            Log.w(TAG, "Unable to instantiate class (InstExc) " + observerClass);
            return null;
        } catch (IllegalAccessException e) {
            Log.w(TAG, "Unable to instantiate class (IllAccExc) " + observerClass);
            return null;
        }
    }
}
