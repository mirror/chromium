// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.os.Looper;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.banners.AppData;
import org.chromium.chrome.browser.banners.InstallerDelegate;
import org.chromium.chrome.browser.tab.Tab;

/**
 * Handles the promotion and installation of an app specified by the current web page. This object
 * is created by and owned by the native AppBannerInfoBarDelegateAndroid.
 */
@JNINamespace("banners")
public class AppBannerInfoBarDelegateAndroid implements InstallerDelegate.Observer {
    /** Weak pointer to the native AppBannerInfoBarDelegateAndroid. */
    private long mNativePointer;

    /** Monitors an installation in progress. */
    private InstallerDelegate mInstallerDelegate;

    /** WebAPK installation is controlled from C++, so this flag is set from native code. */
    private boolean mIsInstallingWebApk;

    /** The package name of the WebAPK. */
    private String mWebApkPackage;

    private AppBannerInfoBarDelegateAndroid(long nativePtr) {
        mNativePointer = nativePtr;
        mInstallerDelegate = new InstallerDelegate(Looper.getMainLooper(), this);
    }

    @CalledByNative
    private void destroy() {
        mInstallerDelegate.destroy();
        mInstallerDelegate = null;
        mNativePointer = 0;
    }

    @CalledByNative
    private boolean installOrOpenNativeApp(Tab tab, AppData appData, String referrer) {
        return mInstallerDelegate.installOrOpenNativeApp(tab, appData, referrer);
    }

    @Override
    public void onInstallIntentCompleted(InstallerDelegate delegate, boolean isInstalling) {
        if (mInstallerDelegate != delegate) return;
        nativeOnInstallIntentReturned(mNativePointer, isInstalling);
    }

    @Override
    public void onInstallFinished(InstallerDelegate delegate, boolean success) {
        if (mInstallerDelegate != delegate) return;
        nativeOnInstallFinished(mNativePointer, success);
    }

    @Override
    public void onApplicationStateChanged(InstallerDelegate delegate, int newState) {
        if (mInstallerDelegate != delegate) return;
        nativeUpdateInstallState(mNativePointer);
    }

    @CalledByNative
    private void openWebApk() {
        mInstallerDelegate.openApp(ContextUtils.getApplicationContext(), mWebApkPackage);
    }

    @CalledByNative
    private void showAppDetails(Tab tab, AppData appData) {
        tab.getWindowAndroid().showIntent(appData.detailsIntent(), null, null);
    }

    @CalledByNative
    private int determineInstallState(AppData data) {
        assert mInstallerDelegate != null;

        if (mInstallerDelegate.isRunning() || mIsInstallingWebApk) {
            return AppBannerInfoBarAndroid.INSTALL_STATE_INSTALLING;
        }

        String packageName = (data != null) ? data.packageName() : mWebApkPackage;
        if (mInstallerDelegate.isInstalled(ContextUtils.getApplicationContext(), packageName)) {
            return AppBannerInfoBarAndroid.INSTALL_STATE_INSTALLED;
        }

        return AppBannerInfoBarAndroid.INSTALL_STATE_NOT_INSTALLED;
    }

    @CalledByNative
    /** Set the flag of whether the installation process has been started for the WebAPK. */
    private void setWebApkInstallingState(boolean isInstalling) {
        mIsInstallingWebApk = isInstalling;
    }

    @CalledByNative
    /** Sets the WebAPK package name. */
    private void setWebApkPackageName(String webApkPackage) {
        mWebApkPackage = webApkPackage;
    }

    @CalledByNative
    private static AppBannerInfoBarDelegateAndroid create(long nativePtr) {
        return new AppBannerInfoBarDelegateAndroid(nativePtr);
    }

    private native void nativeOnInstallIntentReturned(
            long nativeAppBannerInfoBarDelegateAndroid, boolean isInstalling);
    private native void nativeOnInstallFinished(
            long nativeAppBannerInfoBarDelegateAndroid, boolean success);
    private native void nativeUpdateInstallState(long nativeAppBannerInfoBarDelegateAndroid);
}
