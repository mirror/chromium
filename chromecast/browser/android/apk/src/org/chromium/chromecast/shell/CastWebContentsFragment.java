// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import android.app.Fragment;
import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.Toast;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.content_public.browser.WebContents;

/**
 * Fragment for displaying a WebContents in CastShell.
 * <p>
 * Typically, this class is controlled by CastContentWindowAndroid through
 * CastWebContentsSurfaceHelper. If the CastContentWindowAndroid is destroyed,
 * CastWebContentsFragment should be removed from the activity holding it.
 * Similarily, if the fragment is removed from a activity or the activity holding
 * it is destroyed, CastContentWindowAndroid should be notified by intent.
 */
public class CastWebContentsFragment extends Fragment {
    private static final String TAG = "cr_CastWebContentFrg";

    // TODO(vincentli) interrupt touch event from Fragment's root view when it's false.
    private boolean mIsTouchInputEnabled = false;

    private Context mPackageContext;

    private CastWebContentsSurfaceHelper mSurfaceHelper;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate");
        super.onCreate(savedInstanceState);

        // TODO(vincentli) let host activity to set or unset the flags because when fragment
        // is showing non-video cast app or in PIP mode, the screen should be able to be off.
        // Set flags to both exit sleep mode when this activity starts and
        // avoid entering sleep mode while playing media. We cannot distinguish
        // between video and audio so this applies to both.
        getActivity().getWindow().addFlags(WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON);
        getActivity().getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        Log.d(TAG, "onCreateView");
        super.onCreateView(inflater, container, savedInstanceState);

        if (!CastBrowserHelper.initializeBrowser(getActivity().getApplicationContext())) {
            Toast.makeText(getActivity(), R.string.browser_process_initialization_failed,
                         Toast.LENGTH_SHORT)
                    .show();
            return null;
        }

        return inflater.cloneInContext(getContext())
                .inflate(R.layout.cast_web_contents_activity, null);
    }

    @Override
    public Context getContext() {
        if (mPackageContext == null) {
            String packageName = ContextUtils.getApplicationContext().getPackageName();
            // The fragment could be created from another apk. To load local apk's resources,
            // we need local package's context.
            try {
                mPackageContext = super.getContext().createPackageContext(packageName,
                        Context.CONTEXT_IGNORE_SECURITY | Context.CONTEXT_INCLUDE_CODE);
            } catch (NameNotFoundException e) {
                throw new RuntimeException(e);
            }
        }
        return mPackageContext;
    }

    @Override
    public void onStart() {
        Log.d(TAG, "onStart");
        super.onStart();
        if (mSurfaceHelper == null) {
            mSurfaceHelper = new CastWebContentsSurfaceHelper(getActivity(), /* hostActivity */
                    (FrameLayout) getView().findViewById(R.id.web_contents_container),
                    true /* showInFragment */);
        }
        Bundle bundle = getArguments();
        bundle.setClassLoader(WebContents.class.getClassLoader());
        String uriString = bundle.getString(CastWebContentsComponent.INTENT_EXTRA_URI);
        if (uriString == null) {
            return;
        }
        Uri uri = Uri.parse(uriString);
        WebContents webContents = (WebContents) bundle.getParcelable(
                CastWebContentsComponent.ACTION_EXTRA_WEB_CONTENTS);

        boolean touchInputEnabled =
                bundle.getBoolean(CastWebContentsComponent.ACTION_EXTRA_TOUCH_INPUT_ENABLED, false);
        mSurfaceHelper.onNewWebContents(uri, webContents, touchInputEnabled);
    }

    @Override
    public void onPause() {
        Log.d(TAG, "onPause");
        super.onPause();

        if (mSurfaceHelper != null) {
            mSurfaceHelper.onPause();
        }
    }

    @Override
    public void onResume() {
        Log.d(TAG, "onResume");
        super.onResume();
        if (mSurfaceHelper != null) {
            mSurfaceHelper.onResume();
        }
    }

    @Override
    public void onStop() {
        Log.d(TAG, "onStop");
        super.onStop();
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "onDestroy");
        if (mSurfaceHelper != null) {
            mSurfaceHelper.onDestroy();
        }
        super.onDestroy();
    }
}
