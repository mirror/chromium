// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.RemoteException;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;

import java.util.Arrays;
/**
 * Gets system logs for the Assistant Devices and elide PII sensitive info from it.
 *
 * <p>Elided information includes: Emails, IP address, MAC address, URL/domains as well as
 * Javascript console messages.
 */
class ExternalServiceDeviceLogcatProvider extends LogcatProvider {
    private static final String TAG = "AssistantDeviceLogcatProvider";

    @Override
    public void getLogcat(LogcatCallback callback) {
        Log.i(TAG, "Sending bind command");
        Intent intent = new Intent();

        // TODO(sandv): Inject stub of service for testing
        intent.setComponent(new ComponentName(
                BuildConfig.DEVICE_LOGS_PROVIDER_PACKAGE, BuildConfig.DEVICE_LOGS_PROVIDER_CLASS));
        ContextUtils.getApplicationContext().bindService(intent, new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
                Log.i(TAG, "onServiceConnected for this");
                IDeviceLogsProvider provider = IDeviceLogsProvider.Stub.asInterface(service);
                try {
                    String logs = provider.getLogs();
                    Log.i(TAG, "Got logs of length: " + logs.length());
                    callback.onLogsDone(
                            LogcatProvider.elideLogcat(Arrays.asList(logs.split("\n"))));
                } catch (RemoteException e) {
                    Log.e(TAG, "Can't get logs", e);
                    callback.onLogsDone("");
                }
                ContextUtils.getApplicationContext().unbindService(this);
            }

            @Override
            public void onServiceDisconnected(ComponentName name) {
                Log.i(TAG, "onServiceConnected");
            }
        }, Context.BIND_AUTO_CREATE);
        Log.d(TAG, "Sent bind command");
    }
}
