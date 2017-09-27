// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.StrictMode;
import android.support.annotation.IntDef;
import android.util.Log;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.io.BufferedReader;
import java.io.FileReader;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Exposes system related information about the current device.
 */
@JNINamespace("base::android")
public class SysUtils {
    // A device reporting strictly more total memory in megabytes cannot be considered 'low-end'.
    private static final int EXTREMELY_LOW_END_MEMORY_THRESHOLD_MB = 512;
    private static final int LOW_END_MEMORY_THRESHOLD_MB = 512;
    private static final int ANDROID_O_LOW_END_MEMORY_THRESHOLD_MB = 1024;

    private static final String TAG = "SysUtils";

    @IntDef({DeviceTier.EXTREMELY_LOW_END, DeviceTier.LOW_END, DeviceTier.UNKNOWN,
            DeviceTier.NOT_LOW_END})
    @Retention(RetentionPolicy.SOURCE)
    public @interface DeviceTier {
        int EXTREMELY_LOW_END = -2;
        int LOW_END = -1;
        int UNKNOWN = 0;
        int NOT_LOW_END = 1;
    }

    private static @DeviceTier int sDeviceTier = DeviceTier.UNKNOWN;

    private SysUtils() { }

    /**
     * Return the amount of physical memory on this device in kilobytes.
     * @return Amount of physical memory in kilobytes, or 0 if there was
     *         an error trying to access the information.
     */
    private static int amountOfPhysicalMemoryKB() {
        // Extract total memory RAM size by parsing /proc/meminfo, note that
        // this is exactly what the implementation of sysconf(_SC_PHYS_PAGES)
        // does. However, it can't be called because this method must be
        // usable before any native code is loaded.

        // An alternative is to use ActivityManager.getMemoryInfo(), but this
        // requires a valid ActivityManager handle, which can only come from
        // a valid Context object, which itself cannot be retrieved
        // during early startup, where this method is called. And making it
        // an explicit parameter here makes all call paths _much_ more
        // complicated.

        Pattern pattern = Pattern.compile("^MemTotal:\\s+([0-9]+) kB$");
        // Synchronously reading files in /proc in the UI thread is safe.
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
        try {
            FileReader fileReader = new FileReader("/proc/meminfo");
            try {
                BufferedReader reader = new BufferedReader(fileReader);
                try {
                    String line;
                    for (;;) {
                        line = reader.readLine();
                        if (line == null) {
                            Log.w(TAG, "/proc/meminfo lacks a MemTotal entry?");
                            break;
                        }
                        Matcher m = pattern.matcher(line);
                        if (!m.find()) continue;

                        int totalMemoryKB = Integer.parseInt(m.group(1));
                        // Sanity check.
                        if (totalMemoryKB <= 1024) {
                            Log.w(TAG, "Invalid /proc/meminfo total size in kB: " + m.group(1));
                            break;
                        }

                        return totalMemoryKB;
                    }

                } finally {
                    reader.close();
                }
            } finally {
                fileReader.close();
            }
        } catch (Exception e) {
            Log.w(TAG, "Cannot get total physical size from /proc/meminfo", e);
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }

        return 0;
    }

    /**
     * @return Whether or not this device should be considered a low end device.
     */
    @CalledByNative
    public static boolean isLowEndDevice() {
        return getDeviceTier() <= DeviceTier.LOW_END;
    }

    /**
     * @return Whether or not this device should be considered an extremely low end device.
     */
    public static boolean isExtremelyLowEndDevice() {
        return getDeviceTier() <= DeviceTier.EXTREMELY_LOW_END;
    }

    /**
     * @return Whether or not the system has low available memory.
     */
    @CalledByNative
    private static boolean isCurrentlyLowMemory() {
        ActivityManager am =
                (ActivityManager) ContextUtils.getApplicationContext().getSystemService(
                        Context.ACTIVITY_SERVICE);
        ActivityManager.MemoryInfo info = new ActivityManager.MemoryInfo();
        am.getMemoryInfo(info);
        return info.lowMemory;
    }

    /**
     * Resets the cached value, if any.
     */
    @VisibleForTesting
    public static void reset() {
        sDeviceTier = DeviceTier.UNKNOWN;
    }

    public static boolean hasCamera(final Context context) {
        final PackageManager pm = context.getPackageManager();
        // JellyBean support.
        boolean hasCamera = pm.hasSystemFeature(PackageManager.FEATURE_CAMERA);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            hasCamera |= pm.hasSystemFeature(PackageManager.FEATURE_CAMERA_ANY);
        }
        return hasCamera;
    }

    private static @DeviceTier int getDeviceTier() {
        if (sDeviceTier == DeviceTier.UNKNOWN) {
            sDeviceTier = detectDeviceTier();
        }
        return sDeviceTier;
    }

    private static int detectDeviceTier() {
        assert CommandLine.isInitialized();

        if (CommandLine.getInstance().hasSwitch(BaseSwitches.ENABLE_LOW_END_DEVICE_MODE)) {
            return DeviceTier.LOW_END;
        }
        if (CommandLine.getInstance().hasSwitch(BaseSwitches.DISABLE_LOW_END_DEVICE_MODE)) {
            return DeviceTier.NOT_LOW_END;
        }

        int ramSizeMB = amountOfPhysicalMemoryKB() / 1024;
        if (ramSizeMB <= 0) return DeviceTier.NOT_LOW_END;

        if (ramSizeMB <= EXTREMELY_LOW_END_MEMORY_THRESHOLD_MB) return DeviceTier.EXTREMELY_LOW_END;

        if (BuildInfo.isAtLeastO()) {
            if (ramSizeMB <= ANDROID_O_LOW_END_MEMORY_THRESHOLD_MB) return DeviceTier.LOW_END;
        } else {
            if (ramSizeMB <= LOW_END_MEMORY_THRESHOLD_MB) return DeviceTier.LOW_END;
        }

        return DeviceTier.NOT_LOW_END;
    }

    /**
     * Creates a new trace event to log the number of minor / major page faults, if tracing is
     * enabled.
     */
    public static void logPageFaultCountToTracing() {
        nativeLogPageFaultCountToTracing();
    }

    private static native void nativeLogPageFaultCountToTracing();
}
