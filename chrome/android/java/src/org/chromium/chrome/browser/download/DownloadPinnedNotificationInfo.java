// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import org.chromium.base.Log;
import org.chromium.chrome.browser.download.DownloadNotificationFactory.DownloadStatus;

/**
 * Helper class for information persisted about pinned notifications.
 * TODO(jming): Consolidate with other downloads-related objects (http://crbug.com/746692).
 */
public class DownloadPinnedNotificationInfo {
    DownloadStatus mDownloadStatus;
    String mFilePath;
    long mSystemDownloadId;
    boolean mIsSupportedMimeType;
    boolean mIsOpenable;
    String mOriginalUrl;
    String mReferrer;
    DownloadSharedPreferenceEntry mEntry;

    static final DownloadPinnedNotificationInfo INVALID_INFO =
            new DownloadPinnedNotificationInfo(DownloadStatus.UNKNOWN, "", -1, false, false, "", "",
                    DownloadSharedPreferenceEntry.INVALID_ENTRY);

    DownloadPinnedNotificationInfo(DownloadStatus downloadStatus, String filePath,
            long systemDownloadId, boolean isSupportedMimeType, boolean isOpenable,
            String originalUrl, String referrer, DownloadSharedPreferenceEntry entry) {
        this.mDownloadStatus = downloadStatus;
        this.mFilePath = filePath;
        this.mSystemDownloadId = systemDownloadId;
        this.mIsSupportedMimeType = isSupportedMimeType;
        this.mIsOpenable = isOpenable;
        this.mOriginalUrl = originalUrl;
        this.mReferrer = referrer;
        this.mEntry = entry;
    }

    static DownloadPinnedNotificationInfo parseFromString(String sharedPrefString) {
        Log.e("joy", "parseFromString sharedPrefString: " + sharedPrefString);
        String[] fields = sharedPrefString.split(",", 8);
        if (fields.length != 8) return INVALID_INFO;

        String stringDownloadStatus = fields[0];
        String stringFilePath = fields[1];
        String stringSystemDownloadId = fields[2];
        String stringIsSupportedMimeType = fields[3];
        String stringIsOpenable = fields[4];
        String stringOriginalUrl = fields[5];
        String stringReferrer = fields[6];
        String stringEntry = fields[7];

        DownloadStatus downloadStatus = DownloadStatus.valueOf(stringDownloadStatus);
        long systemDownloadId = Long.parseLong(stringSystemDownloadId);
        boolean isSupportedMimeType = "1".equals(stringIsSupportedMimeType);
        boolean isOpenable = "1".equals(stringIsOpenable);
        DownloadSharedPreferenceEntry entry =
                DownloadSharedPreferenceEntry.parseFromString(stringEntry);

        return new DownloadPinnedNotificationInfo(downloadStatus, stringFilePath, systemDownloadId,
                isSupportedMimeType, isOpenable, stringOriginalUrl, stringReferrer, entry);
    }

    String getSharedPreferenceString() {
        String serialized = "";
        serialized += mDownloadStatus.name() + ",";
        serialized += mFilePath + ",";
        serialized += mSystemDownloadId + ",";
        serialized += (mIsSupportedMimeType ? "1" : "0") + ",";
        serialized += (mIsOpenable ? "1" : "0") + ",";
        serialized += mOriginalUrl + ",";
        serialized += mReferrer;
        // Keep filename as the last serialized entry because filename can have commas in it.
        serialized += mEntry.getSharedPreferenceString() + ",";

        Log.e("joy", "getSharedPrefString " + serialized);
        return serialized;
    }
}
