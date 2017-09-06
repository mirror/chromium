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
    private File mMinidumpFile = null;
    private FileOutputStream mMinidumpFileStream = null;

    private String mLocalId = UUID.randomUUID().toString().replace("-", "").substring(0, 16);
    private String mBoundary = "----------" + UUID.randomUUID();

    private static final String FILE_PREFIX = "chromium-renderer-minidump-";
    private static final String FILE_SUFFIX = ".dmp.try0";
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

    public PureJavaExceptionReporter() {
        try {
            String miniDumpFileName = FILE_PREFIX + mLocalId + FILE_SUFFIX;
            mMinidumpFile =
                    new File(ContextUtils.getApplicationContext().getCacheDir(), miniDumpFileName);
            mMinidumpFileStream = new FileOutputStream(mMinidumpFile);
        } catch (Throwable e) {
            mMinidumpFile = null;
            mMinidumpFileStream = null;
        }
    }

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

    private void createReport(Throwable javaException) {
        addPairedString(PRODUCT, "Chrome_Android");
        addPairedString(DEVICE, BuildInfo.getDevice());
        addPairedString(VERSION, ChromeVersionInfo.getProductVersion());

        String channel = getChannel();
        if (channel != null) addPairedString(CHANNEL, channel);

        addPairedString(ANDROID_BUILD_ID, BuildInfo.getBuildId());
        addPairedString(ANDROID_BUILD_FP, BuildInfo.getAndroidBuildFingerprint());
        addPairedString(MODEL, BuildInfo.getModel());
        addPairedString(BRAND, BuildInfo.getBrand());
        addPairedString(GMS_CORE_VERSION, BuildInfo.getGMSVersionCode());
        addPairedString(INSTALLER_PACKAGE_NAME, BuildInfo.getInstallerPackageName());
        addPairedString(ABI_NAME, BuildInfo.getAbiName());
        addPairedString(PACKAGE, BuildInfo.getPackageName());
        addPairedString(EXCEPTION_INFO, Log.getStackTraceString(javaException));
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
        // Force to upload minidump immediately, as upload scheduler may not be ready at this point.
        MinidumpUploadService.tryUploadCrashDump(mMinidumpFile);
    }
}
