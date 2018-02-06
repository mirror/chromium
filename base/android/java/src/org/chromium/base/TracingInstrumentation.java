// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Application;
import android.app.Instrumentation;
import android.app.UiAutomation;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Looper;
import android.os.PersistableBundle;
import android.os.TestLooperManager;
import android.view.KeyEvent;
import android.view.MotionEvent;

import java.lang.reflect.Field;
import java.lang.reflect.Method;

public class TracingInstrumentation extends Instrumentation {
    private Instrumentation mImpl;

    private TracingInstrumentation(Instrumentation impl) {
        mImpl = impl;
    }

    public static void install() {
        try (TraceEvent te = TraceEvent.scoped("TracingInstrumentation.install")) {
            Class<?> activityThreadClass = Class.forName("android.app.ActivityThread");
            Method currentActivityThreadMethod = activityThreadClass.getDeclaredMethod("currentActivityThread");
            currentActivityThreadMethod.setAccessible(true);
            Object activityThread = currentActivityThreadMethod.invoke(null);
            Field instrumentationField = activityThreadClass.getDeclaredField("mInstrumentation");
            instrumentationField.setAccessible(true);
            Instrumentation instrumentation = (Instrumentation)instrumentationField.get(activityThread);
            instrumentation = new TracingInstrumentation(instrumentation);
            instrumentationField.set(activityThread, instrumentation);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static TraceEvent scopedEvent(String name) {
        return TraceEvent.scoped("Instrumentation." + name);
    }

    @Override
    public void onCreate(Bundle arguments) {
        try (TraceEvent te = scopedEvent("onCreate")) {
            mImpl.onCreate(arguments);
        }
    }

    @Override
    public void start() {
        try (TraceEvent te = scopedEvent("start")) {
            mImpl.start();
        }
    }

    @Override
    public void onStart() {
        try (TraceEvent te = scopedEvent("onStart")) {
            mImpl.onStart();
        }
    }

    @Override
    public boolean onException(Object obj, Throwable e) {
        try (TraceEvent te = scopedEvent("onException")) {
            return mImpl.onException(obj, e);
        }
    }

    @Override
    public void sendStatus(int resultCode, Bundle results) {
        try (TraceEvent te = scopedEvent("sendStatus")) {
            mImpl.sendStatus(resultCode, results);
        }
    }

    @SuppressLint("NewApi")
    @Override
    public void addResults(Bundle results) {
        try (TraceEvent te = scopedEvent("addResults")) {
            mImpl.addResults(results);
        }
    }

    @Override
    public void finish(int resultCode, Bundle results) {
        try (TraceEvent te = scopedEvent("finish")) {
            mImpl.finish(resultCode, results);
        }
    }

    @Override
    public void setAutomaticPerformanceSnapshots() {
        try (TraceEvent te = scopedEvent("setAutomaticPerformanceSnapshots")) {
            mImpl.setAutomaticPerformanceSnapshots();
        }
    }

    @Override
    public void startPerformanceSnapshot() {
        try (TraceEvent te = scopedEvent("startPerformanceSnapshot")) {
            mImpl.startPerformanceSnapshot();
        }
    }

    @Override
    public void endPerformanceSnapshot() {
        try (TraceEvent te = scopedEvent("endPerformanceSnapshot")) {
            mImpl.endPerformanceSnapshot();
        }
    }

    @Override
    public void onDestroy() {
        try (TraceEvent te = scopedEvent("onDestroy")) {
            mImpl.onDestroy();
        }
    }

    @Override
    public void waitForIdle(Runnable recipient) {
        try (TraceEvent te = scopedEvent("waitForIdle")) {
            mImpl.waitForIdle(recipient);
        }
    }

    @Override
    public void waitForIdleSync() {
        try (TraceEvent te = scopedEvent("waitForIdleSync")) {
            mImpl.waitForIdleSync();
        }
    }

    @Override
    public void runOnMainSync(Runnable runner) {
        try (TraceEvent te = scopedEvent("runOnMainSync")) {
            mImpl.runOnMainSync(runner);
        }
    }

    @Override
    public Activity startActivitySync(Intent intent) {
        try (TraceEvent te = scopedEvent("startActivitySync")) {
            return mImpl.startActivitySync(intent);
        }
    }

    @Override
    public Application newApplication(ClassLoader cl, String className, Context context) throws InstantiationException, IllegalAccessException, ClassNotFoundException {
        try (TraceEvent te = scopedEvent("newApplication")) {
            return mImpl.newApplication(cl, className, context);
        }
    }

    @Override
    public void callApplicationOnCreate(Application app) {
        try (TraceEvent te = scopedEvent("callApplicationOnCreate")) {
            mImpl.callApplicationOnCreate(app);
        }
    }

    @Override
    public Activity newActivity(Class<?> clazz, Context context, IBinder token, Application application, Intent intent, ActivityInfo info, CharSequence title, Activity parent, String id, Object lastNonConfigurationInstance) throws InstantiationException, IllegalAccessException {
        try (TraceEvent te = scopedEvent("newActivity")) {
            try (TraceEvent te2 = TraceEvent.scoped("Class.newInstance: " + clazz.getName())) {
                clazz.newInstance();
            }
            return mImpl.newActivity(clazz, context, token, application, intent, info, title, parent, id, lastNonConfigurationInstance);
        }
    }

    @Override
    public Activity newActivity(ClassLoader cl, String className, Intent intent) throws InstantiationException, IllegalAccessException, ClassNotFoundException {
        try (TraceEvent te = scopedEvent("newActivity")) {
            Class<?> clazz;
            try (TraceEvent te2 = TraceEvent.scoped("ClassLoader.loadClass: " + className)) {
                clazz = cl.loadClass(className);
            }
            try (TraceEvent te2 = TraceEvent.scoped("Class.newInstance")) {
                clazz.newInstance();
            }
            return mImpl.newActivity(cl, className, intent);
        }
    }

    @Override
    public void callActivityOnCreate(Activity activity, Bundle icicle) {
        try (TraceEvent te = scopedEvent("callActivityOnCreate")) {
            mImpl.callActivityOnCreate(activity, icicle);
        }
    }

    @SuppressLint("NewApi")
    @Override
    public void callActivityOnCreate(Activity activity, Bundle icicle, PersistableBundle persistentState) {
        try (TraceEvent te = scopedEvent("callActivityOnCreate")) {
            mImpl.callActivityOnCreate(activity, icicle, persistentState);
        }
    }

    @Override
    public void callActivityOnDestroy(Activity activity) {
        try (TraceEvent te = scopedEvent("callActivityOnDestroy")) {
            mImpl.callActivityOnDestroy(activity);
        }
    }

    @Override
    public void callActivityOnRestoreInstanceState(Activity activity, Bundle savedInstanceState) {
        try (TraceEvent te = scopedEvent("callActivityOnRestoreInstanceState")) {
            mImpl.callActivityOnRestoreInstanceState(activity, savedInstanceState);
        }
    }

    @SuppressLint("NewApi")
    @Override
    public void callActivityOnRestoreInstanceState(Activity activity, Bundle savedInstanceState, PersistableBundle persistentState) {
        try (TraceEvent te = scopedEvent("callActivityOnRestoreInstanceState")) {
            mImpl.callActivityOnRestoreInstanceState(activity, savedInstanceState, persistentState);
        }
    }

    @Override
    public void callActivityOnPostCreate(Activity activity, Bundle icicle) {
        try (TraceEvent te = scopedEvent("callActivityOnPostCreate")) {
            mImpl.callActivityOnPostCreate(activity, icicle);
        }
    }

    @SuppressLint("NewApi")
    @Override
    public void callActivityOnPostCreate(Activity activity, Bundle icicle, PersistableBundle persistentState) {
        try (TraceEvent te = scopedEvent("callActivityOnPostCreate")) {
            mImpl.callActivityOnPostCreate(activity, icicle, persistentState);
        }
    }

    @Override
    public void callActivityOnNewIntent(Activity activity, Intent intent) {
        try (TraceEvent te = scopedEvent("callActivityOnNewIntent")) {
            mImpl.callActivityOnNewIntent(activity, intent);
        }
    }

    @Override
    public void callActivityOnStart(Activity activity) {
        try (TraceEvent te = scopedEvent("callActivityOnStart")) {
            mImpl.callActivityOnStart(activity);
        }
    }

    @Override
    public void callActivityOnRestart(Activity activity) {
        try (TraceEvent te = scopedEvent("callActivityOnRestart")) {
            mImpl.callActivityOnRestart(activity);
        }
    }

    @Override
    public void callActivityOnResume(Activity activity) {
        try (TraceEvent te = scopedEvent("callActivityOnResume")) {
            mImpl.callActivityOnResume(activity);
        }
    }

    @Override
    public void callActivityOnStop(Activity activity) {
        try (TraceEvent te = scopedEvent("callActivityOnStop")) {
            mImpl.callActivityOnStop(activity);
        }
    }

    @Override
    public void callActivityOnSaveInstanceState(Activity activity, Bundle outState) {
        try (TraceEvent te = scopedEvent("callActivityOnSaveInstanceState")) {
            mImpl.callActivityOnSaveInstanceState(activity, outState);
        }
    }

    @SuppressLint("NewApi")
    @Override
    public void callActivityOnSaveInstanceState(Activity activity, Bundle outState, PersistableBundle outPersistentState) {
        try (TraceEvent te = scopedEvent("callActivityOnSaveInstanceState")) {
            mImpl.callActivityOnSaveInstanceState(activity, outState, outPersistentState);
        }
    }

    @Override
    public void callActivityOnPause(Activity activity) {
        try (TraceEvent te = scopedEvent("callActivityOnPause")) {
            mImpl.callActivityOnPause(activity);
        }
    }

    @Override
    public void callActivityOnUserLeaving(Activity activity) {
        try (TraceEvent te = scopedEvent("callActivityOnUserLeaving")) {
            mImpl.callActivityOnUserLeaving(activity);
        }
    }

    // Not interesting


    @Override
    public Context getContext() {
        return mImpl.getContext();
    }

    @Override
    public ComponentName getComponentName() {
        return mImpl.getComponentName();
    }

    @Override
    public Context getTargetContext() {
        return mImpl.getTargetContext();
    }

    @SuppressLint("NewApi")
    @Override
    public String getProcessName() {
        return mImpl.getProcessName();
    }

    @Override
    public boolean isProfiling() {
        return mImpl.isProfiling();
    }

    @Override
    public void startProfiling() {
        mImpl.startProfiling();
    }

    @Override
    public void stopProfiling() {
        mImpl.stopProfiling();
    }

    @Override
    public void setInTouchMode(boolean inTouch) {
        mImpl.setInTouchMode(inTouch);
    }

    @Override
    public void startAllocCounting() {
        mImpl.startAllocCounting();
    }

    @Override
    public void stopAllocCounting() {
        mImpl.stopAllocCounting();
    }

    @Override
    public Bundle getAllocCounts() {
        return mImpl.getAllocCounts();
    }

    @Override
    public Bundle getBinderCounts() {
        return mImpl.getBinderCounts();
    }

    @SuppressLint("NewApi")
    @Override
    public UiAutomation getUiAutomation() {
        return mImpl.getUiAutomation();
    }

    @SuppressLint("NewApi")
    @Override
    public UiAutomation getUiAutomation(int flags) {
        return mImpl.getUiAutomation(flags);
    }

    @SuppressLint("NewApi")
    @Override
    public TestLooperManager acquireLooperManager(Looper looper) {
        return mImpl.acquireLooperManager(looper);
    }

    @Override
    public void sendStringSync(String text) {
        mImpl.sendStringSync(text);
    }

    @Override
    public void sendKeySync(KeyEvent event) {
        mImpl.sendKeySync(event);
    }

    @Override
    public void sendKeyDownUpSync(int key) {
        mImpl.sendKeyDownUpSync(key);
    }

    @Override
    public void sendCharacterSync(int keyCode) {
        mImpl.sendCharacterSync(keyCode);
    }

    @Override
    public void sendPointerSync(MotionEvent event) {
        mImpl.sendPointerSync(event);
    }

    @Override
    public void sendTrackballEventSync(MotionEvent event) {
        mImpl.sendTrackballEventSync(event);
    }

    @Override
    public void addMonitor(ActivityMonitor monitor) {
        mImpl.addMonitor(monitor);
    }

    @Override
    public ActivityMonitor addMonitor(IntentFilter filter, ActivityResult result, boolean block) {
        return mImpl.addMonitor(filter, result, block);
    }

    @Override
    public ActivityMonitor addMonitor(String cls, ActivityResult result, boolean block) {
        return mImpl.addMonitor(cls, result, block);
    }

    @Override
    public boolean checkMonitorHit(ActivityMonitor monitor, int minHits) {
        return mImpl.checkMonitorHit(monitor, minHits);
    }

    @Override
    public Activity waitForMonitor(ActivityMonitor monitor) {
        return mImpl.waitForMonitor(monitor);
    }

    @Override
    public Activity waitForMonitorWithTimeout(ActivityMonitor monitor, long timeOut) {
        return mImpl.waitForMonitorWithTimeout(monitor, timeOut);
    }

    @Override
    public void removeMonitor(ActivityMonitor monitor) {
        mImpl.removeMonitor(monitor);
    }

    @Override
    public boolean invokeMenuActionSync(Activity targetActivity, int id, int flag) {
        return mImpl.invokeMenuActionSync(targetActivity, id, flag);
    }

    @Override
    public boolean invokeContextMenuAction(Activity targetActivity, int id, int flag) {
        return mImpl.invokeContextMenuAction(targetActivity, id, flag);
    }
}

