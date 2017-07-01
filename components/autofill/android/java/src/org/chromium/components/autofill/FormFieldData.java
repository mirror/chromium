// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.autofill;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * The wrap class of native autofill::FormFieldDataAndroid.
 */
@JNINamespace("autofill")
public class FormFieldData {
    public final String mLabel;
    public final String mName;
    public final String mAutocompleteAttr;
    public final boolean mShouldAutocomplete;
    public final String mPlaceholder;
    public final String mType;
    public final String mId;
    public final String[] mOptionValues;
    public final String[] mOptionContents;
    public final boolean mIsCheckField;

    private boolean mIsChecked;
    private String mValue;

    private FormFieldData(String name, String label, String value, String autocompleteAttr,
        boolean shouldAutocomplete, String placeholder, String type, String id, String[] optionValues, String[] optionContents, boolean isCheckField, boolean isChecked) {
        mName = name;
        mLabel = label;
        mValue = value;
        mAutocompleteAttr = autocompleteAttr;
        mShouldAutocomplete = shouldAutocomplete;
        mPlaceholder = placeholder;
        mType = type;
        mId = id;
        mOptionValues = optionValues;
        mOptionContents = optionContents;
        mIsCheckField = isCheckField;
        mIsChecked = isChecked;
    }

    public boolean isSelectField() {
      return mOptionValues != null && mOptionValues.length != 0;
    }

    public boolean isCheckField() {
      return mIsCheckField;
    }

    /**
     * @return value of field.
     */
    @CalledByNative
    public String getValue() {
        return mValue;
    }

    @CalledByNative
    public void updateValue(String value) {
        mValue = value;
    }

    @CalledByNative
    public boolean isChecked() {
        return mIsChecked;
    }

    public void setChecked(boolean checked) {
        mIsChecked = checked;
    }

    @CalledByNative
    private static FormFieldData createFormFieldData(String name, String label, String value,
            String autocompleteAttr, boolean shouldAutocomplete, String placeholder, String type,
        String id, String[] optionValues, String[] optionContents, boolean isCheckField, boolean isChecked) {
        return new FormFieldData(
            name, label, value, autocompleteAttr, shouldAutocomplete, placeholder, type, id, optionValues, optionContents, isCheckField, isChecked);
    }
}
