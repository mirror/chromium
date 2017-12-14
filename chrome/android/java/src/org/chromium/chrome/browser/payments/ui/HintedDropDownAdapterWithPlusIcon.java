// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments.ui;

import android.content.Context;
import android.content.res.Resources;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.TintedDrawable;
import org.chromium.ui.UiUtils;

import java.util.List;

/**
 * Subclass of DropdownFieldAdapter used to display a dropdown hint that won't appear in the
 * expanded dropdown options but will be used when no element is selected. The last
 * shown element will have a "+" icon on its left and a blue tint to indicate the option to add an
 * element.
 *
 * @param <T> The type of element to be inserted into the adapter.
 *
 *
 * collapsed view:       --------          Expanded view:   ------------
 * (no item selected)   | hint   |                         | option 1   |
 *                       --------                          |------------|
 *                                                         | option 2   |
 * collapsed view:       ----------                        |------------|
 * (with selected item) | option X |                       | + option N | -> stylized and "+" icon
 *                       ----------                        .------------.
 *                                                         . hint       . -> hidden
 *                                                         ..............
 */
public class HintedDropDownAdapterWithPlusIcon<T> extends HintedDropDownAdapter<T> {
    /**
     * Creates an array adapter for which the last element is a hint that is not shown in the
     * expanded view and where the last shown element has a "+" icon on its left and
     * has a blue tint.
     *
     * @param context            The current context.
     * @param resource           The resource ID for a layout file containing a layout to use when
     *                           instantiating views.
     * @param mTextViewResourceId The id of the TextView within the layout resource to be populated.
     * @param objects            The objects to represent in the ListView, the last
     *                           item will have a "+" icon on its left and will have a blue tint.
     * @param hint               The element to be used as a hint when no element is selected. It is
     *                           not taken into account in the count function and thus will not be
     *                           displayed when in the expanded dropdown view.
     */
    public HintedDropDownAdapterWithPlusIcon(
            Context context, int resource, int textViewResourceId, List<T> objects, T hint) {
        super(context, resource, textViewResourceId, objects, hint);
    }

    @Override
    public View getDropDownView(int position, View convertView, ViewGroup parent) {
        convertView = super.getDropDownView(position, convertView, parent);

        // The plus icon is for the last item on the list.
        if (position == getCount() - 1) {
            // Add a "+" icon and a blue tint to the last element.
            Resources resources = getContext().getResources();
            if (mTextView == null) {
                mTextView = (TextView) convertView.findViewById(mTextViewResourceId);
            }

            // Create the "+" icon, put it left of the text and add appropriate padding.
            mTextView.setCompoundDrawablesWithIntrinsicBounds(
                    TintedDrawable.constructTintedDrawable(
                            resources, R.drawable.plus, R.color.light_active_color),
                    null, null, null);
            mTextView.setCompoundDrawablePadding(
                    resources.getDimensionPixelSize(R.dimen.payments_section_large_spacing));

            // Set the correct appearance, face and style for the text.
            ApiCompatibilityUtils.setTextAppearance(
                    mTextView, R.style.PaymentsUiSectionAddButtonLabel);
            mTextView.setTypeface(UiUtils.createRobotoMediumTypeface());

            // Padding at the bottom of the dropdown.
            ApiCompatibilityUtils.setPaddingRelative(convertView,
                    ApiCompatibilityUtils.getPaddingStart(convertView), convertView.getPaddingTop(),
                    ApiCompatibilityUtils.getPaddingEnd(convertView),
                    getContext().getResources().getDimensionPixelSize(
                            R.dimen.payments_section_small_spacing));
        }

        return convertView;
    }
}
