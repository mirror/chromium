// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.preferences.PrefServiceBridge.PrefName;

import java.util.HashMap;
import java.util.Map;

/**
 * This class is Java implementation of native PrefChangeRegistrar. It receives notification for
 * changes of one or more preferences from native PrefService.
 *
 * Note that {@link PrefChangeRegistrar#destroy()} should be called to destroy the native
 * PrefChangeRegistrar.
 */
public class PrefChangeRegistrar {
    /**
     * Interface for callback when registered preference is changed.
     */
    public interface PrefChangeRegistrarObserver { void onPreferenceChange(); }

    private static final String TAG = "PrefChangeRegistrar";

    /** Mapping preference name and correspond observer. **/
    private Map<String, PrefChangeRegistrarObserver> mObservers;
    /** Native pointer for PrefChangeRegistrarAndroid. **/
    private long mNativeRegistrar;

    public PrefChangeRegistrar(boolean isIncognito) {
        mObservers = new HashMap<>();
        mNativeRegistrar = nativeInit(isIncognito);
    }

    /**
     * Add observer that receives notification on change to the specified preference.
     * @param preference The preference to be observed.
     * @param observer The observer that receives notification on change.
     */
    public void addObserver(@PrefName String preference, PrefChangeRegistrarObserver observer) {
        mObservers.put(preference, observer);
        nativeAdd(mNativeRegistrar, preference);
    }

    /**
     * Destroy native PrefChangeRegistrar.
     */
    public void destroy() {
        if (mNativeRegistrar != 0) {
            nativeDestroy(mNativeRegistrar);
        }
        mNativeRegistrar = 0;
    }

    @CalledByNative
    void onPreferenceChange(String preference) {
        if (!mObservers.containsKey(preference)) {
            Log.w(TAG, "Notification from unregistered preference changes: " + preference);
            return;
        }
        mObservers.get(preference).onPreferenceChange();
    }

    @Override
    protected void finalize() throws Throwable {
        destroy();
        super.finalize();
    }

    private native long nativeInit(boolean isIncognito);
    private native void nativeAdd(long nativePrefChangeRegistrarAndroid, String preference);
    private native void nativeDestroy(long nativePrefChangeRegistrarAndroid);
}
