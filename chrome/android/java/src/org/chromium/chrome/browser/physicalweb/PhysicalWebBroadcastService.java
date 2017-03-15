// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.physicalweb;

import android.annotation.TargetApi;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.IBinder;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.AppHooks;
import org.chromium.chrome.browser.notifications.ChromeNotificationBuilder;
import org.chromium.chrome.browser.notifications.NotificationConstants;
import org.chromium.chrome.browser.notifications.NotificationManagerProxy;
import org.chromium.chrome.browser.notifications.NotificationManagerProxyImpl;

/**
 * Broadcasts Physical Web URLs via Bluetooth Low Energy (BLE).
 *
 * This background service will start when the user activates the Physical Web Sharing item in the
 * sharing chooser intent. If the URL received from the triggering Tab is validated
 * by the Physical Web Service then it will encode the URL into an Eddystone URL and
 * broadcast the URL through BLE.
 *
 * To notify and provide the user with the ability to terminate broadcasting
 * a persistent notification with a cancel button will be created. This cancel button will stop the
 * service and will be the only way for the user to stop the service.
 *
 * If the user terminates Chrome or the OS shuts down this service mid-broadcasting, when the
 * service gets restarted it will restart broadcasting as well by gathering the URL from
 * shared preferences. This will create a seamless experience to the user by behaving as
 * though broadcasting is not connected to the Chrome application.
 *
 * bluetooth.le.AdvertiseCallback and bluetooth.BluetoothAdapter require API level 21.
 * This will only be run on M and above.
 **/

@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class PhysicalWebBroadcastService extends Service {
    public static final String DISPLAY_URL_KEY = "display_url";

    public static final String PREVIOUS_BROADCAST_URL_KEY = "previousUrl";
    private static final String STOP_SERVICE =
            "org.chromium.chrome.browser.physicalweb.stop_service";
    private static final String TAG = "PhysicalWebSharing";

    private boolean mStartedByRestart;

    private final BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            if (STOP_SERVICE.equals(action)) {
                stopSelf();
            } else {
                Log.e(TAG, "Unrecognized Broadcast Received");
            }
        }
    };

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String displayUrl = fetchDisplayUrl(intent);

        // This should never happen.
        if (displayUrl == null) {
            Log.e(TAG, "Given null display URL");
            stopSelf();
            return START_STICKY;
        }

        IntentFilter filter = new IntentFilter(STOP_SERVICE);
        registerReceiver(mBroadcastReceiver, filter);

        // TODO(iankc): implement parsing, broadcasting, Url Shortener.
        createBroadcastNotification(displayUrl);
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        unregisterReceiver(mBroadcastReceiver);
        disableUrlBroadcasting();
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private String fetchDisplayUrl(Intent intent) {
        mStartedByRestart = intent == null;
        if (mStartedByRestart) {
            SharedPreferences sharedPrefs = ContextUtils.getAppSharedPreferences();
            return sharedPrefs.getString(PREVIOUS_BROADCAST_URL_KEY, null);
        }

        String displayUrl = intent.getStringExtra(DISPLAY_URL_KEY);
        ContextUtils.getAppSharedPreferences()
                .edit()
                .putString(PREVIOUS_BROADCAST_URL_KEY, displayUrl)
                .apply();
        return displayUrl;
    }

    /**
     * Surfaces a notification to the user that the Physical Web is broadcasting.
     * The notification specifies the URL that is being broadcast. It cannot be swiped away,
     * but broadcasting can be terminated with the stop button on the notification.
     */
    private void createBroadcastNotification(String displayUrl) {
        Context context = getApplicationContext();
        PendingIntent stopPendingIntent = PendingIntent.getBroadcast(
                context, 0, new Intent(STOP_SERVICE), PendingIntent.FLAG_UPDATE_CURRENT);
        NotificationManagerProxy notificationManager = new NotificationManagerProxyImpl(
                (NotificationManager) getSystemService(NOTIFICATION_SERVICE));
        ChromeNotificationBuilder notificationBuilder =
                AppHooks.get()
                        .createChromeNotificationBuilder(true /* preferCompat */,
                                NotificationConstants.CATEGORY_ID_BROWSER,
                                context.getString(R.string.notification_category_browser),
                                NotificationConstants.CATEGORY_GROUP_ID_GENERAL,
                                context.getString(R.string.notification_category_group_general))
                        .setSmallIcon(R.drawable.ic_image_white_24dp)
                        .setContentTitle(getString(R.string.physical_web_broadcast_notification))
                        .setContentText(displayUrl)
                        .setOngoing(true)
                        .addAction(android.R.drawable.ic_menu_close_clear_cancel,
                                getString(R.string.physical_web_stop_broadcast), stopPendingIntent);
        notificationManager.notify(
                NotificationConstants.NOTIFICATION_ID_PHYSICAL_WEB, notificationBuilder.build());
    }

    /** Turns off URL broadcasting. */
    private void disableUrlBroadcasting() {
        NotificationManagerProxy notificationManager = new NotificationManagerProxyImpl(
                (NotificationManager) getSystemService(NOTIFICATION_SERVICE));
        notificationManager.cancel(NotificationConstants.NOTIFICATION_ID_PHYSICAL_WEB);
    }
}
