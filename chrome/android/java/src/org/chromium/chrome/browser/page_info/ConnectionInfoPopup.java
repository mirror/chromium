// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.page_info;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Color;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ResourceId;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;

/**
 * Java side of Android implementation of the page info UI.
 */
public class ConnectionInfoPopup implements OnClickListener {
    private static final String TAG = "ConnectionInfoPopup";

    private static final int DESCRIPTION_TEXT_SIZE_SP = 12;
    private final Context mContext;
    private final Dialog mDialog;
    private final LinearLayout mContainer;
    private final WebContents mWebContents;
    private final int mPaddingWide, mPaddingThin;
    private final long mNativeConnectionInfoPopup;
    private TextView mCertificateViewer, mResetCertLink;
    private ViewGroup mCertificateLayout, mDescriptionLayout;

    private ConnectionInfoPopup(Context context, WebContents webContents) {
        mContext = context;
        mWebContents = webContents;

        mContainer = new LinearLayout(mContext);
        mContainer.setOrientation(LinearLayout.VERTICAL);
        mContainer.setBackgroundColor(Color.WHITE);
        mPaddingWide = (int) context.getResources().getDimension(
                R.dimen.connection_info_padding_wide);
        mPaddingThin = (int) context.getResources().getDimension(
                R.dimen.connection_info_padding_thin);
        mContainer.setPadding(mPaddingWide, mPaddingWide, mPaddingWide,
                mPaddingWide - mPaddingThin);

        mDialog = new Dialog(mContext);
        mDialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
        mDialog.setCanceledOnTouchOutside(true);
        // This needs to come after other member initialization.
        mNativeConnectionInfoPopup = nativeInit(this, webContents);
        final WebContentsObserver webContentsObserver =
                new WebContentsObserver(mWebContents) {
            @Override
            public void navigationEntryCommitted() {
                // If a navigation is committed (e.g. from in-page redirect), the data we're
                // showing is stale so dismiss the dialog.
                mDialog.dismiss();
            }

            @Override
            public void destroy() {
                super.destroy();
                mDialog.dismiss();
            }
        };
        mDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                assert mNativeConnectionInfoPopup != 0;
                webContentsObserver.destroy();
                nativeDestroy(mNativeConnectionInfoPopup);
            }
        });
    }

    /**
     * Add a an explanation section with the given icon, headline, and
     * description paragraph. If |isCertificateSection| is set, records that
     * this is the (unique) explanation regarding the site's SSL
     * certificate.
     */
    @CalledByNative
    private View addExplanation(int enumeratedIconId, String headline, String description,
            boolean isCertificateSection) {
        View section = LayoutInflater.from(mContext).inflate(R.layout.connection_info,
                null);
        ImageView i = (ImageView) section.findViewById(R.id.connection_info_icon);
        int drawableId = ResourceId.mapToDrawableId(enumeratedIconId);
        i.setImageResource(drawableId);

        TextView h = (TextView) section.findViewById(R.id.connection_info_headline);
        h.setText(headline);
        if (TextUtils.isEmpty(headline)) h.setVisibility(View.GONE);

        TextView d = (TextView) section.findViewById(R.id.connection_info_description);
        d.setText(description);
        d.setTextSize(DESCRIPTION_TEXT_SIZE_SP);
        if (TextUtils.isEmpty(description)) d.setVisibility(View.GONE);

        if (isCertificateSection) {
            assert mCertificateLayout == null;
            mCertificateLayout = (ViewGroup) section.findViewById(R.id.connection_info_text_layout);
        }

        mContainer.addView(section);
        return section;
    }

    /**
     * Add a certificate viewer link to the given section, with the given
     * label. Must be called after addExplanation() was called with
     * isCertificateSection set to true.
     */
    @CalledByNative
    private void addCertificateViewerLink(String label) {
        assert mCertificateViewer == null;
        assert mCertificateLayout != null;

        mCertificateViewer = new TextView(mContext);
        mCertificateViewer.setText(label);
        mCertificateViewer.setTextColor(ApiCompatibilityUtils.getColor(
                mContext.getResources(), R.color.page_info_popup_text_link));
        mCertificateViewer.setTextSize(DESCRIPTION_TEXT_SIZE_SP);
        mCertificateViewer.setOnClickListener(this);
        mCertificateViewer.setPadding(0, 0, 0, 0);
        mCertificateLayout.addView(mCertificateViewer);
    }

    /**
     * Add a reset cert decisions link to the given section, with the given
     * label. Must be called after addExplanation() was called with
     * isCertificateSection set to true.
     */
    @CalledByNative
    private void addResetCertDecisionsButton(String description, String linkLabel) {
        assert mResetCertLink == null;
        assert mCertificateLayout != null;

        LinearLayout container = new LinearLayout(mContext);
        container.setOrientation(LinearLayout.VERTICAL);
        container.setPadding(0, mPaddingThin, 0, 0);

        TextView resetCertDescription = new TextView(mContext);
        resetCertDescription.setText(description);
        resetCertDescription.setTextColor(ApiCompatibilityUtils.getColor(
                mContext.getResources(), R.color.page_info_popup_text));
        resetCertDescription.setTextSize(DESCRIPTION_TEXT_SIZE_SP);
        resetCertDescription.setPadding(0, 0, 0, 0);
        container.addView(resetCertDescription);

        mResetCertLink = new TextView(mContext);
        mResetCertLink.setText(linkLabel);
        mResetCertLink.setTextColor(ApiCompatibilityUtils.getColor(
                mContext.getResources(), R.color.page_info_popup_text_link));
        mResetCertLink.setTextSize(DESCRIPTION_TEXT_SIZE_SP);
        mResetCertLink.setOnClickListener(this);
        mResetCertLink.setPadding(0, 0, 0, 0);
        container.addView(mResetCertLink);

        mCertificateLayout.addView(container);
    }

    /** Displays the ConnectionInfoPopup. */
    @CalledByNative
    private void showDialog() {
        ScrollView scrollView = new ScrollView(mContext);
        scrollView.addView(mContainer);
        mDialog.addContentView(scrollView,
                new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                        LinearLayout.LayoutParams.MATCH_PARENT));

        mDialog.getWindow().setLayout(
                ViewGroup.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        mDialog.show();
    }

    @Override
    public void onClick(View v) {
        if (mResetCertLink == v) {
            nativeResetCertDecisions(mNativeConnectionInfoPopup, mWebContents);
            mDialog.dismiss();
        } else if (mCertificateViewer == v) {
            byte[][] certChain = CertificateChainHelper.getCertificateChain(mWebContents);
            if (certChain == null) {
                // The WebContents may have been destroyed/invalidated. If so,
                // ignore this request.
                return;
            }
            CertificateViewer.showCertificateChain(mContext, certChain);
        }
    }

    /**
     * Shows a connection info dialog for the provided WebContents.
     *
     * The popup adds itself to the view hierarchy which owns the reference while it's
     * visible.
     *
     * @param context Context which is used for launching a dialog.
     * @param webContents The WebContents for which to show Website information. This
     *         information is retrieved for the visible entry.
     */
    public static void show(Context context, WebContents webContents) {
        new ConnectionInfoPopup(context, webContents);
    }

    private static native long nativeInit(ConnectionInfoPopup popup,
            WebContents webContents);
    private native void nativeDestroy(long nativeConnectionInfoPopupAndroid);
    private native void nativeResetCertDecisions(
            long nativeConnectionInfoPopupAndroid, WebContents webContents);
}
