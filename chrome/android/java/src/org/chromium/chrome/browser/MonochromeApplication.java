// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.util.Log;

import com.android.webview.chromium.LicenseContentProvider;
import com.android.webview.chromium.MonochromeLibraryPreloader;

import org.chromium.base.ThreadUtils;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.base.process_launcher.ChildProcessCreationParams;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.components.aboutui.CreditUtils;

import java.util.concurrent.Callable;

/**
 * This is Application class for Monochrome.
 * <p>
 * You shouldn't add anything else in this file, this class is split off from
 * normal chrome in order to access Android system API through Android WebView
 * glue layer and have monochrome specific code.
 */
public class MonochromeApplication extends ChromeApplication {
    @Override
    public void onCreate() {
        super.onCreate();
        LibraryLoader.setNativeLibraryPreloader(new MonochromeLibraryPreloader());
        // ChildProcessCreationParams is only needed for browser process, though it is
        // created and set in all processes.
        boolean bindToCaller = false;
        ChildProcessCreationParams.registerDefault(new ChildProcessCreationParams(getPackageName(),
                true /* isExternalService */, LibraryProcessType.PROCESS_CHILD, bindToCaller));
        LicenseContentProvider.licenseProvider = new Callable<byte[]>() {
            @Override
            public byte[] call() throws Exception {
                ThreadUtils.runOnUiThreadBlocking(new Runnable() {
                    @Override
                    public void run() {
                        try {
                            ChromeBrowserInitializer.getInstance(getApplicationContext())
                                    .handleSynchronousStartup();
                        } catch (ProcessInitException e) {
                            Log.e("LicenseCP", "Fail to initialize the Chrome Browser.", e);
                        }
                    }
                });

                return CreditUtils.nativeGetJavaWrapperCredits();
            }
        };
    }
}
