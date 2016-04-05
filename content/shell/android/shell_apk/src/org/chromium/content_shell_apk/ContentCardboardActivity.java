// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_shell_apk;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.widget.Toast;

import com.google.vrtoolkit.cardboard.CardboardActivity;
import com.google.vrtoolkit.cardboard.CardboardView;

import org.chromium.base.BaseSwitches;
import org.chromium.base.CommandLine;
import org.chromium.base.MemoryPressureListener;
import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.content.app.ContentApplication;
import org.chromium.content.browser.BrowserStartupController;
import org.chromium.content.browser.ContentViewCore;
import org.chromium.content.browser.DeviceUtils;
import org.chromium.content.common.ContentSwitches;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_shell.Shell;
import org.chromium.content_shell.ShellManager;
import org.chromium.ui.base.ActivityWindowAndroid;


/**
 * Activity for managing the Content Shell.
 */
public class ContentCardboardActivity extends CardboardActivity {

    private static final String TAG = "ContentShellActivity";

    private static final String ACTIVE_SHELL_URL_KEY = "activeUrl";
    public static final String COMMAND_LINE_ARGS_KEY = "commandLineArgs";

    private ShellManager mShellManager;
    private ActivityWindowAndroid mWindowAndroid;
    private Intent mLastSentIntent;
    private ContentCardboardRenderer mRenderer;

    @Override
    @SuppressFBWarnings("DM_EXIT")
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        //DeviceUtils.addDeviceSpecificUserAgentSwitch(this);
        try {
            LibraryLoader.get(LibraryProcessType.PROCESS_BROWSER)
                    .ensureInitialized(getApplicationContext());
        } catch (ProcessInitException e) {
            Log.e(TAG, "ContentView initialization failed.", e);
            // Since the library failed to initialize nothing in the application
            // can work, so kill the whole application not just the activity
            System.exit(-1);
            return;
        }

        setContentView(R.layout.content_vr_shell_activity);
        CardboardView cardboardView = (CardboardView) findViewById(R.id.cardboard_view);
        mRenderer = new ContentCardboardRenderer(this);
        cardboardView.setRenderer(mRenderer);
        setCardboardView(cardboardView);

        mShellManager = (ShellManager) findViewById(R.id.shell_container);
        final boolean listenToActivityState = true;
        mWindowAndroid = new ActivityWindowAndroid(this, listenToActivityState);
        mWindowAndroid.restoreInstanceState(savedInstanceState);
        mShellManager.setWindow(mWindowAndroid);
        // Set up the animation placeholder to be the SurfaceView. This disables the
        // SurfaceView's 'hole' clipping during animations that are notified to the window.
        mWindowAndroid.setAnimationPlaceholderView(
                mShellManager.getContentViewRenderView().getSurfaceView());

        String startupUrl = getUrlFromIntent(getIntent());
        if (!TextUtils.isEmpty(startupUrl)) {
            mShellManager.setStartupUrl(Shell.sanitizeUrl(startupUrl));
        }

        try {
                BrowserStartupController.get(this, LibraryProcessType.PROCESS_BROWSER)
                        .startBrowserProcessesAsync(
                                new BrowserStartupController.StartupCallback() {
                                    @Override
                                    public void onSuccess(boolean alreadyStarted) {
                                        finishInitialization(savedInstanceState);
                                    }

                                    @Override
                                    public void onFailure() {
                                        initializationFailed();
                                    }
                                });
            } catch (ProcessInitException e) {
                Log.e(TAG, "Unable to load native library.", e);
                System.exit(-1);
            }

    }

    private static String getUrlFromIntent(Intent intent) {
        return intent != null ? intent.getDataString() : null;
    }

    private void finishInitialization(Bundle savedInstanceState) {
        String shellUrl = ShellManager.DEFAULT_SHELL_URL;
        if (savedInstanceState != null
                && savedInstanceState.containsKey(ACTIVE_SHELL_URL_KEY)) {
            shellUrl = savedInstanceState.getString(ACTIVE_SHELL_URL_KEY);
        }
        mShellManager.launchShell(shellUrl);
    }

    private void initializationFailed() {
        Log.e(TAG, "ContentView initialization failed.");
        Toast.makeText(ContentCardboardActivity.this,
                R.string.browser_process_initialization_failed,
                Toast.LENGTH_SHORT).show();
        finish();
    }
    @Override
    public void onCardboardTrigger() {
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

}
