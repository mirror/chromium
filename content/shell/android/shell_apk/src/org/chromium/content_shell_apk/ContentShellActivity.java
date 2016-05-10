// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_shell_apk;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Handler;
import android.os.Vibrator;
import android.text.TextUtils;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Toast;

import com.google.vrtoolkit.cardboard.controller.Controller;
import com.google.vrtoolkit.cardboard.controller.Controller.ConnectionStates;
import com.google.vrtoolkit.cardboard.controller.ControllerManager;

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
import org.chromium.content.browser.MotionEventModifier;
import org.chromium.content_public.browser.GestureStateListener;
import org.chromium.content.common.ContentSwitches;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_shell.Shell;
import org.chromium.content_shell.ShellManager;
import org.chromium.ui.base.ActivityWindowAndroid;
//TODO move these two imports. they should be in GestureHandler
import android.view.MotionEvent.PointerCoords;
import android.view.MotionEvent.PointerProperties;

import com.google.vrtoolkit.cardboard.CardboardView;
import com.google.vrtoolkit.cardboard.sensors.SensorConnection;
import com.google.vrtoolkit.cardboard.CardboardDeviceParams;

import java.lang.reflect.*;

/**
 * Activity for managing the Content Shell.
 */
public class ContentShellActivity extends Activity
    implements SensorConnection.SensorListener {

    private static final String TAG = "ContentShellActivity";

    private static final String ACTIVE_SHELL_URL_KEY = "activeUrl";
    public static final String COMMAND_LINE_ARGS_KEY = "commandLineArgs";

    private ShellManager mShellManager;
    private ActivityWindowAndroid mWindowAndroid;
    private Intent mLastSentIntent;

    private CardboardView mCardboardView;
    private ContentCardboardRenderer mRenderer;
    private boolean mVrEnabled = false;
    private int mSystemUiVisibilityFlag = -1;

    private Vibrator vibrator;
    private int mWidth;
    private int mHeight;

    private ControllerManager controllerManager;
    private Controller controller;
    private Handler uiHandler;

    private final SensorConnection sensorConnection = new SensorConnection(this);

    private void setupCardboardWindowFlags(boolean isCardboard) {
      Window window = getWindow();
      if (isCardboard) {
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        mSystemUiVisibilityFlag = window.getDecorView().getSystemUiVisibility();
        window.getDecorView().setSystemUiVisibility(
             View.SYSTEM_UI_FLAG_LAYOUT_STABLE
             | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
             | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
             | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
             | View.SYSTEM_UI_FLAG_FULLSCREEN
             | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        );
      } else {
        window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        if (mSystemUiVisibilityFlag != -1) {
          window.getDecorView().setSystemUiVisibility(mSystemUiVisibilityFlag);
        }
      }
    }

    /**
     * @hide
     * Called when the device was placed inside a Cardboard.
     *
     * @param cardboardDeviceParams Parameters of the Cardboard device.
     */
    @Override
    public void onInsertedIntoCardboard(CardboardDeviceParams cardboardDeviceParams) {
        Log.d("bshe:log", "---inserted to cardboard----");
    }

    /**
     * @hide
     * Called when the device was removed from a Cardboard.
     */
    @Override
    public void onRemovedFromCardboard() {
      Log.d("bshe:log", "---removed from cardboard----");
    }

    /**
     * Override to detect when the Cardboard trigger was pulled and released.
     * </p><p>
     * Provides click-like events when the device is inside a Cardboard.
     */
    @Override
    public void onCardboardTrigger() {
      Log.d("bshe:log", "---triggered----");
      vibrator.vibrate(50);
    }

    @Override
    protected void onPause() {
      super.onPause();
      mCardboardView.onPause();
      setupCardboardWindowFlags(false);
      sensorConnection.onPause(this);
    }

    @Override
    protected void onResume() {
      super.onResume();
      mCardboardView.onResume();
      // Should check if we are in vr mode.
      if (mVrEnabled) {
          setupCardboardWindowFlags(true);
      }
      sensorConnection.onResume(this);
    }

    @Override
    @SuppressFBWarnings("DM_EXIT")
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Initializing the command line must occur before loading the library.
        if (!CommandLine.isInitialized()) {
            ContentApplication.initCommandLine(this);
            String[] commandLineParams = getCommandLineParamsFromIntent(getIntent());
            if (commandLineParams != null) {
                CommandLine.getInstance().appendSwitchesAndArguments(commandLineParams);
            }
        }
        waitForDebuggerIfNeeded();

        DeviceUtils.addDeviceSpecificUserAgentSwitch(this);
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

        setContentView(R.layout.content_shell_activity);
        mShellManager = (ShellManager) findViewById(R.id.shell_container);
        final boolean listenToActivityState = true;
        mWindowAndroid = new ActivityWindowAndroid(this, listenToActivityState);
        mWindowAndroid.restoreInstanceState(savedInstanceState);
        mShellManager.setWindow(mWindowAndroid);
        // Set up the animation placeholder to be the SurfaceView. This disables the
        // SurfaceView's 'hole' clipping during animations that are notified to the window.
        mWindowAndroid.setAnimationPlaceholderView(
                mShellManager.getContentViewRenderView().getSurfaceView());

        String startupUrl = "file:///storage/self/primary/Cardboard/print.html";//getUrlFromIntent(getIntent());
        if (!TextUtils.isEmpty(startupUrl)) {
            mShellManager.setStartupUrl(Shell.sanitizeUrl(startupUrl));
        }

        if (CommandLine.getInstance().hasSwitch(ContentSwitches.RUN_LAYOUT_TEST)) {
            try {
                BrowserStartupController.get(this, LibraryProcessType.PROCESS_BROWSER)
                        .startBrowserProcessesSync(false);
            } catch (ProcessInitException e) {
                Log.e(TAG, "Failed to load native library.", e);
                System.exit(-1);
            }
        } else {
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

        ////
        mCardboardView = (CardboardView) findViewById(R.id.cardboard_view);
        mRenderer = new ContentCardboardRenderer(this);
        mCardboardView.setRenderer(mRenderer);
        mCardboardView.setTransitionViewEnabled(true);
        mCardboardView.setVisibility(View.GONE);
        sensorConnection.onCreate(this);

        vibrator = (Vibrator) getSystemService(Context.VIBRATOR_SERVICE);
        EventListener listener = new EventListener();
        controllerManager = new ControllerManager(this, listener);
        controller = controllerManager.getController();
        controller.setEventListener(listener);
        uiHandler = new Handler();
        ////
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
      super.onConfigurationChanged(newConfig);

      // Hack. Connect orientation change to cardboard.
      if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
//        Intent cardboardIntend = new Intent(this,
//            ContentCardboardActivity.class);
//        startActivity(cardboardIntend);

        //requestWindowFeature(Window.FEATURE_NO_TITLE);
        mVrEnabled = true;
        mCardboardView.setVisibility(View.VISIBLE);
        mShellManager.getContentViewRenderView().useSurface(false);
        setupCardboardWindowFlags(true);
        Log.d("bshe:log", "About to make cardboard view visible.");
        controllerManager.start();
      } else {
        mVrEnabled = false;
        mShellManager.getContentViewRenderView().useSurface(true);
        mCardboardView.setVisibility(View.GONE);
        setupCardboardWindowFlags(false);
        Log.d("bshe:log", "About to make cardboard view disappear.");
      }
    }

    private void finishInitialization(Bundle savedInstanceState) {
        String shellUrl = "file:///storage/self/primary/Cardboard/print.html";//ShellManager.DEFAULT_SHELL_URL;
        if (savedInstanceState != null
                && savedInstanceState.containsKey(ACTIVE_SHELL_URL_KEY)) {
            shellUrl = savedInstanceState.getString(ACTIVE_SHELL_URL_KEY);
        }
        mShellManager.launchShell(shellUrl);

        mShellManager.getActiveShell().getContentViewCore().addGestureStateListener(new GestureStateListener(){
               @Override
               public void onSingleTap(boolean consumed, int x, int y) {
                  onCardboardTrigger();
               }
            });

    }

    private <T> void inspect(Class<T> klazz) {
        Field[] fields = klazz.getDeclaredFields();
        Log.i("Inspect: ",String.format("%d fields:%n", fields.length));
        for (Field field : fields) {
            Log.i("  + ", String.format("%s %s %s%n",
                Modifier.toString(field.getModifiers()),
                field.getType().getSimpleName(),
                field.getName())
            );
        }
    }

    private void initializationFailed() {
        Log.e(TAG, "ContentView initialization failed.");
        Toast.makeText(ContentShellActivity.this,
                R.string.browser_process_initialization_failed,
                Toast.LENGTH_SHORT).show();
        finish();
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        ContentViewCore contentViewCore = getActiveContentViewCore();
        if (contentViewCore != null) {
            outState.putString(ACTIVE_SHELL_URL_KEY, contentViewCore.getWebContents().getUrl());
        }

        mWindowAndroid.saveInstanceState(outState);
    }

    private void waitForDebuggerIfNeeded() {
        if (CommandLine.getInstance().hasSwitch(BaseSwitches.WAIT_FOR_JAVA_DEBUGGER)) {
            Log.e(TAG, "Waiting for Java debugger to connect...");
            android.os.Debug.waitForDebugger();
            Log.e(TAG, "Java debugger connected. Resuming execution.");
        }
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            ContentViewCore contentViewCore = getActiveContentViewCore();
            if (contentViewCore != null && contentViewCore.getWebContents()
                    .getNavigationController().canGoBack()) {
                contentViewCore.getWebContents().getNavigationController().goBack();
                return true;
            }
        }

        return super.onKeyUp(keyCode, event);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        if (getCommandLineParamsFromIntent(intent) != null) {
            Log.i(TAG, "Ignoring command line params: can only be set when creating the activity.");
        }

        if (MemoryPressureListener.handleDebugIntent(this, intent.getAction())) return;

        String url = getUrlFromIntent(intent);
        if (!TextUtils.isEmpty(url)) {
            Shell activeView = getActiveShell();
            if (activeView != null) {
                activeView.loadUrl(url);
            }
        }
    }

    @Override
    protected void onStart() {
        super.onStart();
        if (mVrEnabled)
            controllerManager.start();
        ContentViewCore contentViewCore = getActiveContentViewCore();
        if (contentViewCore != null) contentViewCore.onShow();
    }

    @Override
    protected void onStop() {
       controllerManager.stop();
       super.onStop();
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        mWindowAndroid.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    public void startActivity(Intent i) {
        mLastSentIntent = i;
        super.startActivity(i);
    }

    @Override
    protected void onDestroy() {
        if (mShellManager != null) mShellManager.destroy();
        super.onDestroy();
        sensorConnection.onDestroy(this);
    }

    public Intent getLastSentIntent() {
        return mLastSentIntent;
    }

    private static String getUrlFromIntent(Intent intent) {
        return intent != null ? intent.getDataString() : null;
    }

    private static String[] getCommandLineParamsFromIntent(Intent intent) {
        return intent != null ? intent.getStringArrayExtra(COMMAND_LINE_ARGS_KEY) : null;
    }

    /**
     * @return The {@link ShellManager} configured for the activity or null if it has not been
     *         created yet.
     */
    public ShellManager getShellManager() {
        return mShellManager;
    }

    /**
     * @return The currently visible {@link Shell} or null if one is not showing.
     */
    public Shell getActiveShell() {
        return mShellManager != null ? mShellManager.getActiveShell() : null;
    }

    /**
     * @return The {@link ContentViewCore} owned by the currently visible {@link Shell} or null if
     *         one is not showing.
     */
    public ContentViewCore getActiveContentViewCore() {
        Shell shell = getActiveShell();
        return shell != null ? shell.getContentViewCore() : null;
    }

    /**
     * @return The {@link WebContents} owned by the currently visible {@link Shell} or null if
     *         one is not showing.
     */
    public WebContents getActiveWebContents() {
        Shell shell = getActiveShell();
        return shell != null ? shell.getWebContents() : null;
    }

    private class EventListener extends Controller.EventListener
        implements ControllerManager.EventListener, Runnable {

    private int controllerState = ConnectionStates.DISCONNECTED;
    private String serviceState;
    private float lastx;
    private float lasty;
    private boolean lastb = false;

    @Override
    public void onServiceDebugStringChanged(String state) {
      serviceState = state;
      uiHandler.post(this);
    }

    @Override
    public void onConnectionStateChanged(int state) {
      controllerState = state;
      uiHandler.post(this);
    }

    @Override
    public void onUpdate() {
      uiHandler.post(this);
    }
    private static final int MAX_NUM_POINTERS = 16;
    @Override
    public void run() {
      controller.update();
      Log.i(TAG, String.format("Hoverboard input: [%4.2f, %4.2f]-[w: %4.2f, x: %4.2f, y: %4.2f, z: %4.2f][%s][%s][%s][%s][%s]",
          controller.touch.x, controller.touch.y,
          controller.orientation.w, controller.orientation.x,
          controller.orientation.y, controller.orientation.z,
          controller.appButtonState ? "A" : " ",
          controller.homeButtonState ? "H" : " ",
          controller.clickButtonState ? "T" : " ",
          controller.volumeUpButtonState ? "+" : " ",
          controller.volumeDownButtonState ? "-" : " "));
      float dx = (lastx - controller.touch.x) * 500;
      float dy = (lasty - controller.touch.y) * 500;
      if ((controller.touch.x != 0 || controller.touch.y != 0) &&
          (dx != 0 || dy != 0)) {
        if (Math.abs(lastx - controller.touch.x) < 0.3 && Math.abs(lasty - controller.touch.y) < 0.3)
          mShellManager.getActiveShell().getContentViewCore().scrollBy(dx, dy, false);
      }
      lastx = controller.touch.x;
      lasty = controller.touch.y;
      if (controller.homeButtonState)
          mRenderer.setAngleDot(controller.orientation.y, controller.orientation.x);
      if (controller.appButtonState) {
        Log.i("CHROME: ", "Step 1");
        Display display = getWindowManager().getDefaultDisplay();
        mWidth = display.getWidth();
        mHeight = display.getHeight();
        if (!lastb)
            mShellManager.getActiveShell().getContentViewCore().hoverboardSingleTap(
                mRenderer.getLookAtX(mWidth, mHeight), mRenderer.getLookAtY(mWidth, mHeight));
      }
      lastb = controller.appButtonState;
    }

  }

}
