// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.crash;

import android.os.AsyncTask;
import android.util.Log;

import org.chromium.base.BuildInfo;
import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.MainDex;
import org.chromium.chrome.browser.ChromeVersionInfo;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.UUID;

/**
 * Creates a crash report and uploads it to crash server if there is a Java exception.
 *
 * This class is written in pure Java, so it can handle exception happens before native is loaded.
 */
@MainDex
public class PureJavaExceptionReporter {
    private static final String CRASH_DUMP_DIR = "Crash Reports";
    private static final String FILE_PREFIX = "chromium-browser-minidump-";
    private static final String FILE_SUFFIX = ".dmp";
    private static final String RN = "\r\n";
    private static final String FORM_DATA_MESSAGE = "Content-Disposition: form-data; name=\"";

    // report fields
    private static final String CHANNEL = "Channel";
    private static final String VERSION = "ver";
    private static final String PRODUCT = "prod";
    private static final String ANDROID_BUILD_ID = "android_build_id";
    private static final String ANDROID_BUILD_FP = "android_build_fp";
    private static final String DEVICE = "device";
    private static final String GMS_CORE_VERSION = "gms_core_version";
    private static final String INSTALLER_PACKAGE_NAME = "installer_package_name";
    private static final String ABI_NAME = "abi_name";
    private static final String PACKAGE = "package";
    private static final String MODEL = "model";
    private static final String BRAND = "brand";
    private static final String EXCEPTION_INFO = "exception_info";
    private static final String PROCESS_TYPE = "ptype";
    private static final String EARLY_JAVA_EXCEPTION = "early_java_exception";

    private File mMinidumpFile = null;
    private FileOutputStream mMinidumpFileStream = null;
    private final String mLocalId = UUID.randomUUID().toString().replace("-", "").substring(0, 16);
    private final String mBoundary = "----------" + UUID.randomUUID();

    /**
     * Report and upload the device info and stack trace as if it was a crash.
     *
     * @param javaException The exception to report.
     */
    public void createAndUploadReport(Throwable javaException) {
        new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... params) {
                createReport(javaException);
                flushToFile();
                uploadReport();
                return null;
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private void addPairedString(String messageType, String messageData) {
        addString("--" + mBoundary + RN);
        addString(FORM_DATA_MESSAGE + messageType + "\"");
        addString(RN + RN + messageData + RN);
    }

    private void addString(String s) {
        try {
            mMinidumpFileStream.write(s.getBytes());
        } catch (IOException e) {
            // Nothing we can do here.
        }
    }

    private void initMinidumpFile() {
        try {
            String minidumpFileName = FILE_PREFIX + mLocalId + FILE_SUFFIX;
            mMinidumpFile = new File(
                    new File(ContextUtils.getApplicationContext().getCacheDir(), CRASH_DUMP_DIR),
                    minidumpFileName);
            mMinidumpFileStream = new FileOutputStream(mMinidumpFile);
        } catch (Throwable e) {
            mMinidumpFile = null;
            mMinidumpFileStream = null;
        }
    }

    private void createReport(Throwable javaException) {
        initMinidumpFile();

        String[] allInfo = BuildInfo.getAll();
        addPairedString(PRODUCT, "Chrome_Android");
        addPairedString(PROCESS_TYPE, "browser");
        addPairedString(DEVICE, allInfo[1]);
        addPairedString(VERSION, ChromeVersionInfo.getProductVersion());

        String channel = getChannel();
        if (channel != null) addPairedString(CHANNEL, channel);

        addPairedString(ANDROID_BUILD_ID, allInfo[2]);
        addPairedString(MODEL, allInfo[4]);
        addPairedString(BRAND, allInfo[0]);
        addPairedString(ANDROID_BUILD_FP, allInfo[11]);
        addPairedString(GMS_CORE_VERSION, allInfo[12]);
        addPairedString(INSTALLER_PACKAGE_NAME, allInfo[13]);
        addPairedString(ABI_NAME, allInfo[14]);
        addPairedString(PACKAGE, BuildInfo.getPackageName());
        addPairedString(EXCEPTION_INFO, Log.getStackTraceString(javaException));
        addPairedString(EARLY_JAVA_EXCEPTION, "true");
        addString("--" + mBoundary + RN);
    }

    private void flushToFile() {
        if (mMinidumpFileStream != null) {
            try {
                mMinidumpFileStream.flush();
                mMinidumpFileStream.close();
            } catch (Throwable e) {
                mMinidumpFileStream = null;
                mMinidumpFile = null;
            }
        }
    }

    private static String getChannel() {
        if (ChromeVersionInfo.isCanaryBuild()) {
            return "canary";
        }
        if (ChromeVersionInfo.isDevBuild()) {
            return "dev";
        }
        if (ChromeVersionInfo.isBetaBuild()) {
            return "beta";
        }
        if (ChromeVersionInfo.isStableBuild()) {
            return "stable";
        }
        return null;
    }

    private void uploadReport() {
        if (mMinidumpFile == null) {
            return;
        }
        MinidumpUploadService.tryUploadCrashDump(mMinidumpFile);
    }
}
