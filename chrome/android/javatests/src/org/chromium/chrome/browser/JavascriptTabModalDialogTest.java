// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.support.test.filters.MediumTest;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.modaldialog.ModalDialog;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.TestCallbackHelperContainer.OnEvaluateJavaScriptResultHelper;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.ui.base.PageTransition;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeoutException;

/**
 * Test suite for displaying and functioning of JavaScript tab modal dialogs. Some of the tests
 * are identical to JavascriptAppModalDialogTest.java and some are for tab-modal features.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@RetryOnFailure
@CommandLineFlags.
Add({"enable-features=TabModalJsDialog", ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG})
public class JavascriptTabModalDialogTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    private static final String TAG = "JsTabModalDialogTest";
    private static final String EMPTY_PAGE = UrlUtils.encodeHtmlDataUri(
            "<html><title>Modal Dialog Test</title><p>Testcase.</p></title></html>");
    private static final String OTHER_PAGE = UrlUtils.encodeHtmlDataUri(
            "<html><title>Modal Dialog Test</title><p>Testcase. Other tab.</p></title></html>");

    private ChromeTabbedActivity mActivity;

    @Before
    public void setUp() throws InterruptedException {
        mActivityTestRule.startMainActivityWithURL(EMPTY_PAGE);
        mActivity = mActivityTestRule.getActivity();
    }

    /**
     * Verifies modal alert-dialog appearance and that JavaScript execution is
     * able to continue after dismissal.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testAlertModalDialog()
            throws InterruptedException, TimeoutException, ExecutionException {
        final OnEvaluateJavaScriptResultHelper scriptEvent =
                executeJavaScriptAndWaitForDialog("alert('Hello Android!');");

        JavascriptTabModalDialog jsDialog = getCurrentDialog();
        Assert.assertNotNull("No dialog showing.", jsDialog);

        clickOk(jsDialog);
        Assert.assertTrue("JavaScript execution should continue after closing prompt.",
                scriptEvent.waitUntilHasValue());
    }

    /**
     * Verifies that clicking on a button twice doesn't crash.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testAlertModalDialogWithTwoClicks()
            throws InterruptedException, TimeoutException, ExecutionException {
        OnEvaluateJavaScriptResultHelper scriptEvent =
                executeJavaScriptAndWaitForDialog("alert('Hello Android');");
        JavascriptTabModalDialog jsDialog = getCurrentDialog();
        Assert.assertNotNull("No dialog showing.", jsDialog);

        clickOk(jsDialog);
        clickOk(jsDialog);

        Assert.assertTrue("JavaScript execution should continue after closing prompt.",
                scriptEvent.waitUntilHasValue());
    }

    /**
     * Verifies that modal confirm-dialogs display, two buttons are visible and
     * the return value of [Ok] equals true, [Cancel] equals false.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testConfirmModalDialog()
            throws InterruptedException, TimeoutException, ExecutionException {
        OnEvaluateJavaScriptResultHelper scriptEvent =
                executeJavaScriptAndWaitForDialog("confirm('Android');");

        JavascriptTabModalDialog jsDialog = getCurrentDialog();
        Assert.assertNotNull("No dialog showing.", jsDialog);

        Button[] buttons = getAlertDialogButtons(jsDialog.getDialogForTest());
        Assert.assertNotNull("No cancel button in confirm dialog.", buttons[0]);
        Assert.assertEquals(
                "Cancel button is not visible.", View.VISIBLE, buttons[0].getVisibility());
        Assert.assertNotNull("No OK button in confirm dialog.", buttons[1]);
        Assert.assertEquals("OK button is not visible.", View.VISIBLE, buttons[1].getVisibility());

        clickOk(jsDialog);
        Assert.assertTrue("JavaScript execution should continue after closing dialog.",
                scriptEvent.waitUntilHasValue());

        String resultString = scriptEvent.getJsonResultAndClear();
        Assert.assertEquals("Invalid return value.", "true", resultString);

        // Try again, pressing cancel this time.
        scriptEvent = executeJavaScriptAndWaitForDialog("confirm('Android');");
        jsDialog = getCurrentDialog();
        Assert.assertNotNull("No dialog showing.", jsDialog);

        clickCancel(jsDialog);
        Assert.assertTrue("JavaScript execution should continue after closing dialog.",
                scriptEvent.waitUntilHasValue());

        resultString = scriptEvent.getJsonResultAndClear();
        Assert.assertEquals("Invalid return value.", "false", resultString);
    }

    /**
     * Verifies that modal prompt-dialogs display and the result is returned.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testPromptModalDialog()
            throws InterruptedException, TimeoutException, ExecutionException {
        final String promptText = "Hello Android!";
        final OnEvaluateJavaScriptResultHelper scriptEvent =
                executeJavaScriptAndWaitForDialog("prompt('Android', 'default');");

        final JavascriptTabModalDialog jsDialog = getCurrentDialog();
        Assert.assertNotNull("No dialog showing.", jsDialog);

        // Set the text in the prompt field of the dialog.
        boolean result = ThreadUtils.runOnUiThreadBlocking(() -> {
            EditText prompt = jsDialog.getDialogForTest().getDialogView().findViewById(
                    org.chromium.chrome.R.id.js_modal_dialog_prompt);
            if (prompt == null) return false;
            prompt.setText(promptText);
            return true;
        });
        Assert.assertTrue("Failed to find prompt view in prompt dialog.", result);

        clickOk(jsDialog);
        Assert.assertTrue("JavaScript execution should continue after closing prompt.",
                scriptEvent.waitUntilHasValue());

        String resultString = scriptEvent.getJsonResultAndClear();
        Assert.assertEquals("Invalid return value.", '"' + promptText + '"', resultString);
    }

    /**
     * Verifies that message content in a dialog is only focusable if the message itself is long
     * enough to require scrolling.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testAlertModalDialogMessageFocus()
            throws InterruptedException, TimeoutException, ExecutionException {
        assertScrollViewFocusabilityInAlertDialog("alert('Short message!');", false);

        assertScrollViewFocusabilityInAlertDialog(
                "alert(new Array(200).join('Long message!'));", true);
    }

    private void assertScrollViewFocusabilityInAlertDialog(
            final String jsAlertScript, final boolean expectedFocusability)
            throws InterruptedException, TimeoutException, ExecutionException {
        final OnEvaluateJavaScriptResultHelper scriptEvent =
                executeJavaScriptAndWaitForDialog(jsAlertScript);

        final JavascriptTabModalDialog jsDialog = getCurrentDialog();
        Assert.assertNotNull("No dialog showing.", jsDialog);

        final String errorMessage =
                "Scroll view focusability was incorrect. Expected: " + expectedFocusability;

        CriteriaHelper.pollUiThread(new Criteria(errorMessage) {
            @Override
            public boolean isSatisfied() {
                return jsDialog.getDialogForTest()
                               .getDialogView()
                               .findViewById(R.id.js_modal_dialog_scroll_view)
                               .isFocusable()
                        == expectedFocusability;
            }
        });

        clickOk(jsDialog);
        Assert.assertTrue("JavaScript execution should continue after closing prompt.",
                scriptEvent.waitUntilHasValue());
    }

    /**
     * Displays a dialog and closes the tab in the background before attempting
     * to accept the dialog. Verifies that the dialog is dismissed when the tab
     * is closed.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testDialogDismissedAfterClosingTab() {
        executeJavaScriptAndWaitForDialog("alert('Android')");

        ThreadUtils.runOnUiThreadBlocking(
                () -> { mActivity.getCurrentTabModel().closeTab(mActivity.getActivityTab()); });

        // Closing the tab should have dismissed the dialog.
        checkDialogShowing("The dialog should have been dismissed when its tab was closed.", false);
    }

    /**
     * Displays a dialog and goes to tab switcher in the before attempting to accept or cancel the
     * dialog. Verifies that the dialog is dismissed.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testDialogDismissedAfterToggleOverview() {
        executeJavaScriptAndWaitForDialog("alert('Android')");

        ThreadUtils.runOnUiThreadBlocking(
                () -> { mActivity.findViewById(R.id.tab_switcher_button).performClick(); });

        // Entering tab switcher should have dismissed the dialog.
        checkDialogShowing(
                "The dialog should have been dismissed when switching to overview mode.", false);
    }

    /**
     * Displays a dialog and loads a new URL before attempting to accept or cancel the
     * dialog. Verifies that the dialog is dismissed.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testDialogDismissedAfterUrlUpdated() {
        executeJavaScriptAndWaitForDialog("alert('Android')");

        ThreadUtils.runOnUiThreadBlocking(() -> {
            mActivity.getActivityTab().loadUrl(new LoadUrlParams(OTHER_PAGE, PageTransition.LINK));
        });

        // Loading a different URL should have dismissed the dialog.
        checkDialogShowing(
                "The dialog should have been dismissed when a new url is loaded.", false);
    }

    /**
     * Displays a dialog and performs back press before attempting to accept or cancel the
     * dialog. Verifies that the dialog is dismissed.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testDialogDismissedAfterBackPressed() {
        executeJavaScriptAndWaitForDialog("alert('Android')");

        ThreadUtils.runOnUiThreadBlocking(() -> { mActivity.onBackPressed(); });

        // Performing back press should have dismissed the dialog.
        checkDialogShowing("The dialog should have been dismissed after back press.", false);
    }

    /**
     * Asynchronously executes the given code for spawning a dialog and waits
     * for the dialog to be visible.
     */
    private OnEvaluateJavaScriptResultHelper executeJavaScriptAndWaitForDialog(String script) {
        return executeJavaScriptAndWaitForDialog(new OnEvaluateJavaScriptResultHelper(), script);
    }

    /**
     * Given a JavaScript evaluation helper, asynchronously executes the given
     * code for spawning a dialog and waits for the dialog to be visible.
     */
    private OnEvaluateJavaScriptResultHelper executeJavaScriptAndWaitForDialog(
            final OnEvaluateJavaScriptResultHelper helper, String script) {
        helper.evaluateJavaScriptForTests(
                mActivity.getCurrentContentViewCore().getWebContents(), script);
        checkDialogShowing("Could not spawn or locate a modal dialog.", true);
        return helper;
    }

    /**
     * Returns an array of the 2 buttons for this dialog, in the order
     * BUTTON_NEGATIVE and BUTTON_POSITIVE. Any of these value can be null.
     */
    private Button[] getAlertDialogButtons(final ModalDialog dialog) throws ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(() -> {
            final Button[] buttons = new Button[3];
            buttons[0] = dialog.getButtonForTest(ModalDialog.BUTTON_NEGATIVE);
            buttons[1] = dialog.getButtonForTest(ModalDialog.BUTTON_POSITIVE);
            return buttons;
        });
    }

    /**
     * Returns the current JavaScript modal dialog showing or null if no such dialog is currently
     * showing.
     */
    private JavascriptTabModalDialog getCurrentDialog() throws ExecutionException {
        return (JavascriptTabModalDialog) ThreadUtils.runOnUiThreadBlocking(
                () -> mActivity.getModalDialogManager().getCurrentDialogForTest().getController());
    }

    /**
     * Check whether dialog is showing as expected.
     */
    private void checkDialogShowing(final String errorMessage, final boolean shouldBeShown) {
        CriteriaHelper.pollUiThread(new Criteria(errorMessage) {
            @Override
            public boolean isSatisfied() {
                try {
                    return ThreadUtils.runOnUiThreadBlocking(() -> {
                        final boolean isShown = mActivity.getModalDialogManager().isShowing();
                        return shouldBeShown == isShown;
                    });
                } catch (ExecutionException e) {
                    Log.e(TAG, "Failed to getCurrentDialog", e);
                    return false;
                }
            }
        });
    }

    /**
     * Simulates pressing the OK button of the passed dialog.
     */
    private void clickOk(JavascriptTabModalDialog dialog) {
        clickButton(dialog, ModalDialog.BUTTON_POSITIVE);
    }

    /**
     * Simulates pressing the Cancel button of the passed dialog.
     */
    private void clickCancel(JavascriptTabModalDialog dialog) {
        clickButton(dialog, ModalDialog.BUTTON_NEGATIVE);
    }

    private void clickButton(final JavascriptTabModalDialog dialog, final int whichButton) {
        ThreadUtils.runOnUiThreadBlocking(() -> dialog.onClick(whichButton));
    }
}
