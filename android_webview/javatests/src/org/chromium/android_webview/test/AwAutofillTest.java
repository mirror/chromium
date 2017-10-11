// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import android.annotation.SuppressLint;
import android.os.Build;
import android.support.test.filters.SmallTest;
import android.util.Pair;
import android.util.SparseArray;
import android.view.KeyEvent;
import android.view.View;
import android.view.autofill.AutofillValue;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import static org.chromium.android_webview.test.AwAutofillActivityTestRule.AUTOFILL_CANCEL;
import static org.chromium.android_webview.test.AwAutofillActivityTestRule.AUTOFILL_COMMIT;
import static org.chromium.android_webview.test.AwAutofillActivityTestRule.AUTOFILL_VALUE_CHANGED;
import static org.chromium.android_webview.test.AwAutofillActivityTestRule.AUTOFILL_VIEW_ENTERED;
import static org.chromium.android_webview.test.AwAutofillActivityTestRule.AUTOFILL_VIEW_EXITED;

import org.chromium.android_webview.test.AwAutofillActivityTestRule.TestViewStructure;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.net.test.util.TestWebServer;

import java.net.URL;
import java.util.ArrayList;

/**
 * Tests for WebView Autofill.
 */
@RunWith(AwJUnit4ClassRunner.class)
@MinAndroidSdkLevel(Build.VERSION_CODES.O)
@SuppressLint("NewApi")
public class AwAutofillTest {

    public static final String FILE = "/login.html";
    public static final String FILE_URL = "file:///android_asset/autofill.html";
    @Rule
    public AwAutofillActivityTestRule mRule = new AwAutofillActivityTestRule();

    @Before
    public void setUp() throws Exception {
        mRule.setUpForAutofill();
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testBasicAutofill() throws Throwable {
        TestWebServer webServer = TestWebServer.start();
        final String data = "<html><head></head><body><form action='a.html' name='formname'>"
                + "<input type='text' id='text1' name='username'"
                + " placeholder='placeholder@placeholder.com' autocomplete='username name'>"
                + "<input type='checkbox' id='checkbox1' name='showpassword'>"
                + "<select id='select1' name='month'>"
                + "<option value='1'>Jan</option>"
                + "<option value='2'>Feb</option>"
                + "</select>"
                + "<input type='submit'>"
                + "</form></body></html>";
        final int totalControls = 3;
        try {
            final String url = webServer.setResponse(FILE, data, null);
            mRule.loadUrlSync(url);
            int cnt = 0;
            mRule.executeJavaScriptAndWaitForResult("document.getElementById('text1').select();");
            cnt += mRule.waitForCallbackAndVerifyTypes(
                    cnt, new Integer[] {AUTOFILL_CANCEL, AUTOFILL_CANCEL});
            mRule.dispatchDownAndUpKeyEvents(KeyEvent.KEYCODE_A);
            cnt += mRule.waitForCallbackAndVerifyTypes(cnt,
                    new Integer[] {AUTOFILL_CANCEL, AUTOFILL_VIEW_ENTERED, AUTOFILL_VIEW_EXITED,
                            AUTOFILL_VIEW_ENTERED, AUTOFILL_VALUE_CHANGED});
            mRule.invokeOnProvideAutoFillVirtualStructure();
            TestViewStructure viewStructure = mRule.getTestViewStructure();
            assertNotNull(viewStructure);
            assertEquals(totalControls, viewStructure.getChildCount());

            // Verify form filled correctly in ViewStructure.
            URL pageURL = new URL(url);
            String webDomain =
                    new URL(pageURL.getProtocol(), pageURL.getHost(), pageURL.getPort(), "/")
                            .toString();
            assertEquals(webDomain, viewStructure.getWebDomain());
            // WebView shouldn't set class name.
            assertNull(viewStructure.getClassName());
            TestViewStructure.AwHtmlInfo htmlInfoForm = viewStructure.getHtmlInfo();
            assertEquals("form", htmlInfoForm.getTag());
            assertEquals("formname", htmlInfoForm.getAttribute("name"));

            // Verify input text control filled correctly in ViewStructure.
            TestViewStructure child0 = viewStructure.getChild(0);
            assertEquals(View.AUTOFILL_TYPE_TEXT, child0.getAutofillType());
            assertEquals("placeholder@placeholder.com", child0.getHint());
            assertEquals("username", child0.getAutofillHints()[0]);
            assertEquals("name", child0.getAutofillHints()[1]);
            TestViewStructure.AwHtmlInfo htmlInfo0 = child0.getHtmlInfo();
            assertEquals("text", htmlInfo0.getAttribute("type"));
            assertEquals("username", htmlInfo0.getAttribute("name"));

            // Verify checkbox control filled correctly in ViewStructure.
            TestViewStructure child1 = viewStructure.getChild(1);
            assertEquals(View.AUTOFILL_TYPE_TOGGLE, child1.getAutofillType());
            assertEquals("", child1.getHint());
            assertNull(child1.getAutofillHints());
            TestViewStructure.AwHtmlInfo htmlInfo1 = child1.getHtmlInfo();
            assertEquals("checkbox", htmlInfo1.getAttribute("type"));
            assertEquals("showpassword", htmlInfo1.getAttribute("name"));

            // Verify select control filled correctly in ViewStructure.
            TestViewStructure child2 = viewStructure.getChild(2);
            assertEquals(View.AUTOFILL_TYPE_LIST, child2.getAutofillType());
            assertEquals("", child2.getHint());
            assertNull(child2.getAutofillHints());
            TestViewStructure.AwHtmlInfo htmlInfo2 = child2.getHtmlInfo();
            assertEquals("month", htmlInfo2.getAttribute("name"));
            CharSequence[] options = child2.getAutofillOptions();
            assertEquals("Jan", options[0]);
            assertEquals("Feb", options[1]);

            // Autofill form and verify filled values.
            SparseArray<AutofillValue> values = new SparseArray<AutofillValue>();
            values.append(child0.getId(), AutofillValue.forText("example@example.com"));
            values.append(child1.getId(), AutofillValue.forToggle(true));
            values.append(child2.getId(), AutofillValue.forList(1));
            cnt = mRule.getCallbackCount();
            mRule.invokeAutofill(values);
            mRule.waitForCallbackAndVerifyTypes(cnt,
                    new Integer[] {AUTOFILL_VALUE_CHANGED, AUTOFILL_VALUE_CHANGED,
                            AUTOFILL_VALUE_CHANGED});

            // Verify form filled by Javascript
            String value0 = mRule.executeJavaScriptAndWaitForResult(
                    "document.getElementById('text1').value;");
            assertEquals("\"example@example.com\"", value0);
            String value1 = mRule.executeJavaScriptAndWaitForResult(
                    "document.getElementById('checkbox1').value;");
            assertEquals("\"on\"", value1);
            String value2 = mRule.executeJavaScriptAndWaitForResult(
                    "document.getElementById('select1').value;");
            assertEquals("\"2\"", value2);
        } finally {
            webServer.shutdown();
        }
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testNotifyVirtualValueChanged() throws Throwable {
        TestWebServer webServer = TestWebServer.start();
        final String data = "<html><head></head><body><form action='a.html' name='formname'>"
                + "<input type='text' id='text1' name='username'"
                + " placeholder='placeholder@placeholder.com' autocomplete='username name'>"
                + "</form></body></html>";
        try {
            final String url = webServer.setResponse(FILE, data, null);
            mRule.loadUrlSync(url);
            int cnt = 0;
            mRule.executeJavaScriptAndWaitForResult("document.getElementById('text1').select();");
            cnt += mRule.waitForCallbackAndVerifyTypes(
                    cnt, new Integer[] {AUTOFILL_CANCEL, AUTOFILL_CANCEL});

            mRule.dispatchDownAndUpKeyEvents(KeyEvent.KEYCODE_A);

            cnt += mRule.waitForCallbackAndVerifyTypes(cnt,
                    new Integer[] {AUTOFILL_CANCEL, AUTOFILL_VIEW_ENTERED, AUTOFILL_VIEW_EXITED,
                            AUTOFILL_VIEW_ENTERED, AUTOFILL_VALUE_CHANGED});
            ArrayList<Pair<Integer, AutofillValue>> values = mRule.getChangedValues();
            // Check if NotifyVirtualValueChanged() called and value is 'a'.
            assertEquals(1, values.size());
            assertEquals("a", values.get(0).second.getTextValue());

            mRule.dispatchDownAndUpKeyEvents(KeyEvent.KEYCODE_B);

            // Check if NotifyVirtualValueChanged() called again, first value is 'a',
            // second value is 'ab', and both time has the same id.
            mRule.waitForCallbackAndVerifyTypes(cnt,
                    new Integer[] {
                            AUTOFILL_VIEW_EXITED, AUTOFILL_VIEW_ENTERED, AUTOFILL_VALUE_CHANGED});
            values = mRule.getChangedValues();
            assertEquals(2, values.size());
            assertEquals("a", values.get(0).second.getTextValue());
            assertEquals("ab", values.get(1).second.getTextValue());
            assertEquals(values.get(0).first, values.get(1).first);
        } finally {
            webServer.shutdown();
        }
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testJavascriptNotTriggerNotifyVirtualValueChanged() throws Throwable {
        TestWebServer webServer = TestWebServer.start();
        final String data = "<html><head></head><body><form action='a.html' name='formname'>"
                + "<input type='text' id='text1' name='username'"
                + " placeholder='placeholder@placeholder.com' autocomplete='username name'>"
                + "</form></body></html>";
        try {
            final String url = webServer.setResponse(FILE, data, null);
            mRule.loadUrlSync(url);
            int cnt = 0;
            mRule.executeJavaScriptAndWaitForResult("document.getElementById('text1').select();");
            cnt += mRule.waitForCallbackAndVerifyTypes(
                    cnt, new Integer[] {AUTOFILL_CANCEL, AUTOFILL_CANCEL});
            mRule.dispatchDownAndUpKeyEvents(KeyEvent.KEYCODE_A);
            cnt += mRule.waitForCallbackAndVerifyTypes(cnt,
                    new Integer[] {AUTOFILL_CANCEL, AUTOFILL_VIEW_ENTERED, AUTOFILL_VIEW_EXITED,
                            AUTOFILL_VIEW_ENTERED, AUTOFILL_VALUE_CHANGED});
            ArrayList<Pair<Integer, AutofillValue>> values = mRule.getChangedValues();
            // Check if NotifyVirtualValueChanged() called and value is 'a'.
            assertEquals(1, values.size());
            assertEquals("a", values.get(0).second.getTextValue());
            mRule.executeJavaScriptAndWaitForResult("document.getElementById('text1').value='c';");
            assertEquals(7, mRule.getCallbackCount());
            mRule.dispatchDownAndUpKeyEvents(KeyEvent.KEYCODE_B);
            // Check if NotifyVirtualValueChanged() called one more time and value is 'cb', this
            // means javascript change didn't trigger the NotifyVirtualValueChanged().
            mRule.waitForCallbackAndVerifyTypes(cnt,
                    new Integer[] {
                            AUTOFILL_VIEW_EXITED, AUTOFILL_VIEW_ENTERED, AUTOFILL_VALUE_CHANGED});
            values = mRule.getChangedValues();
            assertEquals(2, values.size());
            assertEquals("a", values.get(0).second.getTextValue());
            assertEquals("cb", values.get(1).second.getTextValue());
            assertEquals(values.get(0).first, values.get(1).first);
        } finally {
            webServer.shutdown();
        }
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testCommit() throws Throwable {
        TestWebServer webServer = TestWebServer.start();
        final String data =
                "<html><head></head><body><form action='a.html' name='formname' id='formid'>"
                + "<input type='text' id='text1' name='username'"
                + " placeholder='placeholder@placeholder.com' autocomplete='username name'>"
                + "<input type='password' id='passwordid' name='passwordname'"
                + "<input type='submit'>"
                + "</form></body></html>";
        try {
            final String url = webServer.setResponse(FILE, data, null);
            mRule.loadUrlSync(url);
            int cnt = 0;
            mRule.executeJavaScriptAndWaitForResult("document.getElementById('text1').select();");
            cnt += mRule.waitForCallbackAndVerifyTypes(
                    cnt, new Integer[] {AUTOFILL_CANCEL, AUTOFILL_CANCEL});
            mRule.dispatchDownAndUpKeyEvents(KeyEvent.KEYCODE_A);
            cnt += mRule.waitForCallbackAndVerifyTypes(cnt,
                    new Integer[] {AUTOFILL_CANCEL, AUTOFILL_VIEW_ENTERED, AUTOFILL_VIEW_EXITED,
                            AUTOFILL_VIEW_ENTERED, AUTOFILL_VALUE_CHANGED});
            mRule.invokeOnProvideAutoFillVirtualStructure();
            // Fill the password.
            mRule.executeJavaScriptAndWaitForResult(
                    "document.getElementById('passwordid').select();");
            cnt += mRule.waitForCallbackAndVerifyTypes(cnt,
                    new Integer[] {
                            AUTOFILL_VIEW_EXITED, AUTOFILL_VIEW_ENTERED, AUTOFILL_VALUE_CHANGED});
            mRule.dispatchDownAndUpKeyEvents(KeyEvent.KEYCODE_B);
            cnt += mRule.waitForCallbackAndVerifyTypes(cnt,
                    new Integer[] {
                            AUTOFILL_VIEW_EXITED, AUTOFILL_VIEW_ENTERED, AUTOFILL_VALUE_CHANGED});
            mRule.clearChangedValues();
            // Submit form.
            mRule.executeJavaScriptAndWaitForResult("document.getElementById('formid').submit();");
            mRule.waitForCallbackAndVerifyTypes(cnt,
                    new Integer[] {AUTOFILL_VALUE_CHANGED, AUTOFILL_VALUE_CHANGED, AUTOFILL_COMMIT,
                            AUTOFILL_CANCEL});
            ArrayList<Pair<Integer, AutofillValue>> values = mRule.getChangedValues();
            assertEquals(2, values.size());
            assertEquals("a", values.get(0).second.getTextValue());
            assertEquals("b", values.get(1).second.getTextValue());
        } finally {
            webServer.shutdown();
        }
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testLoadFileURL() throws Throwable {
        int cnt = 0;
        mRule.loadUrlSync(FILE_URL);
        mRule.executeJavaScriptAndWaitForResult("document.getElementById('text1').select();");
        cnt += mRule.waitForCallbackAndVerifyTypes(
                cnt, new Integer[] {AUTOFILL_CANCEL, AUTOFILL_CANCEL});
        mRule.dispatchDownAndUpKeyEvents(KeyEvent.KEYCODE_A);
        // Cancel called for the first query.
        mRule.waitForCallbackAndVerifyTypes(cnt,
                new Integer[] {AUTOFILL_CANCEL, AUTOFILL_VIEW_ENTERED, AUTOFILL_VIEW_EXITED,
                        AUTOFILL_VIEW_ENTERED, AUTOFILL_VALUE_CHANGED});
    }

    @Test
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testMovingToOtherForm() throws Throwable {
        TestWebServer webServer = TestWebServer.start();
        final String data =
                "<html><head></head><body><form action='a.html' name='formname' id='formid'>"
                + "<input type='text' id='text1' name='username'"
                + " placeholder='placeholder@placeholder.com' autocomplete='username name'>"
                + "<input type='submit'></form>"
                + "<form action='a.html' name='formname' id='formid2'>"
                + "<input type='text' id='text2' name='username'"
                + " placeholder='placeholder@placeholder.com' autocomplete='username name'>"
                + "<input type='submit'>"
                + "</form></body></html>";
        try {
            int cnt = 0;
            final String url = webServer.setResponse(FILE, data, null);
            mRule.loadUrlSync(url);
            mRule.executeJavaScriptAndWaitForResult("document.getElementById('text1').select();");
            cnt += mRule.waitForCallbackAndVerifyTypes(
                    cnt, new Integer[] {AUTOFILL_CANCEL, AUTOFILL_CANCEL});
            mRule.dispatchDownAndUpKeyEvents(KeyEvent.KEYCODE_A);
            cnt += mRule.waitForCallbackAndVerifyTypes(cnt,
                    new Integer[] {AUTOFILL_CANCEL, AUTOFILL_VIEW_ENTERED, AUTOFILL_VIEW_EXITED,
                            AUTOFILL_VIEW_ENTERED, AUTOFILL_VALUE_CHANGED});
            // Move to form2, cancel() should be called again.
            mRule.executeJavaScriptAndWaitForResult("document.getElementById('text2').select();");
            cnt += mRule.waitForCallbackAndVerifyTypes(cnt, new Integer[] {AUTOFILL_VIEW_EXITED});
            mRule.dispatchDownAndUpKeyEvents(KeyEvent.KEYCODE_A);
            mRule.waitForCallbackAndVerifyTypes(cnt,
                    new Integer[] {AUTOFILL_CANCEL, AUTOFILL_VIEW_ENTERED, AUTOFILL_VIEW_EXITED,
                            AUTOFILL_VIEW_ENTERED, AUTOFILL_VALUE_CHANGED});
        } finally {
            webServer.shutdown();
        }
    }
}
