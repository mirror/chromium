// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.page_info;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.v7.widget.SwitchCompat;
import android.view.View;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ContentSettingsType;
import org.chromium.chrome.browser.preferences.website.ContentSetting;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.net.test.EmbeddedTestServer;

/**
 * Tests for PageInfoPopup.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class PageInfoPopupTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);
    private PageInfoPopup mPageInfo;
    private EmbeddedTestServer mTestServer;

    @Before
    public void setUp() throws InterruptedException {
        mActivityTestRule.startMainActivityFromLauncher();
        mTestServer = EmbeddedTestServer.createAndStartServer(
                InstrumentationRegistry.getInstrumentation().getContext());
        mActivityTestRule.loadUrl(mTestServer.getURL("/chrome/test/data/android/simple.html"));
    }

    @After
    public void tearDown() throws Exception {
        mTestServer.stopAndDestroyServer();
    }

    /**
     * Tests that PageInfoPopup can be instantiated and shown.
     */
    @Test
    @MediumTest
    @Feature({"PageInfoPopup"})
    @RetryOnFailure
    public void testShow() throws InterruptedException {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PageInfoPopup.show(mActivityTestRule.getActivity(),
                        mActivityTestRule.getActivity().getActivityTab(), null,
                        PageInfoPopup.OPENED_FROM_MENU);
            }
        });
    }

    /**
     * Tests that by default, PageInfoPopup doesn't show any permissions.
     */
    @Test
    @MediumTest
    @Feature({"PageInfoPopup"})
    @RetryOnFailure
    public void testNoPermissionsShownByDefault() throws InterruptedException {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PageInfoPopup mPageInfo = new PageInfoPopup(mActivityTestRule.getActivity(),
                        mActivityTestRule.getActivity().getActivityTab(), null, null);
                View permissionsList =
                        mPageInfo.mContainer.findViewById(R.id.page_info_permissions_list);
                Assert.assertEquals(View.GONE, permissionsList.getVisibility());
            }
        });
    }

    /**
     * Tests that PageInfoPopup will show permissions if they are set to non-default values.
     */
    @Test
    @MediumTest
    @Feature({"PageInfoPopup"})
    @RetryOnFailure
    public void testNonDefaultPermissionsAreShown() throws InterruptedException {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mPageInfo = new PageInfoPopup(mActivityTestRule.getActivity(),
                        mActivityTestRule.getActivity().getActivityTab(), null, null);
                mPageInfo.nativeOnSitePermissionChanged(mPageInfo.mNativePageInfoPopup,
                        ContentSettingsType.CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
                        ContentSetting.ALLOW.toInt());
            }
        });

        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                if (mPageInfo == null) return false;

                return mPageInfo.mPermissionsList.getVisibility() == View.VISIBLE
                        && mPageInfo.mDisplayedPermissions.size() == 1
                        && mPageInfo.mDisplayedPermissions.get(0).name.equals("Notifications");
            }
        });
    }

    /**
     * Tests that tapping a permission switch on the PageInfoPopup will change the permission.
     */
    @Test
    @MediumTest
    @Feature({"PageInfoPopup"})
    @RetryOnFailure
    public void testTogglingPermission() throws InterruptedException {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mPageInfo = new PageInfoPopup(mActivityTestRule.getActivity(),
                        mActivityTestRule.getActivity().getActivityTab(), null, null);
                mPageInfo.nativeOnSitePermissionChanged(mPageInfo.mNativePageInfoPopup,
                        ContentSettingsType.CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC,
                        ContentSetting.ALLOW.toInt());
            }
        });

        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                if (mPageInfo == null) return false;

                SwitchCompat permissionSwitch = (SwitchCompat) mPageInfo.mContainer.findViewById(
                        R.id.page_info_permission_switch);
                if (permissionSwitch == null) return false;

                permissionSwitch.setChecked(false);
                return true;
            }
        });

        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                if (mPageInfo == null) return false;

                return mPageInfo.mDisplayedPermissions.size() == 1
                        && mPageInfo.mDisplayedPermissions.get(0).name.equals("Microphone")
                        && mPageInfo.mDisplayedPermissions.get(0).setting == ContentSetting.BLOCK;
            }
        });
    }
}
