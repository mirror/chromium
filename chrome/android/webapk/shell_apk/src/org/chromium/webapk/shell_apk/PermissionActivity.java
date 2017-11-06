// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.shell_apk;

import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;

/**
 * Created by hanxi on 10/5/17.
 */

public class PermissionActivity extends Activity {
    public static final String EXTRA_PERMISSIONS = "extra.permissions";
    public static final String EXTRA_GRANT_RESULTS = "extra.granted.results";
    public static final String EXTRA_RESULT_RECEIVER = "extra.result.receiver";

    private Messenger mMessenger;
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        String[] permissions = getIntent().getStringArrayExtra(EXTRA_PERMISSIONS);
        mMessenger = getIntent().getParcelableExtra(EXTRA_RESULT_RECEIVER);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(permissions, 1);
        } else {
            notify(null);
        }
    }

    @Override
    public void onRequestPermissionsResult(
            int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        notify(grantResults);
    }

    private void notify(int[] grantResults) {
        Message message = new Message();
        Bundle data = new Bundle();
        data.putIntArray(EXTRA_GRANT_RESULTS, grantResults);
        message.setData(data);
        try {
            mMessenger.send(message);
        } catch (RemoteException e) {
            e.printStackTrace();
        } finally {
            done();
        }
    }

    private void done() {
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP) {
            finishAndRemoveTask();
        } else {
            finish();
        }
    }
}
