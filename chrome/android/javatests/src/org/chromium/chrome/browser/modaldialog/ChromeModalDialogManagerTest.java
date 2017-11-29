// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.modaldialog;

import static android.support.test.espresso.Espresso.onView;
import static android.support.test.espresso.action.ViewActions.click;
import static android.support.test.espresso.assertion.ViewAssertions.matches;
import static android.support.test.espresso.matcher.ViewMatchers.hasDescendant;
import static android.support.test.espresso.matcher.ViewMatchers.isEnabled;
import static android.support.test.espresso.matcher.ViewMatchers.withId;
import static android.support.test.espresso.matcher.ViewMatchers.withText;

import static org.hamcrest.Matchers.allOf;
import static org.hamcrest.Matchers.not;

import android.support.test.espresso.Espresso;
import android.support.test.filters.SmallTest;
import android.view.View;
import android.view.ViewGroup;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;

import java.util.concurrent.Callable;

/**
 * Tests for displaying and functioning of modal dialogs on tabs.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({
        ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG,
})
public class ChromeModalDialogManagerTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    private static final int MAX_DIALOGS = 3;
    private ChromeTabbedActivity mActivity;
    private ModalDialogManager mManager;
    private ModalDialogView[] mModalDialogViews;

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.startMainActivityOnBlankPage();
        mActivity = mActivityTestRule.getActivity();
        mManager = mActivity.getModalDialogManager();
        mModalDialogViews = new ModalDialogView[MAX_DIALOGS];
        for (int i = 0; i < MAX_DIALOGS; i++) mModalDialogViews[i] = createDialog(i);
    }

    @Test
    @SmallTest
    public void testOneDialog() throws Exception {
        // Initially there is no dialogs in the pending list. Browser controls are not restricted.
        checkPendingSize(0);
        checkBrowserControls(false);

        // Show a dialog. The pending list should be empty, and the dialog should be showing.
        // Browser controls should be restricted.
        showDialog(0);
        checkPendingSize(0);
        onView(withId(R.id.tab_modal_dialog_container))
                .check(matches(hasDescendant(withText("0"))));

        // Dismiss the dialog by clicking positive button.
        onView(withText(R.string.ok)).perform(click());
        checkPendingSize(0);
        onView(withId(R.id.tab_modal_dialog_container))
                .check(matches(not(hasDescendant(withText("0")))));
        checkBrowserControls(false);
    }

    @Test
    @SmallTest
    public void testTwoDialogs() throws Exception {
        // Initially there is no dialogs in the pending list. Browser controls are not restricted.
        checkPendingSize(0);
        checkBrowserControls(false);

        // Show the first dialog.
        // The pending list should be empty, and the dialog should be showing.
        // Browser controls should be restricted.
        showDialog(0);
        checkPendingSize(0);
        onView(withId(R.id.tab_modal_dialog_container))
                .check(matches(hasDescendant(withText("0"))));
        checkBrowserControls(true);

        // Show the second dialog. It should be added to the pending list, and the first dialog
        // should still be shown.
        showDialog(1);
        checkPendingSize(1);
        onView(withId(R.id.tab_modal_dialog_container))
                .check(matches(
                        allOf(hasDescendant(withText("0")), not(hasDescendant(withText("1"))))));
        checkBrowserControls(true);

        // Dismiss the first dialog by clicking cancel. The second dialog should be removed from
        // pending list and shown immediately after.
        onView(withText(R.string.cancel)).perform(click());
        checkPendingSize(0);
        onView(withId(R.id.tab_modal_dialog_container))
                .check(matches(
                        allOf(not(hasDescendant(withText("0"))), hasDescendant(withText("1")))));
        checkBrowserControls(true);

        // Dismiss the second dialog by clicking ok. Browser controls should no longer be
        // restricted.
        onView(withText(R.string.ok)).perform(click());
        checkPendingSize(0);
        onView(withId(R.id.tab_modal_dialog_container))
                .check(matches(allOf(
                        not(hasDescendant(withText("0"))), not(hasDescendant(withText("1"))))));
        checkBrowserControls(false);
    }

    @Test
    @SmallTest
    public void testThreeDialogs() throws Exception {
        // Initially there is no dialogs in the pending list. Browser controls are not restricted.
        checkPendingSize(0);
        checkBrowserControls(false);

        // Show the first dialog.
        // The pending list should be empty, and the dialog should be showing.
        // Browser controls should be restricted.
        showDialog(0);
        checkPendingSize(0);
        onView(withId(R.id.tab_modal_dialog_container))
                .check(matches(hasDescendant(withText("0"))));
        checkBrowserControls(true);

        // Show the second dialog. It should be added to the pending list, and the first dialog
        // should still be shown.
        showDialog(1);
        checkPendingSize(1);
        onView(withId(R.id.tab_modal_dialog_container))
                .check(matches(
                        allOf(hasDescendant(withText("0")), not(hasDescendant(withText("1"))))));
        checkBrowserControls(true);

        // Show the third dialog. It should be added to the pending list, and the first dialog
        // should still be shown.
        showDialog(2);
        checkPendingSize(2);
        onView(withId(R.id.tab_modal_dialog_container))
                .check(matches(allOf(hasDescendant(withText("0")),
                        not(hasDescendant(withText("1"))), not(hasDescendant(withText("2"))))));
        checkBrowserControls(true);

        // Stimulate dismissing the dialog by non-user action. The second dialog should be removed
        // from pending list without showing.
        dismissDialog(1);
        checkPendingSize(1);
        onView(withId(R.id.tab_modal_dialog_container))
                .check(matches(allOf(hasDescendant(withText("0")),
                        not(hasDescendant(withText("1"))), not(hasDescendant(withText("2"))))));
        checkBrowserControls(true);

        // Dismiss the second dialog twice and verify nothing breaks.
        dismissDialog(1);

        // Dismiss the first dialog. The third dialog should be removed from pending list and
        // shown immediately after.
        dismissDialog(0);
        checkPendingSize(0);
        onView(withId(R.id.tab_modal_dialog_container))
                .check(matches(allOf(not(hasDescendant(withText("0"))),
                        not(hasDescendant(withText("1"))), hasDescendant(withText("2")))));
        checkBrowserControls(true);

        // Dismiss the third dialog by clicking OK. Browser controls should no longer be restricted.
        onView(withText(R.string.ok)).perform(click());
        checkPendingSize(0);
        onView(withId(R.id.tab_modal_dialog_container))
                .check(matches(allOf(not(hasDescendant(withText("0"))),
                        not(hasDescendant(withText("1"))), not(hasDescendant(withText("2"))))));
        checkBrowserControls(false);
    }

    @Test
    @SmallTest
    public void testShow_UrlBarFocused() throws Exception {
        // Show a dialog. The dialog should be shown on top of the toolbar.
        showDialog(0);

        TabModalPresenter presenter =
                (TabModalPresenter) mManager.getPresenterForTest(ModalDialogManager.TAB_MODAL);
        final View dialogContainer = presenter.getDialogContainerForTest();
        final View controlContainer = mActivity.findViewById(R.id.control_container);
        final ViewGroup containerParent = presenter.getContainerParentForTest();

        ThreadUtils.runOnUiThreadBlocking(() -> {
            Assert.assertTrue(containerParent.indexOfChild(dialogContainer)
                    > containerParent.indexOfChild(controlContainer));
        });

        // When editing URL, it should be shown on top of the dialog.
        onView(withId(R.id.url_bar)).perform(click());
        ThreadUtils.runOnUiThreadBlocking(() -> {
            Assert.assertTrue(containerParent.indexOfChild(dialogContainer)
                    < containerParent.indexOfChild(controlContainer));
        });

        // When URL bar is not focused, the dialog should be shown on top of the toolbar again.
        Espresso.pressBack();
        ThreadUtils.runOnUiThreadBlocking(() -> {
            Assert.assertTrue(containerParent.indexOfChild(dialogContainer)
                    > containerParent.indexOfChild(controlContainer));
        });

        // Dismiss the dialog by clicking OK.
        onView(withText(R.string.ok)).perform(click());
    }

    @Test
    @SmallTest
    public void testDismiss_ToggleOverview() throws Exception {
        // Initially there is no dialogs in the pending list. Browser controls are not restricted.
        checkPendingSize(0);
        checkBrowserControls(false);

        // Add two dialogs available for showing.
        showDialog(0);
        showDialog(1);
        checkPendingSize(1);
        onView(withId(R.id.tab_modal_dialog_container))
                .check(matches(
                        allOf(hasDescendant(withText("0")), not(hasDescendant(withText("1"))))));
        checkBrowserControls(true);

        // Dialogs should all be dismissed on entering tab switcher.
        onView(withId(R.id.tab_switcher_button)).perform(click());
        checkPendingSize(0);
        onView(withId(R.id.tab_modal_dialog_container))
                .check(matches(allOf(
                        not(hasDescendant(withText("0"))), not(hasDescendant(withText("1"))))));
        checkBrowserControls(false);
    }

    @Test
    @SmallTest
    public void testDismiss_BackPressed() throws Exception {
        // Initially there is no dialogs in the pending list. Browser controls are not restricted.
        checkPendingSize(0);
        checkBrowserControls(false);

        // Add two dialogs available for showing.
        showDialog(0);
        showDialog(1);
        checkPendingSize(1);
        onView(withId(R.id.tab_modal_dialog_container))
                .check(matches(
                        allOf(hasDescendant(withText("0")), not(hasDescendant(withText("1"))))));
        checkBrowserControls(true);

        // Perform back press. The first dialog should be dismissed.
        // The second dialog should be shown.
        Espresso.pressBack();
        checkPendingSize(0);
        onView(withId(R.id.tab_modal_dialog_container))
                .check(matches(
                        allOf(not(hasDescendant(withText("0"))), hasDescendant(withText("1")))));
        checkBrowserControls(true);

        // Perform a second back press. The second dialog should be dismissed.
        Espresso.pressBack();
        checkPendingSize(0);
        onView(withId(R.id.tab_modal_dialog_container))
                .check(matches(allOf(
                        not(hasDescendant(withText("0"))), not(hasDescendant(withText("1"))))));
        checkBrowserControls(false);
    }

    private ModalDialogView createDialog(final int index) throws Exception {
        return ThreadUtils.runOnUiThreadBlocking(new Callable<ModalDialogView>() {
            @Override
            public ModalDialogView call() {
                ModalDialogView.Controller controller = new ModalDialogView.Controller() {
                    @Override
                    public void onCancel() {
                        dismissDialog(index);
                    }

                    @Override
                    public void onClick(int buttonType) {
                        switch (buttonType) {
                            case ModalDialogView.BUTTON_POSITIVE:
                            case ModalDialogView.BUTTON_NEGATIVE:
                                dismissDialog(index);
                                break;
                            default:
                                Assert.fail("Unknown button type: " + buttonType);
                        }
                    }
                };
                final ModalDialogView dialog = new ModalDialogView(controller);
                dialog.setTitle(Integer.toString(index));
                dialog.setPositiveButton(R.string.ok);
                dialog.setNegativeButton(R.string.cancel);
                return dialog;
            }
        });
    }

    private void showDialog(final int index) {
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mManager.showDialog(mModalDialogViews[index], ModalDialogManager.TAB_MODAL);
        });
    }

    private void dismissDialog(final int index) {
        ThreadUtils.runOnUiThreadBlocking(
                () -> { mManager.dismissDialog(mModalDialogViews[index]); });
    }

    private void checkPendingSize(final int expected) {
        ThreadUtils.runOnUiThreadBlocking(() -> {
            Assert.assertEquals(expected, mManager.getPendingDialogsForTest().size());
        });
    }

    private void checkBrowserControls(boolean restricted) {
        if (restricted) {
            Assert.assertTrue(mManager.isShowing());
            Assert.assertTrue("All tabs should be obscured", mActivity.isViewObscuringAllTabs());
            onView(withId(R.id.menu_button)).check(matches(not(isEnabled())));
        } else {
            Assert.assertFalse(mManager.isShowing());
            Assert.assertFalse("Tabs shouldn't be obscured", mActivity.isViewObscuringAllTabs());
            onView(withId(R.id.menu_button)).check(matches(isEnabled()));
        }
    }
}
