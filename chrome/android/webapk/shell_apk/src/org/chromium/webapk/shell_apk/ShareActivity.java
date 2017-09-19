// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.shell_apk;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.text.TextUtils;

import org.chromium.webapk.lib.common.WebApkConstants;
import org.chromium.webapk.lib.common.WebApkMetaDataKeys;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * WebAPK's share handler Activity.
 */
public class ShareActivity extends HostBrowserLaunchingActivity {
    private static final String TAG = "cr_ShareActivity";

    private String mStartUrl;

    @Override
    protected boolean canHandleIntent(Bundle savedInstanceState) {
        ActivityInfo ai;
        try {
            ai = getPackageManager().getActivityInfo(
                    getComponentName(), PackageManager.GET_META_DATA);
        } catch (PackageManager.NameNotFoundException e) {
            return false;
        }

        mStartUrl = extractShareTarget(ai.metaData);
        if (TextUtils.isEmpty(mStartUrl)) {
            return false;
        }
        return true;
    }

    @Override
    protected Intent getLaunchIntent() {
        Intent intent = new Intent();
        intent.setAction(ACTION_START_WEBAPK);
        intent.putExtra(WebApkConstants.EXTRA_URL, mStartUrl)
                .putExtra(WebApkConstants.EXTRA_FORCE_NAVIGATION, true)
                .putExtra(WebApkConstants.EXTRA_SOURCE, WebApkConstants.SHORTCUT_SOURCE_SHARE)
                .putExtra(WebApkConstants.EXTRA_WEBAPK_PACKAGE_NAME, getPackageName());
        return intent;
    }

    private String extractShareTarget(Bundle metadata) {
        String shareTemplate = metadata.getString(WebApkMetaDataKeys.SHARE_TEMPLATE);
        String sharedText = null;
        try {
            sharedText = URLEncoder.encode(getIntent().getStringExtra(Intent.EXTRA_TEXT), "UTF-8");
        } catch (UnsupportedEncodingException e) {
            // No-op: we'll just treat this as unspecified.
        }
        String sharedTitle = null;
        try {
            sharedTitle =
                    URLEncoder.encode(getIntent().getStringExtra(Intent.EXTRA_SUBJECT), "UTF-8");
        } catch (UnsupportedEncodingException e) {
            // No-op: we'll just treat this as unspecified.
        }

        Pattern placeholder = Pattern.compile("\\{.*?\\}");
        Matcher matcher = placeholder.matcher(shareTemplate);
        StringBuffer buffer = new StringBuffer();
        while (matcher.find()) {
            if (matcher.group().equals("{text}")) {
                matcher.appendReplacement(buffer, sharedText);
            } else if (matcher.group().equals("{title}")) {
                matcher.appendReplacement(buffer, sharedTitle);
            } else {
                matcher.appendReplacement(buffer, "");
            }
        }
        matcher.appendTail(buffer);
        return buffer.toString();
    }
}
