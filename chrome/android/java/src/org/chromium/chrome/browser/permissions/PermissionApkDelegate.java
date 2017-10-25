// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.permissions;

import android.content.pm.PackageManager;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.webapps.WebApkServiceClient;
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
        Log.e("ABCD", "request permissions in PermissionApkDelegate!!!");
        IPermissionRequestCallback callback = new IPermissionRequestCallback.Stub() {
            @Override
            public void onRequestPermissionsResult(final int[] grantResults) {
                Log.w("ABCD", "PermissionApkDelegate onRequestPermissionsResult!!!");
                /*Handler handler = new Handler(Looper.myLooper());
                handler.post(new Runnable() {
                    public void run() {
                        Log.w("ABCD", "PermissionApkDelegate post runable!!!");
                        onGetRequestPermissionsResult(grantResults);
                    }
                });
*/
                onGetRequestPermissionsResult(grantResults);
            }
        };
        WebApkServiceClient.getInstance().requestPermission(mPackageName, callback, permissions);
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
