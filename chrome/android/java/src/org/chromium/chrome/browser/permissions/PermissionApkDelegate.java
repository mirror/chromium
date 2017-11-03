// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.permissions;

import android.content.pm.PackageManager;

import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.webapk.lib.client.WebApkServiceImplClient;
import org.chromium.webapk.lib.runtime_library.IPermissionRequestCallback;

/**
 * Created by hanxi on 10/2/17.
 */
public class PermissionApkDelegate {
    private long mNativePtr;
    private String mPackageName;

    @CalledByNative
    public static PermissionApkDelegate create(long nativePtr, String packageName) {
        return new PermissionApkDelegate(nativePtr, packageName);
    }

    private PermissionApkDelegate(long nativePtr, String packageName) {
        mNativePtr = nativePtr;
        mPackageName = packageName;
    }

    @CalledByNative
    private void requestPermission(String[] permissions) {
        IPermissionRequestCallback callback = new IPermissionRequestCallback.Stub() {
            @Override
            public void onRequestPermissionsResult(final int[] grantResults) {
                onGetRequestPermissionsResult(grantResults);
            }
        };
        WebApkServiceImplClient.getInstance().requestPermission(
                ContextUtils.getApplicationContext(), mPackageName, callback, permissions);
    }

    public void onGetRequestPermissionsResult(final int[] grantResults) {
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                for (int result : grantResults) {
                    if (result == PackageManager.PERMISSION_DENIED) {
                        nativeOnRequestPermissionsResult(mNativePtr, false);
                        return;
                    }
                }
                nativeOnRequestPermissionsResult(mNativePtr, true);
            }
        });
    }

    private native void nativeOnRequestPermissionsResult(
            long nativePermissionApkDelegateAndroid, boolean areAllPermissionsGranted);
}
