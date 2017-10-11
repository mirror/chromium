// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.os.Build;
import android.os.Bundle;
import android.os.LocaleList;
import android.os.Parcel;
import android.util.Pair;
import android.util.SparseArray;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStructure;
import android.view.ViewStructure.HtmlInfo.Builder;
import android.view.autofill.AutofillId;
import android.view.autofill.AutofillValue;

import org.junit.Assert;

import org.chromium.android_webview.AwAutofillManager;
import org.chromium.android_webview.AwAutofillProvider;
import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.test.AwTestBase.TestDependencyFactory;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.components.autofill.AutofillProvider;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.TimeoutException;

/**
 * A test rule for Autofill related activity (instrumentation) tests.
 */
@MinAndroidSdkLevel(Build.VERSION_CODES.O)
@SuppressLint("NewApi")
class AwAutofillActivityTestRule extends AwActivityTestRule {
    public final static int AUTOFILL_VIEW_ENTERED = 1;
    public final static int AUTOFILL_VIEW_EXITED = 2;
    public final static int AUTOFILL_VALUE_CHANGED = 3;
    public final static int AUTOFILL_COMMIT = 4;
    public final static int AUTOFILL_CANCEL = 5;

    private AwTestContainerView mTestContainerView;
    private TestAwContentsClient mContentsClient;
    private TestAwAutofillManager mManager;
    private CallbackHelper mCallbackHelper = new CallbackHelper();
    private AwContents mAwContents;
    private TestViewStructure mTestViewStructure;
    private ArrayList<Pair<Integer, AutofillValue>> mChangedValues;
    private ConcurrentLinkedQueue<Integer> mEventQueue = new ConcurrentLinkedQueue<>();

    void setUpForAutofill() throws Exception {
        mContentsClient = new TestAwContentsClient();
        mTestContainerView = createAwTestContainerViewOnMainSync(
                mContentsClient, false, new TestDependencyFactory() {
                    @Override
                    public AutofillProvider createAutofillProvider(
                            Context context, ViewGroup containerView) {
                        mManager = new TestAwAutofillManager(context);
                        return new AwAutofillProvider(containerView, mManager);
                    }
                });
        mAwContents = mTestContainerView.getAwContents();
        enableJavaScriptOnUiThread(mAwContents);
    }

    public void loadUrlSync(String url) throws Exception {
        loadUrlSync(
                mTestContainerView.getAwContents(), mContentsClient.getOnPageFinishedHelper(), url);
    }

    public String executeJavaScriptAndWaitForResult(String code) throws Throwable {
        return executeJavaScriptAndWaitForResult(
                mTestContainerView.getAwContents(), mContentsClient, code);
    }

    public ArrayList<Pair<Integer, AutofillValue>> getChangedValues() {
        return mChangedValues;
    }

    public void clearChangedValues() {
        if (mChangedValues != null) mChangedValues.clear();
    }

    public void invokeOnProvideAutoFillVirtualStructure() {
        mTestViewStructure = new TestViewStructure();
        mAwContents.onProvideAutoFillVirtualStructure(mTestViewStructure, 1);
    }

    public void invokeAutofill(SparseArray<AutofillValue> values) {
        mAwContents.autofill(values);
    }

    public TestViewStructure getTestViewStructure() {
        return mTestViewStructure;
    }

    public int getCallbackCount() {
        return mCallbackHelper.getCallCount();
    }

    /**
     * Wait for expected callbacks to be called, and verify the types.
     *
     * @param currentCallCount The current call count to start from.
     * @param expectedEventArray The callback types that need to be verified.
     * @return The number of new callbacks since currentCallCount. This should be same as the length
     *         of expectedEventArray.
     * @throws InterruptedException
     * @throws TimeoutException
     */
    public int waitForCallbackAndVerifyTypes(int currentCallCount, Integer[] expectedEventArray)
            throws InterruptedException, TimeoutException {
        // Check against the call count to avoid missing out a callback in between waits, while
        // exposing it so that the test can control where the call count starts.
        mCallbackHelper.waitForCallback(currentCallCount, expectedEventArray.length);
        Object[] objectArray = mEventQueue.toArray();
        mEventQueue.clear();
        Integer[] resultArray = Arrays.copyOf(objectArray, objectArray.length, Integer[].class);
        Assert.assertArrayEquals(Arrays.toString(resultArray), expectedEventArray, resultArray);
        return expectedEventArray.length;
    }

    public void dispatchDownAndUpKeyEvents(final int code) throws Throwable {
        dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, code));
        dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, code));
    }

    public boolean dispatchKeyEvent(final KeyEvent event) throws Throwable {
        return runTestOnUiThreadAndGetResult(new Callable<Boolean>() {
            @Override
            public Boolean call() {
                return mTestContainerView.dispatchKeyEvent(event);
            }
        });
    }

    /**
     * This class only implements the necessary methods of ViewStructure for testing.
     */
    static class TestViewStructure extends ViewStructure {
        /**
         * Implementation of HtmlInfo.
         */
        public static class AwHtmlInfo extends HtmlInfo {
            private String mTag;
            private List<Pair<String, String>> mAttributes;
            public AwHtmlInfo(String tag, List<Pair<String, String>> attributes) {
                mTag = tag;
                mAttributes = attributes;
            }

            @Override
            public List<Pair<String, String>> getAttributes() {
                return mAttributes;
            }

            public String getAttribute(String attribute) {
                for (Pair<String, String> pair : mAttributes) {
                    if (attribute.equals(pair.first)) {
                        return pair.second;
                    }
                }
                return null;
            }

            @Override
            public String getTag() {
                return mTag;
            }
        }

        /**
         * Implementation of Builder
         */
        public static class AwBuilder extends Builder {
            private String mTag;
            private ArrayList<Pair<String, String>> mAttributes;
            public AwBuilder(String tag) {
                mTag = tag;
                mAttributes = new ArrayList<Pair<String, String>>();
            }

            @Override
            public Builder addAttribute(String name, String value) {
                mAttributes.add(new Pair<String, String>(name, value));
                return this;
            }

            @Override
            public HtmlInfo build() {
                return new AwHtmlInfo(mTag, mAttributes);
            }
        }

        public TestViewStructure() {
            mChildren = new ArrayList<TestViewStructure>();
        }

        @Override
        public void setAlpha(float alpha) {}

        @Override
        public void setAccessibilityFocused(boolean state) {}

        @Override
        public void setCheckable(boolean state) {}

        @Override
        public void setChecked(boolean state) {
            mChecked = state;
        }

        public boolean getChecked() {
            return mChecked;
        }

        @Override
        public void setActivated(boolean state) {}

        @Override
        public CharSequence getText() {
            return null;
        }

        @Override
        public int getTextSelectionStart() {
            return 0;
        }

        @Override
        public int getTextSelectionEnd() {
            return 0;
        }

        @Override
        public CharSequence getHint() {
            return mHint;
        }

        @Override
        public Bundle getExtras() {
            return null;
        }

        @Override
        public boolean hasExtras() {
            return false;
        }

        @Override
        public void setChildCount(int num) {}

        @Override
        public int addChildCount(int num) {
            int index = mChildCount;
            mChildCount += num;
            mChildren.ensureCapacity(mChildCount);
            return index;
        }

        @Override
        public int getChildCount() {
            return mChildren.size();
        }

        @Override
        public ViewStructure newChild(int index) {
            TestViewStructure viewStructure = new TestViewStructure();
            if (index < mChildren.size()) {
                mChildren.set(index, viewStructure);
            } else if (index == mChildren.size()) {
                mChildren.add(viewStructure);
            } else {
                // Failed intentionally, we shouldn't run into this case.
                mChildren.add(index, viewStructure);
            }
            return viewStructure;
        }

        public TestViewStructure getChild(int index) {
            return mChildren.get(index);
        }

        @Override
        public ViewStructure asyncNewChild(int index) {
            return null;
        }

        @Override
        public void asyncCommit() {}

        @Override
        public AutofillId getAutofillId() {
            Parcel parcel = Parcel.obtain();
            // Check AutofillId(Parcel) in AutofillId.java, always set parent id as 0.
            parcel.writeInt(0);
            parcel.writeInt(1);
            parcel.writeInt(mId);

            return AutofillId.CREATOR.createFromParcel(parcel);
        }

        @Override
        public Builder newHtmlInfoBuilder(String tag) {
            return new AwBuilder(tag);
        }

        @Override
        public void setAutofillHints(String[] arg0) {
            mAutofillHints = arg0.clone();
        }

        public String[] getAutofillHints() {
            if (mAutofillHints == null) return null;
            return mAutofillHints.clone();
        }

        @Override
        public void setAutofillId(AutofillId arg0) {}

        @Override
        public void setAutofillId(AutofillId arg0, int arg1) {
            mId = arg1;
        }

        public int getId() {
            return mId;
        }

        @Override
        public void setAutofillOptions(CharSequence[] arg0) {
            mAutofillOptions = arg0.clone();
        }

        public CharSequence[] getAutofillOptions() {
            if (mAutofillOptions == null) return null;
            return mAutofillOptions.clone();
        }

        @Override
        public void setAutofillType(int arg0) {
            mAutofillType = arg0;
        }

        public int getAutofillType() {
            return mAutofillType;
        }

        @Override
        public void setAutofillValue(AutofillValue arg0) {
            mAutofillValue = arg0;
        }

        public AutofillValue getAutofillValue() {
            return mAutofillValue;
        }

        @Override
        public void setId(int id, String packageName, String typeName, String entryName) {}

        @Override
        public void setDimens(int left, int top, int scrollX, int scrollY, int width, int height) {}

        @Override
        public void setElevation(float elevation) {}

        @Override
        public void setEnabled(boolean state) {}

        @Override
        public void setClickable(boolean state) {}

        @Override
        public void setLongClickable(boolean state) {}

        @Override
        public void setContextClickable(boolean state) {}

        @Override
        public void setFocusable(boolean state) {}

        @Override
        public void setFocused(boolean state) {}

        @Override
        public void setClassName(String className) {
            mClassName = className;
        }

        public String getClassName() {
            return mClassName;
        }

        @Override
        public void setContentDescription(CharSequence contentDescription) {}

        @Override
        public void setDataIsSensitive(boolean arg0) {
            mDataIsSensitive = arg0;
        }

        public boolean getDataIsSensitive() {
            return mDataIsSensitive;
        }

        @Override
        public void setHint(CharSequence hint) {
            mHint = hint;
        }

        @Override
        public void setHtmlInfo(HtmlInfo arg0) {
            mAwHtmlInfo = (AwHtmlInfo) arg0;
        }

        public AwHtmlInfo getHtmlInfo() {
            return mAwHtmlInfo;
        }

        @Override
        public void setInputType(int arg0) {}

        @Override
        public void setLocaleList(LocaleList arg0) {}

        @Override
        public void setOpaque(boolean arg0) {}

        @Override
        public void setTransformation(Matrix matrix) {}

        @Override
        public void setVisibility(int visibility) {}

        @Override
        public void setSelected(boolean state) {}

        @Override
        public void setText(CharSequence text) {}

        @Override
        public void setText(CharSequence text, int selectionStart, int selectionEnd) {}

        @Override
        public void setTextStyle(float size, int fgColor, int bgColor, int style) {}

        @Override
        public void setTextLines(int[] charOffsets, int[] baselines) {}

        @Override
        public void setWebDomain(String webDomain) {
            mWebDomain = webDomain;
        }

        public String getWebDomain() {
            return mWebDomain;
        }

        private int mAutofillType;
        private CharSequence mHint;
        private String[] mAutofillHints;
        private int mId;
        private String mClassName;
        private String mWebDomain;
        private int mChildCount;
        private ArrayList<TestViewStructure> mChildren;
        private CharSequence[] mAutofillOptions;
        private AutofillValue mAutofillValue;
        private boolean mDataIsSensitive;
        private AwHtmlInfo mAwHtmlInfo;
        private boolean mChecked;
    }

    private class TestAwAutofillManager extends AwAutofillManager {
        public TestAwAutofillManager(Context context) {
            super(context);
        }

        @Override
        public void notifyVirtualViewEntered(View parent, int childId, Rect absBounds) {
            mEventQueue.add(AUTOFILL_VIEW_ENTERED);
            mCallbackHelper.notifyCalled();
        }

        @Override
        public void notifyVirtualViewExited(View parent, int childId) {
            mEventQueue.add(AUTOFILL_VIEW_EXITED);
            mCallbackHelper.notifyCalled();
        }

        @Override
        public void notifyVirtualValueChanged(View parent, int childId, AutofillValue value) {
            if (mChangedValues == null) {
                mChangedValues = new ArrayList<Pair<Integer, AutofillValue>>();
            }
            mChangedValues.add(new Pair<Integer, AutofillValue>(childId, value));
            mEventQueue.add(AUTOFILL_VALUE_CHANGED);
            mCallbackHelper.notifyCalled();
        }

        @Override
        public void commit() {
            mEventQueue.add(AUTOFILL_COMMIT);
            mCallbackHelper.notifyCalled();
        }

        @Override
        public void cancel() {
            mEventQueue.add(AUTOFILL_CANCEL);
            mCallbackHelper.notifyCalled();
        }
    }
}
