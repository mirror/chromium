// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.TransitionDrawable;
import android.media.ThumbnailUtils;
import android.os.StrictMode;
import android.os.SystemClock;
import android.support.v4.text.BidiFormatter;
import android.text.format.DateUtils;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.Callback;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.compositor.layouts.ChromeAnimation;
import org.chromium.chrome.browser.download.DownloadUtils;
import org.chromium.chrome.browser.download.ui.DownloadFilter;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.widget.TintedImageView;

import java.util.concurrent.TimeUnit;

/**
 * This class is directly connected to suggestions view holders. It takes over the responsibility
 * of the view holder to update information on the views on the suggestion card.
 */
public class SuggestionsBinder {
    private static final String ARTICLE_AGE_FORMAT_STRING = " - %s";
    private static final int FADE_IN_ANIMATION_TIME_MS = 300;

    private final ImageFetcher mImageFetcher;
    private final SuggestionsUiDelegate mUiDelegate;

    private final View mCardContainerView;
    private final LinearLayout mTextLayout;
    private final TextView mHeadlineTextView;
    private final TextView mPublisherTextView;
    private final TextView mArticleSnippetTextView;
    private final TextView mArticleAgeTextView;
    private final TintedImageView mThumbnailView;
    private final ImageView mOfflineBadge;
    private final View mPublisherBar;

    /** Total horizontal space occupied by the thumbnail, sum of its size and margin. */
    private final int mThumbnailFootprintPx;
    private final int mThumbnailSize;

    private int mPublisherFaviconSizePx;

    private ImageFetcher.DownloadThumbnailRequest mThumbnailRequest;

    private SnippetArticle mSuggestion;

    public SuggestionsBinder(View cardContainerView, SuggestionsUiDelegate uiDelegate) {
        mCardContainerView = cardContainerView;
        mUiDelegate = uiDelegate;
        mImageFetcher = uiDelegate.getImageFetcher();

        mTextLayout = (LinearLayout) mCardContainerView.findViewById(R.id.text_layout);
        mThumbnailView = (TintedImageView) mCardContainerView.findViewById(R.id.article_thumbnail);

        mHeadlineTextView = (TextView) mCardContainerView.findViewById(R.id.article_headline);
        mPublisherTextView = (TextView) mCardContainerView.findViewById(R.id.article_publisher);
        mArticleSnippetTextView = (TextView) mCardContainerView.findViewById(R.id.article_snippet);
        mArticleAgeTextView = (TextView) mCardContainerView.findViewById(R.id.article_age);
        mPublisherBar = mCardContainerView.findViewById(R.id.publisher_bar);
        mOfflineBadge = (ImageView) mCardContainerView.findViewById(R.id.offline_icon);

        boolean useLargeThumbnailLayout =
                ChromeFeatureList.isEnabled(ChromeFeatureList.CONTENT_SUGGESTIONS_LARGE_THUMBNAIL);
        mThumbnailSize = mCardContainerView.getResources().getDimensionPixelSize(
                useLargeThumbnailLayout ? R.dimen.snippets_thumbnail_size_large
                                        : R.dimen.snippets_thumbnail_size);
        mThumbnailFootprintPx = mThumbnailSize
                + mCardContainerView.getResources().getDimensionPixelSize(
                          R.dimen.snippets_thumbnail_margin);
    }

    public void updateViewInformation(SnippetArticle suggestion, int headerMaxLines) {
        mSuggestion = suggestion;

        mHeadlineTextView.setText(suggestion.mTitle);
        mHeadlineTextView.setMaxLines(headerMaxLines);
        mArticleSnippetTextView.setText(suggestion.mPreviewText);
        mPublisherTextView.setText(getPublisherString(suggestion));
        mArticleAgeTextView.setText(getArticleAge(suggestion));

        setFavicon();
        setThumbnail();
    }

    public void updateFieldsVisibility(
            boolean showHeadline, boolean showDescription, boolean showThumbnail) {
        mHeadlineTextView.setVisibility(showHeadline ? View.VISIBLE : View.GONE);
        mArticleSnippetTextView.setVisibility(showDescription ? View.VISIBLE : View.GONE);
        mThumbnailView.setVisibility(showThumbnail ? View.VISIBLE : View.GONE);

        ViewGroup.MarginLayoutParams publisherBarParams =
                (ViewGroup.MarginLayoutParams) mPublisherBar.getLayoutParams();

        if (showDescription) {
            publisherBarParams.topMargin = mPublisherBar.getResources().getDimensionPixelSize(
                    R.dimen.snippets_publisher_margin_top_with_article_snippet);
        } else if (showHeadline) {
            // When we show a headline and not a description, we reduce the top margin of the
            // publisher bar.
            publisherBarParams.topMargin = mPublisherBar.getResources().getDimensionPixelSize(
                    R.dimen.snippets_publisher_margin_top_without_article_snippet);
        } else {
            // When there is no headline and no description, we remove the top margin of the
            // publisher bar.
            publisherBarParams.topMargin = 0;
        }

        if (!ChromeFeatureList.isEnabled(ChromeFeatureList.CONTENT_SUGGESTIONS_LARGE_THUMBNAIL)) {
            mTextLayout.setMinimumHeight(showThumbnail ? mThumbnailSize : 0);

            ApiCompatibilityUtils.setMarginEnd(
                    publisherBarParams, showThumbnail ? mThumbnailFootprintPx : 0);
        }

        mPublisherBar.setLayoutParams(publisherBarParams);
    }

    public void updateOfflineBadgeVisibility(boolean visible) {
        if (visible == (mOfflineBadge.getVisibility() == View.VISIBLE)) return;
        mOfflineBadge.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    private void setFavicon() {
        // The favicon of the publisher should match the TextView height.
        int widthSpec = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED);
        int heightSpec = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED);
        mPublisherTextView.measure(widthSpec, heightSpec);
        mPublisherFaviconSizePx = mPublisherTextView.getMeasuredHeight();

        // Set the favicon of the publisher.
        // We start initialising with the default favicon to reserve the space and prevent the text
        // from moving later.
        setDefaultFaviconOnView();
        Callback<Bitmap> faviconCallback = new Callback<Bitmap>() {
            @Override
            public void onResult(Bitmap bitmap) {
                setFaviconOnView(bitmap);
            }
        };

        mImageFetcher.makeFaviconRequest(mSuggestion, mPublisherFaviconSizePx, faviconCallback);
    }

    private void setThumbnail() {
        // If there's still a pending thumbnail fetch, cancel it.
        cancelThumbnailFetch();

        // mThumbnailView's visibility is modified in updateFieldsVisibility().
        if (mThumbnailView.getVisibility() != View.VISIBLE) return;

        Bitmap thumbnail = mSuggestion.getThumbnailBitmap();
        if (thumbnail != null) {
            setThumbnailFromBitmap(thumbnail);
            return;
        }

        if (mSuggestion.isDownload()) {
            setDownloadThumbnail(mSuggestion);
            return;
        }

        // Temporarily set placeholder and then fetch the thumbnail from a provider.
        mThumbnailView.setBackground(null);
        mThumbnailView.setImageResource(R.drawable.ic_snippet_thumbnail_placeholder);
        mThumbnailView.setTint(null);

        // Fetch thumbnail for the current article.
        mImageFetcher.makeArticleThumbnailRequest(
                mSuggestion, new FetchThumbnailCallback(mSuggestion, mThumbnailSize));
    }

    private void setThumbnailFromBitmap(Bitmap thumbnail) {
        assert thumbnail != null;
        assert !thumbnail.isRecycled();
        assert thumbnail.getWidth() == mThumbnailSize;
        assert thumbnail.getHeight() == mThumbnailSize;

        mThumbnailView.setScaleType(ImageView.ScaleType.CENTER_CROP);
        mThumbnailView.setBackground(null);
        mThumbnailView.setImageBitmap(thumbnail);
        mThumbnailView.setTint(null);
    }

    private void setThumbnailFromFileType(int fileType) {
        int mIconBackgroundColor =
                DownloadUtils.getIconBackgroundColor(mThumbnailView.getContext());
        ColorStateList mIconForegroundColorList =
                DownloadUtils.getIconForegroundColorList(mThumbnailView.getContext());

        mThumbnailView.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
        mThumbnailView.setBackgroundColor(mIconBackgroundColor);
        mThumbnailView.setImageResource(
                DownloadUtils.getIconResId(fileType, DownloadUtils.ICON_SIZE_36_DP));
        mThumbnailView.setTint(mIconForegroundColorList);
    }

    private void setDownloadThumbnail(SnippetArticle article) {
        if (!article.isAssetDownload()) {
            setThumbnailFromFileType(DownloadFilter.FILTER_PAGE);
            return;
        }

        int fileType = DownloadFilter.fromMimeType(article.getAssetDownloadMimeType());
        if (fileType != DownloadFilter.FILTER_IMAGE) {
            setThumbnailFromFileType(fileType);
            return;
        }

        // Temporarily set placeholder and then fetch the thumbnail from a provider.
        mThumbnailView.setBackground(null);
        mThumbnailView.setImageResource(R.drawable.ic_snippet_thumbnail_placeholder);
        mThumbnailView.setTint(null);

        mThumbnailRequest = mImageFetcher.makeDownloadThumbnailRequest(
                article, mThumbnailSize, new FetchThumbnailCallback(article, mThumbnailSize));
    }

    private void setDefaultFaviconOnView() {
        setFaviconOnView(ApiCompatibilityUtils.getDrawable(
                mPublisherTextView.getContext().getResources(), R.drawable.default_favicon));
    }

    private void setFaviconOnView(Bitmap image) {
        setFaviconOnView(new BitmapDrawable(mPublisherTextView.getContext().getResources(), image));
    }

    private void setFaviconOnView(Drawable drawable) {
        drawable.setBounds(0, 0, mPublisherFaviconSizePx, mPublisherFaviconSizePx);
        ApiCompatibilityUtils.setCompoundDrawablesRelative(
                mPublisherTextView, drawable, null, null, null);
        mPublisherTextView.setVisibility(View.VISIBLE);
    }

    private void cancelThumbnailFetch() {
        if (mThumbnailRequest != null) {
            mThumbnailRequest.cancel();
            mThumbnailRequest = null;
        }
    }

    private void fadeThumbnailIn(Bitmap thumbnail) {
        mThumbnailView.setScaleType(ImageView.ScaleType.CENTER_CROP);
        mThumbnailView.setBackground(null);
        mThumbnailView.setTint(null);
        int duration = (int) (FADE_IN_ANIMATION_TIME_MS
                * ChromeAnimation.Animation.getAnimationMultiplier());
        if (duration == 0) {
            mThumbnailView.setImageBitmap(thumbnail);
            return;
        }

        // Cross-fade between the placeholder and the thumbnail. We cross-fade because the incoming
        // image may have transparency and we don't want the previous image showing up behind.
        Drawable[] layers = {mThumbnailView.getDrawable(),
                new BitmapDrawable(mThumbnailView.getResources(), thumbnail)};
        TransitionDrawable transitionDrawable = new TransitionDrawable(layers);
        mThumbnailView.setImageDrawable(transitionDrawable);
        transitionDrawable.setCrossFadeEnabled(true);
        transitionDrawable.startTransition(duration);
    }

    private static String getPublisherString(SnippetArticle article) {
        // We format the publisher here so that having a publisher name in an RTL language
        // doesn't mess up the formatting on an LTR device and vice versa.
        return BidiFormatter.getInstance().unicodeWrap(article.mPublisher);
    }

    private static String getArticleAge(SnippetArticle article) {
        if (article.mPublishTimestampMilliseconds == 0) return "";

        // DateUtils.getRelativeTimeSpanString(...) calls through to TimeZone.getDefault(). If this
        // has never been called before it loads the current time zone from disk. In most likelihood
        // this will have been called previously and the current time zone will have been cached,
        // but in some cases (eg instrumentation tests) it will cause a strict mode violation.
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
        CharSequence relativeTimeSpan;
        try {
            long time = SystemClock.elapsedRealtime();
            relativeTimeSpan =
                    DateUtils.getRelativeTimeSpanString(article.mPublishTimestampMilliseconds,
                            System.currentTimeMillis(), DateUtils.MINUTE_IN_MILLIS);
            RecordHistogram.recordTimesHistogram("Android.StrictMode.SnippetUIBuildTime",
                    SystemClock.elapsedRealtime() - time, TimeUnit.MILLISECONDS);
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }

        // We add a dash before the elapsed time, e.g. " - 14 minutes ago".
        return String.format(ARTICLE_AGE_FORMAT_STRING,
                BidiFormatter.getInstance().unicodeWrap(relativeTimeSpan));
    }

    private class FetchThumbnailCallback extends Callback<Bitmap> {
        private final SnippetArticle mSuggestion;
        private final int mThumbnailSize;

        FetchThumbnailCallback(SnippetArticle suggestion, int size) {
            mSuggestion = suggestion;
            mThumbnailSize = size;
        }

        @Override
        public void onResult(Bitmap thumbnail) {
            if (thumbnail == null) return; // Nothing to do, we keep the placeholder.

            // We need to crop and scale the downloaded bitmap, as the ImageView we set it on won't
            // be able to do so when using a TransitionDrawable (as opposed to the straight bitmap).
            // That's a limitation of TransitionDrawable, which doesn't handle layers of varying
            // sizes.
            if (thumbnail.getHeight() != mThumbnailSize || thumbnail.getWidth() != mThumbnailSize) {
                // Resize the thumbnail. If we fully own the input bitmap (e.g. it isn't cached
                // anywhere else), recycle the input image in the process.
                thumbnail = ThumbnailUtils.extractThumbnail(thumbnail, mThumbnailSize,
                        mThumbnailSize,
                        mSuggestion.isDownload() ? ThumbnailUtils.OPTIONS_RECYCLE_INPUT : 0);
            }

            // Store the bitmap to skip the download task next time we display this snippet.
            mSuggestion.setThumbnailBitmap(mUiDelegate.getReferencePool().put(thumbnail));

            if (mSuggestion.getUrl() != SuggestionsBinder.this.mSuggestion.getUrl()) return;

            fadeThumbnailIn(thumbnail);
        }
    }
}
