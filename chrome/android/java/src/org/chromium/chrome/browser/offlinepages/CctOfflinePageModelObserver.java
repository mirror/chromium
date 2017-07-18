// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.offlinepages;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.Signature;
import android.text.TextUtils;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;

import java.util.Arrays;

/**
 * Observer for offline page model events related to CCT pages.
 *
 * Will send a broadcast intent to the originating app if a page related to it has changed.
 */
public class CctOfflinePageModelObserver extends OfflinePageBridge.OfflinePageModelObserver {
    private static final String TAG = "CctModelObserver";
    private static final String ACTION_OFFLINE_PAGES_UPDATED_SUFFIX = ".OFFLINE_PAGES_CHANGED";

    @Override
    public void offlinePageAdded(OfflinePageItem addedPage) {
        String origin = OfflinePageUtils.getOriginPackage(addedPage.getRequestOrigin());
        if (!TextUtils.isEmpty(origin)) {
            Context context = ContextUtils.getApplicationContext();
            // Check signatures.
            int[] recordedSignatures =
                    OfflinePageUtils.getOriginSignatures(addedPage.getRequestOrigin());
            int[] currentSignatures = getSignatureHashSorted(context, origin);
            if (Arrays.equals(recordedSignatures, currentSignatures)) {
                // Create broadcast if signatures match.
                Intent intent = new Intent();
                intent.setAction(ACTION_OFFLINE_PAGES_UPDATED_SUFFIX);
                intent.setPackage(origin);
                context.sendBroadcast(intent);
            } else {
                Log.w(TAG, "Signature hashes are different");
            }
        }
    }

    @Override
    public void offlinePageDeleted(DeletedPageInfo deletedPageInfo) {
        String origin = OfflinePageUtils.getOriginPackage(deletedPageInfo.getRequestOrigin());
        if (!TextUtils.isEmpty(origin)) {
            Context context = ContextUtils.getApplicationContext();
            // Check signatures.
            int[] recordedSignatures =
                    OfflinePageUtils.getOriginSignatures(deletedPageInfo.getRequestOrigin());
            int[] currentSignatures = getSignatureHashSorted(context, origin);
            if (Arrays.equals(recordedSignatures, currentSignatures)) {
                // Create broadcast if signatures match.
                Intent intent = new Intent();
                intent.setAction(ACTION_OFFLINE_PAGES_UPDATED_SUFFIX);
                intent.setPackage(origin);
                context.sendBroadcast(intent);
            } else {
                Log.w(TAG, "Signature hashes are different");
            }
        }
    }

    private int[] getSignatureHashSorted(Context context, String packageName) {
        if (TextUtils.isEmpty(packageName)) return null;
        PackageManager packageManager = context.getPackageManager();
        Signature[] signatureList;
        try {
            signatureList =
                    packageManager.getPackageInfo(packageName, PackageManager.GET_SIGNATURES)
                            .signatures;
        } catch (NameNotFoundException e) {
            return null;
        }
        int[] resultHash = new int[signatureList.length];
        for (int i = 0; i < signatureList.length; i++) {
            resultHash[i] = signatureList[i].hashCode();
        }
        Arrays.sort(resultHash);
        return resultHash;
    }
}
