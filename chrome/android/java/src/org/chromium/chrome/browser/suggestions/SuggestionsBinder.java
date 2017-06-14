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
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.Callback;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.R;
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
    private SuggestionsUiDelegate mUiDelegate;

    private final View mCardContainerView;
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

    private ImageFetcher.DownloadThumbnailRequest mThumbnailCallback;

    private SnippetArticle mArticle;

    public SuggestionsBinder(View cardContainerView, SuggestionsUiDelegate uiDelegate) {
        mCardContainerView = cardContainerView;
        mUiDelegate = uiDelegate;
        mImageFetcher = uiDelegate.getImageFetcher();

        mThumbnailView = (TintedImageView) mCardContainerView.findViewById(R.id.article_thumbnail);
        mThumbnailSize = mCardContainerView.getResources().getDimensionPixelSize(
                R.dimen.snippets_thumbnail_size);

        mHeadlineTextView = (TextView) mCardContainerView.findViewById(R.id.article_headline);
        mPublisherTextView = (TextView) mCardContainerView.findViewById(R.id.article_publisher);
        mArticleSnippetTextView = (TextView) mCardContainerView.findViewById(R.id.article_snippet);
        mArticleAgeTextView = (TextView) mCardContainerView.findViewById(R.id.article_age);
        mPublisherBar = mCardContainerView.findViewById(R.id.publisher_bar);
        mOfflineBadge = (ImageView) mCardContainerView.findViewById(R.id.offline_icon);

        mThumbnailFootprintPx = mThumbnailSize
                + mCardContainerView.getResources().getDimensionPixelSize(
                          R.dimen.snippets_thumbnail_margin);
    }

    public void updateCardInformation(SnippetArticle article) {
        mArticle = article;

        mHeadlineTextView.setText(article.mTitle);
        mArticleSnippetTextView.setText(article.mPreviewText);
        mPublisherTextView.setText(getPublisherString(article));
        mArticleAgeTextView.setText(getArticleAge(article));

        setFavicon(article);
        setThumbnail(article);
    }

    public void setFavicon(SnippetArticle suggestion) {
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

        mImageFetcher.makeFaviconRequest(suggestion, mPublisherFaviconSizePx, faviconCallback);
    }

    public void setThumbnail(SnippetArticle suggestion) {
        // If there's still a pending thumbnail fetch, cancel it.
        cancelThumbnailFetch();

        // mThumbnailView's visibility is modified in updateCardVisibility().
        if (mThumbnailView.getVisibility() != View.VISIBLE) return;

        Bitmap thumbnail = suggestion.getThumbnailBitmap();
        if (thumbnail != null) {
            setThumbnailFromBitmap(thumbnail);
            return;
        }

        if (suggestion.isDownload()) {
            setDownloadThumbnail(suggestion);
            return;
        }

        // Temporarily set placeholder and then fetch the thumbnail from a provider.
        mThumbnailView.setBackground(null);
        mThumbnailView.setImageResource(R.drawable.ic_snippet_thumbnail_placeholder);
        mThumbnailView.setTint(null);

        // Fetch thumbnail for the current article.
        mImageFetcher.makeArticleThumbnailRequest(
                suggestion, new FetchThumbnailCallback(suggestion, mThumbnailSize));
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
        assert article.isDownload();
        if (!article.isAssetDownload()) {
            setThumbnailFromFileType(DownloadFilter.FILTER_PAGE);
            return;
        }

        int fileType = DownloadFilter.fromMimeType(article.getAssetDownloadMimeType());
        if (fileType != DownloadFilter.FILTER_IMAGE) {
            setThumbnailFromFileType(fileType);
            return;
        }

        mImageFetcher.makeDownloadThumbnailRequest(
                article, mThumbnailSize, new FetchThumbnailCallback(article, mThumbnailSize));
    }

    public void updateCardVisibility(
            boolean showHeadline, boolean showDescription, boolean showThumbnail) {
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

        ApiCompatibilityUtils.setMarginEnd(
                publisherBarParams, showThumbnail ? mThumbnailFootprintPx : 0);
        mPublisherBar.setLayoutParams(publisherBarParams);
    }

    public void updateOfflineBadgeVisibility(boolean visible) {
        if (visible == (mOfflineBadge.getVisibility() == View.VISIBLE)) return;
        mOfflineBadge.setVisibility(visible ? View.VISIBLE : View.GONE);
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
        if (mThumbnailCallback != null) {
            mThumbnailCallback.cancel();
            mThumbnailCallback = null;
        }
    }

    private void fadeThumbnailIn(Bitmap thumbnail) {
        // Cross-fade between the placeholder and the thumbnail. We cross-fade because the incoming
        // image may have transparency and we don't want the previous image showing up behind.
        Drawable[] layers = {mThumbnailView.getDrawable(),
                new BitmapDrawable(mThumbnailView.getResources(), thumbnail)};
        TransitionDrawable transitionDrawable = new TransitionDrawable(layers);
        mThumbnailView.setScaleType(ImageView.ScaleType.CENTER_CROP);
        mThumbnailView.setBackground(null);
        mThumbnailView.setImageDrawable(transitionDrawable);
        mThumbnailView.setTint(null);
        transitionDrawable.setCrossFadeEnabled(true);
        transitionDrawable.startTransition(FADE_IN_ANIMATION_TIME_MS);
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

            if (mSuggestion.getUrl() != mArticle.getUrl()) return;

            fadeThumbnailIn(thumbnail);
        }
    }
}
