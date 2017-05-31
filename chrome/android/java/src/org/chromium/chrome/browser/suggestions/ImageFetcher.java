// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.annotation.SuppressLint;
import android.graphics.Bitmap;
import android.os.SystemClock;
import android.support.annotation.IntDef;

import org.chromium.base.Callback;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.browser.download.ui.ThumbnailProvider;
import org.chromium.chrome.browser.download.ui.ThumbnailProviderImpl;
import org.chromium.chrome.browser.favicon.FaviconHelper;
import org.chromium.chrome.browser.favicon.LargeIconBridge;
import org.chromium.chrome.browser.ntp.cards.CardsVariationParameters;
import org.chromium.chrome.browser.ntp.snippets.FaviconFetchResult;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.ntp.snippets.SnippetsConfig;
import org.chromium.chrome.browser.profiles.Profile;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.concurrent.TimeUnit;

/**
 * Class responsible for fetching images for the views in the NewTabPage and Chrome Home.
 * The images fetched by this class include:
 *   - favicons for content suggestions
 *   - thumbnails for content suggestions
 *   - large icons for URLs (used by the tiles in the NTP/Chrome Home)
 *
 * Fetching an image is available through a {@link ImageFetcherRequest}, where requests are built
 * using the Builder pattern. Each type of image has different compulsory fields that need to be
 * set by the caller. To start building an image fetch request use the {@link #createRequest()}
 * method.
 */
public class ImageFetcher implements DestructionObserver {
    private static final int[] FAVICON_SERVICE_SUPPORTED_SIZES = {16, 24, 32, 48, 64};
    private static final String FAVICON_SERVICE_FORMAT =
            "https://s2.googleusercontent.com/s2/favicons?domain=%s&src=chrome_newtab_mobile&sz=%d&alt=404";
    private static final int PUBLISHER_FAVICON_MINIMUM_SIZE_PX = 16;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({THUMBNAIL, FAVICON, LARGE_ICON})
    private @interface ImageType {}
    private static final int THUMBNAIL = 0;
    private static final int FAVICON = 1;
    private static final int LARGE_ICON = 2;

    private final boolean mUseFaviconService;

    private boolean mIsDestroyed;

    private final SuggestionsUiDelegate mUiDelegate;
    private final Profile mProfile;
    private final ThumbnailProvider mThumbnailProvider;
    private FaviconHelper mFaviconHelper;
    private LargeIconBridge mLargeIconBridge;

    public ImageFetcher(SuggestionsUiDelegate uiDelegate, Profile profile) {
        mUiDelegate = uiDelegate;
        mProfile = profile;
        mThumbnailProvider = new ThumbnailProviderImpl();
        mUseFaviconService = CardsVariationParameters.isFaviconServiceEnabled();
    }

    public ImageFetcherRequest createRequest() {
        return new ImageFetcherRequest();
    }

    /**
     * Gets the large icon (e.g. favicon or touch icon) for a given URL.
     * @param url The URL of the site whose icon is being requested.
     * @param size The desired size of the icon in pixels.
     * @param callback The callback to be notified when the icon is available.
     */
    public void getLargeIconForUrl(
            String url, int size, LargeIconBridge.LargeIconCallback callback) {
        if (mIsDestroyed) return;
        getLargeIconBridge().getLargeIconForUrl(url, size, callback);
    }

    @Override
    public void onDestroy() {
        if (mFaviconHelper != null) {
            mFaviconHelper.destroy();
            mFaviconHelper = null;
        }
        if (mLargeIconBridge != null) {
            mLargeIconBridge.destroy();
            mLargeIconBridge = null;
        }

        mIsDestroyed = true;
    }

    private void fetchFavicon(SnippetArticle article, final int faviconSizePx,
            final Callback<Bitmap> faviconCallback) {
        try {
            long faviconFetchStartTimeMs = SystemClock.elapsedRealtime();
            URI pageUrl = new URI(article.mUrl);
            if (!article.isArticle() || !SnippetsConfig.isFaviconsFromNewServerEnabled()) {
                // The old code path. Remove when the experiment is successful.
                // Currently, we have to use this for non-articles, due to privacy.
                fetchFaviconFromLocalCache(pageUrl, true, faviconFetchStartTimeMs, faviconSizePx,
                        article, faviconCallback);
            } else {
                // The new code path.
                fetchFaviconFromLocalCacheOrGoogleServer(
                        article, faviconFetchStartTimeMs, faviconCallback);
            }
        } catch (URISyntaxException e) {
            // Do nothing, stick to the default favicon.
        }
    }

    private void fetchFaviconFromLocalCache(final URI snippetUri, final boolean fallbackToService,
            final long faviconFetchStartTimeMs, final int faviconSizePx,
            final SnippetArticle article, final Callback<Bitmap> faviconCallback) {
        getLocalFaviconImageForURL(getSnippetDomain(snippetUri), faviconSizePx,
                new FaviconHelper.FaviconImageCallback() {
                    @Override
                    public void onFaviconAvailable(Bitmap image, String iconUrl) {
                        if (image != null) {
                            assert faviconCallback != null;

                            faviconCallback.onResult(image);
                            recordFaviconFetchResult(article,
                                    fallbackToService ? FaviconFetchResult.SUCCESS_CACHED
                                                      : FaviconFetchResult.SUCCESS_FETCHED,
                                    faviconFetchStartTimeMs);
                        } else if (fallbackToService) {
                            if (!fetchFaviconFromService(article, snippetUri,
                                        faviconFetchStartTimeMs, faviconSizePx, faviconCallback)) {
                                recordFaviconFetchResult(article, FaviconFetchResult.FAILURE,
                                        faviconFetchStartTimeMs);
                            }
                        }
                        // Else do nothing, we already have the placeholder set.
                    }
                });
    }

    private void fetchFaviconFromLocalCacheOrGoogleServer(SnippetArticle article,
            final long faviconFetchStartTimeMs, final Callback<Bitmap> faviconCallback) {
        // Set the desired size to 0 to specify we do not want to resize in c++, we'll resize here.
        mUiDelegate.getSuggestionsSource().fetchSuggestionFavicon(article,
                PUBLISHER_FAVICON_MINIMUM_SIZE_PX, /* desiredSizePx */ 0, new Callback<Bitmap>() {
                    @Override
                    public void onResult(Bitmap image) {
                        recordFaviconFetchTime(faviconFetchStartTimeMs);
                        if (image == null) return;
                        faviconCallback.onResult(image);
                    }
                });
    }

    // TODO(crbug.com/635567): Fix this properly.
    @SuppressLint("DefaultLocale")
    private boolean fetchFaviconFromService(final SnippetArticle article, final URI snippetUri,
            final long faviconFetchStartTimeMs, final int faviconSizePx,
            final Callback<Bitmap> faviconCallback) {
        if (!mUseFaviconService) return false;
        int sizePx = getFaviconServiceSupportedSize(faviconSizePx);
        if (sizePx == 0) return false;

        // Replace the default icon by another one from the service when it is fetched.
        ensureIconIsAvailable(
                getSnippetDomain(snippetUri), // Store to the cache for the whole domain.
                String.format(FAVICON_SERVICE_FORMAT, snippetUri.getHost(), sizePx),
                /*useLargeIcon=*/false, /*isTemporary=*/true,
                new FaviconHelper.IconAvailabilityCallback() {
                    @Override
                    public void onIconAvailabilityChecked(boolean newlyAvailable) {
                        if (!newlyAvailable) {
                            recordFaviconFetchResult(
                                    article, FaviconFetchResult.FAILURE, faviconFetchStartTimeMs);
                            return;
                        }
                        // The download succeeded, the favicon is in the cache; fetch it.
                        fetchFaviconFromLocalCache(snippetUri, /* fallbackToService =*/false,
                                faviconFetchStartTimeMs, faviconSizePx, article, faviconCallback);
                    }
                });
        return true;
    }

    private void recordFaviconFetchTime(long faviconFetchStartTimeMs) {
        RecordHistogram.recordMediumTimesHistogram(
                "NewTabPage.ContentSuggestions.ArticleFaviconFetchTime",
                SystemClock.elapsedRealtime() - faviconFetchStartTimeMs, TimeUnit.MILLISECONDS);
    }

    private void recordFaviconFetchResult(
            SnippetArticle article, @FaviconFetchResult int result, long faviconFetchStartTimeMs) {
        // Record the histogram for articles only to have a fair comparision.
        if (!article.isArticle()) return;
        RecordHistogram.recordEnumeratedHistogram(
                "NewTabPage.ContentSuggestions.ArticleFaviconFetchResult", result,
                FaviconFetchResult.COUNT);
        recordFaviconFetchTime(faviconFetchStartTimeMs);
    }

    /**
     * Gets the favicon image for a given URL.
     * @param url The URL of the site whose favicon is being requested.
     * @param size The desired size of the favicon in pixels.
     * @param faviconCallback The callback to be notified when the favicon is available.
     */
    private void getLocalFaviconImageForURL(
            String url, int size, FaviconHelper.FaviconImageCallback faviconCallback) {
        if (mIsDestroyed) return;
        getFaviconHelper().getLocalFaviconImageForURL(mProfile, url, size, faviconCallback);
    }

    /**
     * Checks if an icon with the given URL is available. If not,
     * downloads it and stores it as a favicon/large icon for the given {@code pageUrl}.
     * @param pageUrl The URL of the site whose icon is being requested.
     * @param iconUrl The URL of the favicon/large icon.
     * @param isLargeIcon Whether the {@code iconUrl} represents a large icon or favicon.
     * @param callback The callback to be notified when the favicon has been checked.
     */
    private void ensureIconIsAvailable(String pageUrl, String iconUrl, boolean isLargeIcon,
            boolean isTemporary, FaviconHelper.IconAvailabilityCallback callback) {
        if (mIsDestroyed) return;
        if (mUiDelegate.getHost().getActiveTab() != null) {
            getFaviconHelper().ensureIconIsAvailable(mProfile,
                    mUiDelegate.getHost().getActiveTab().getWebContents(), pageUrl, iconUrl,
                    isLargeIcon, isTemporary, callback);
        }
    }

    private int getFaviconServiceSupportedSize(int faviconSizePX) {
        // Take the smallest size larger than mFaviconSizePx.
        for (int size : FAVICON_SERVICE_SUPPORTED_SIZES) {
            if (size > faviconSizePX) return size;
        }
        // Or at least the largest available size (unless too small).
        int largestSize =
                FAVICON_SERVICE_SUPPORTED_SIZES[FAVICON_SERVICE_SUPPORTED_SIZES.length - 1];
        if (faviconSizePX <= largestSize * 1.5) return largestSize;
        return 0;
    }

    private String getSnippetDomain(URI snippetUri) {
        return String.format("%s://%s", snippetUri.getScheme(), snippetUri.getHost());
    }

    /**
     * Utility method to lazily create the {@link FaviconHelper}, and avoid unnecessary native
     * calls in tests.
     */
    private FaviconHelper getFaviconHelper() {
        assert !mIsDestroyed;
        if (mFaviconHelper == null) mFaviconHelper = new FaviconHelper();
        return mFaviconHelper;
    }

    /**
     * Utility method to lazily create the {@link LargeIconBridge}, and avoid unnecessary native
     * calls in tests.
     */
    private LargeIconBridge getLargeIconBridge() {
        assert !mIsDestroyed;
        if (mLargeIconBridge == null) mLargeIconBridge = new LargeIconBridge(mProfile);
        return mLargeIconBridge;
    }

    private void fetchDownloadThumbnail(
            SnippetArticle article, int thumbnailSizePx, Callback<Bitmap> imageCallback) {
        ThumbnailCallback downloadThumbnailCallback =
                new ThumbnailCallback(imageCallback, article, thumbnailSizePx);

        mThumbnailProvider.getThumbnail(downloadThumbnailCallback);
    }

    private void fetchArticleThumbnail(SnippetArticle article, Callback<Bitmap> callback) {
        mUiDelegate.getSuggestionsSource().fetchSuggestionImage(article, callback);
    }

    private static class ThumbnailCallback implements ThumbnailProvider.ThumbnailRequest {
        private final SnippetArticle mSnippet;
        private final Callback<Bitmap> mCallback;
        private int mIconSizePx;

        private ThumbnailCallback(
                Callback<Bitmap> callback, SnippetArticle snippet, int iconSizePx) {
            assert snippet != null;
            assert callback != null;

            mSnippet = snippet;
            mCallback = callback;
            mIconSizePx = iconSizePx;
        }

        @Override
        public String getFilePath() {
            return mSnippet.getAssetDownloadFile().getAbsolutePath();
        }

        @Override
        public void onThumbnailRetrieved(String filePath, Bitmap thumbnail) {
            assert !thumbnail.isRecycled();

            if (!getFilePath().equals(filePath)) return;
            if (thumbnail.getWidth() <= 0 || thumbnail.getHeight() <= 0) return;

            mCallback.onResult(thumbnail);
        }

        @Override
        public int getIconSize() {
            return mIconSizePx;
        }
    }

    /**
     * A holder for all information which is needed to fetch an image. According to the type of the
     * image, different fields need to be set. The class is following the builder pattern, but
     * an instance of it can be acquired via a call to {@link ImageFetcher#createRequest() }.
     * The three type of images which can be requested are:
     *   - favicons for articles
     *   - thumbnails for articles
     *   - large icons for urls
     * Check these methods for more information on what information is required to fetch an image
     * for each of these image types.
     */
    public class ImageFetcherRequest {
        private @ImageType Integer mImageType;
        private Integer mSizePx;
        private SnippetArticle mArticle;
        private Callback<Bitmap> mCallback;
        private boolean mFetched;

        @VisibleForTesting
        ImageFetcherRequest() {}

        /**
         * Starting point to get a thumbnail image for an article. Other compulsory methods for this
         * type of image are {@link #forArticle(SnippetArticle)}, {@link #ofSize(int)},
         * {@link #intoCallback(Callback)}. These methods must be called with relevant information
         * about the desired thumbnail in order to get a thumbnail from the relevant source
         * properly. Once all these methods are called, a call to {@link #fetch()} will initiate
         * the search and setting of the thumbnail.
         *
         * Otherwise a null will be set as an image in the provided ImageView.
         * @return Reference to the current {@link ImageFetcherRequest}.
         */
        public ImageFetcherRequest thumbnail() {
            assert mImageType == null;
            assert !mFetched;

            mImageType = THUMBNAIL;
            return this;
        }

        /**
         * Starting point to get a favicon image for an article. Other compulsory methods for this
         * type of image are {@link #forArticle(SnippetArticle)}, {@link #ofSize(int)},
         * {@link #intoCallback(Callback)}. These methods must be called with relevant information
         * for the favicon <u>before</u> the {@link #fetch()} method is called.
         * Calling {@link #fetch()} will initiate the search for the icon.
         *
         * @return Reference to the current {@link ImageFetcherRequest}.
         */
        public ImageFetcherRequest favicon() {
            assert mImageType == null;
            assert !mFetched;

            mImageType = FAVICON;
            return this;
        }

        /**
         * Sets the suggestion for which an image will be fetched. This method must be called for
         * thumbnails and favicons.
         *
         * @param suggestion The suggestion for which an image is needed.
         * @return Reference to the current {@link ImageFetcherRequest}.
         */
        public ImageFetcherRequest forArticle(SnippetArticle suggestion) {
            assert mImageType != null;
            assert !mFetched;

            mArticle = suggestion;
            return this;
        }

        /**
         * Sets the needed size of the image which will be fetched. This method must be called for
         * all imageTypes. Default value is 0.
         *
         * @param sizePx Desired size of the image.
         * @return Reference to the current {@link ImageFetcherRequest}.
         */
        public ImageFetcherRequest ofSize(int sizePx) {
            assert mImageType != null;
            assert !mFetched;

            mSizePx = sizePx;
            return this;
        }

        /**
         * Sets the Callback which will be called after the image is fetched. This method must be
         * called for favicons.
         *
         * @param callback Callback which will be called after the image is fetched.
         * @return Reference to the {@link ImageFetcherRequest}.
         */
        public ImageFetcherRequest intoCallback(Callback<Bitmap> callback) {
            assert mImageType != null;
            assert !mFetched;

            mCallback = callback;
            return this;
        }

        /**
         * Initiate the image fetch.
         * After calling this method, any other method will not do anything.
         *
         * @return Reference to the {@link ImageFetcherRequest}.
         */
        public ImageFetcherRequest fetch() {
            assert mImageType != null;
            assert mCallback != null;
            assert mArticle != null;

            switch (mImageType) {
                case THUMBNAIL:
                    if (mArticle.isDownload()) {
                        assert mSizePx != null;
                        fetchDownloadThumbnail(mArticle, mSizePx, mCallback);
                    } else {
                        fetchArticleThumbnail(mArticle, mCallback);
                    }
                    break;
                case FAVICON:
                    assert mSizePx != null;
                    fetchFavicon(mArticle, mSizePx, mCallback);
                    break;
                case LARGE_ICON:
                    assert mSizePx != null;
                    getLargeIconForUrl(mArticle.getUrl(), mSizePx,
                            (LargeIconBridge.LargeIconCallback) mCallback);
                    break;
                default:
                    assert false;
            }

            mFetched = true;
            return this;
        }
    }
}
