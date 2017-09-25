// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.page_info;

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
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.browser.LocationSettingsTestUtil;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.net.test.EmbeddedTestServer;
import org.chromium.ui.base.PageTransition;

// import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;

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
    public void setUp() throws ExecutionException, InterruptedException {
        mActivityTestRule.startMainActivityOnBlankPage();
        // mTestServer = EmbeddedTestServer.createAndStartServer(
        //        InstrumentationRegistry.getInstrumentation().getContext());
        // ThreadUtils.runOnUiThreadBlocking(new Callable<Tab>() {
        //    @Override
        //    public Tab call() throws InterruptedException {
        //        Tab tab = mActivityTestRule.getActivity().getActivityTab();
        //        mActivityTestRule.loadUrlInTab(mTestServer.getURL("/chrome/test/data/empty.html"),
        //                PageTransition.LINK, tab, 0);
        //        tab.initialize(null, null, new TabDelegateFactory(), false, false);
        //        tab.show(TabSelectionType.FROM_USER);
        //        return tab;
        //    }
        //});
        String url = "https://www.google.com";
        Tab tab = mActivityTestRule.getActivity().getActivityTab();
        mActivityTestRule.loadUrlInTab(url, PageTransition.LINK, tab, 0);
    }

    @After
    public void tearDown() throws Exception {
        // mTestServer.stopAndDestroyServer();
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

    /**
     * Tests that tapping the text on a permission on the PageInfoPopup when Chrome has not been
     * granted the permission on an app-level takes the user to the settings page.
     */
    @Test
    @MediumTest
    @Feature({"PageInfoPopup"})
    @RetryOnFailure
    public void testGrantingLocationToSiteWhileOff() throws InterruptedException {
        LocationSettingsTestUtil.setSystemLocationSettingEnabled(false);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mPageInfo = new PageInfoPopup(mActivityTestRule.getActivity(),
                        mActivityTestRule.getActivity().getActivityTab(), null, null);

                // Turn off system-wide location, but turn on for this site.
                mPageInfo.nativeOnSitePermissionChanged(mPageInfo.mNativePageInfoPopup,
                        ContentSettingsType.CONTENT_SETTINGS_TYPE_GEOLOCATION,
                        ContentSetting.ALLOW.toInt());
            }
        });

        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                if (mPageInfo == null) return false;

                // Make sure this doesn't crash.
                View view = mPageInfo.mContainer.findViewById(R.id.page_info_permission_text);
                mPageInfo.onClick(view);

                // Double check that by checking it's the right permission tag added.
                String[] permissionTags = (String[]) view.getTag(R.id.permission_type);
                return permissionTags != null && permissionTags.length == 1
                        && permissionTags[0] == "Location";
            }
        });
    }
}
