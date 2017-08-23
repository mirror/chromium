// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webview_shell.page_cycler;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.LargeTest;
import android.support.test.rule.ActivityTestRule;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.Restriction;
import org.chromium.webview_shell.PageCyclerTestActivity;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * Tests running on bots with internet connection to load popular urls,
 * making sure webview doesn't crash
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class PageCyclerTest {
    @Rule
    public ActivityTestRule<PageCyclerTestActivity> mActivityTestRule =
            new ActivityTestRule<>(PageCyclerTestActivity.class);

    private static final long TIMEOUT_IN_SECS = 20;

    private PageCyclerTestActivity mTestActivity;

    @Before
    public void setUp() throws Exception {
        mTestActivity = mActivityTestRule.launchActivity(null);
    }

    @Test
    @LargeTest
    @Restriction(Restriction.RESTRICTION_TYPE_INTERNET)
    public void testVisitGoogleCom() throws Throwable {
        //TODO(yolandyan@): verify the page
        visitUrlSync("http://google.com");
    }

    @Test
    @LargeTest
    @Restriction(Restriction.RESTRICTION_TYPE_INTERNET)
    public void testVisitFacebookCom() throws Throwable {
        visitUrlSync("http://facebook.com");
    }

    @Test
    @LargeTest
    @Restriction(Restriction.RESTRICTION_TYPE_INTERNET)
    public void testVisitWikipediaOrg() throws Throwable {
        visitUrlSync("http://wikipedia.org");
    }

    @Test
    @LargeTest
    @Restriction(Restriction.RESTRICTION_TYPE_INTERNET)
    public void testVisitAmazonCom() throws Throwable {
        visitUrlSync("http://amazon.com");
    }

    @Test
    @LargeTest
    @Restriction(Restriction.RESTRICTION_TYPE_INTERNET)
    public void testVisitYoutubeCom() throws Throwable {
        visitUrlSync("http://youtube.com");
    }

    @Test
    @LargeTest
    @Restriction(Restriction.RESTRICTION_TYPE_INTERNET)
    public void testVisitYahooCom() throws Throwable {
        visitUrlSync("http://yahoo.com");
    }

    @Test
    @LargeTest
    @Restriction(Restriction.RESTRICTION_TYPE_INTERNET)
    public void testVisitEbayCom() throws Throwable {
        visitUrlSync("http://ebay.com");
    }

    @Test
    @LargeTest
    @Restriction(Restriction.RESTRICTION_TYPE_INTERNET)
    public void testVisitRedditCom() throws Throwable {
        visitUrlSync("http://reddit.com");
    }

    private static class PageCyclerWebViewClient extends WebViewClient {
        private final CallbackHelper mPageFinishedCallback;
        private final CallbackHelper mErrorCallback;

        public PageCyclerWebViewClient() {
            super();
            mPageFinishedCallback = new CallbackHelper();
            mErrorCallback = new CallbackHelper();
        }

        public CallbackHelper getPageFinishedCallback() {
            return mPageFinishedCallback;
        }

        public CallbackHelper getErrorCallback() {
            return mErrorCallback;
        }

        @Override
        public void onPageFinished(WebView view, String url) {
            mPageFinishedCallback.notifyCalled();
        }

        // TODO(yolandyan@): create helper class to manage network error
        @Override
        public void onReceivedError(WebView webview, int code, String description,
                String failingUrl) {
            mErrorCallback.notifyCalled();
        }
    }

    private void visitUrlSync(final String url) throws Throwable {
        final PageCyclerWebViewClient pageCyclerWebViewClient = new PageCyclerWebViewClient();
        InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                final WebView view = mTestActivity.getWebView();
                WebSettings settings = view.getSettings();
                settings.setJavaScriptEnabled(true);
                view.setWebViewClient(pageCyclerWebViewClient);
            }
        });
        CallbackHelper pageFinishedCallback = pageCyclerWebViewClient.getPageFinishedCallback();
        CallbackHelper errorCallback = pageCyclerWebViewClient.getErrorCallback();
        loadUrlSync(url, pageFinishedCallback, errorCallback);
    }

    private void loadUrlSync(final String url, final CallbackHelper pageFinishedCallback,
            final CallbackHelper errorCallback) throws InterruptedException {
        boolean timeout = false;
        int pageFinishedCount = pageFinishedCallback.getCallCount();
        int errorCount = errorCallback.getCallCount();
        loadUrlAsync(url);
        try {
            pageFinishedCallback.waitForCallback(pageFinishedCount, pageFinishedCount + 1,
                    TIMEOUT_IN_SECS, TimeUnit.SECONDS);
        } catch (TimeoutException ex) {
            timeout = true;
        }
        Assert.assertEquals(String.format("Network error while accessing %s", url), errorCount,
                errorCallback.getCallCount());
        Assert.assertFalse(String.format("Timeout error while accessing %s", url), timeout);
    }

    private void loadUrlAsync(final String url) {
        InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                WebView view = mTestActivity.getWebView();
                view.loadUrl(url);
            }
        });
    }
}
