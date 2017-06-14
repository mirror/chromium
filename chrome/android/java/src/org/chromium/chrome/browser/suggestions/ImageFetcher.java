// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.annotation.SuppressLint;
import android.graphics.Bitmap;
import android.os.SystemClock;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.download.ui.ThumbnailProvider;
import org.chromium.chrome.browser.download.ui.ThumbnailProviderImpl;
import org.chromium.chrome.browser.favicon.FaviconHelper;
import org.chromium.chrome.browser.favicon.LargeIconBridge;
import org.chromium.chrome.browser.ntp.cards.CardsVariationParameters;
import org.chromium.chrome.browser.ntp.snippets.FaviconFetchResult;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.ntp.snippets.SnippetsConfig;
import org.chromium.chrome.browser.profiles.Profile;

import java.net.URI;
import java.net.URISyntaxException;

/**
 * Class responsible for fetching images for the views in the NewTabPage and Chrome Home.
 * The images fetched by this class include:
 *   - favicons for content suggestions
 *   - thumbnails for content suggestions
 *   - large icons for URLs (used by the tiles in the NTP/Chrome Home)
 *
 * To fetch an image, the caller should create a request which is done in the following way:
 *   - for favicons: {@link #makeFaviconRequest(SnippetArticle, int, Callback)}
 *   - for article thumbnails: {@link #makeArticleThumbnailRequest(SnippetArticle, Callback)}
 *   - for article downloads: {@link #makeDownloadThumbnailRequest(SnippetArticle, int, Callback)}
 *   - for large icons: {@link #makeLargeIconRequest(String, int,
 * LargeIconBridge.LargeIconCallback)}
 *
 * If there are no errors is the image fetching process, the corresponding bitmap will be returned
 * in the callback. Otherwise, the callback will not be called.
 */
public class ImageFetcher {
    /** Supported favicon sizes in pixels. */
    private static final int[] FAVICON_SERVICE_SUPPORTED_SIZES = {16, 24, 32, 48, 64};
    private static final String FAVICON_SERVICE_FORMAT =
            "https://s2.googleusercontent.com/s2/favicons?domain=%s&src=chrome_newtab_mobile&sz=%d&alt=404";
    private static final int PUBLISHER_FAVICON_MINIMUM_SIZE_PX = 16;

    private final boolean mUseFaviconService;

    private boolean mIsDestroyed;

    private final SuggestionsUiDelegate mUiDelegate;
    private final Profile mProfile;
    private ThumbnailProvider mThumbnailProvider;
    private FaviconHelper mFaviconHelper;
    private LargeIconBridge mLargeIconBridge;

    public ImageFetcher(SuggestionsUiDelegate uiDelegate, Profile profile) {
        mUiDelegate = uiDelegate;
        mProfile = profile;
        mUseFaviconService = CardsVariationParameters.isFaviconServiceEnabled();
    }

    /**
     * Creates a request for a thumbnail from a downloaded image.
     *
     * If there is an error while fetching the thumbnail, the callback will not be called.
     *
     * @param suggestion The suggestion for which a thumbnail is needed.
     * @param thumbnailSizePx The required size for the thumbnail.
     * @param imageCallback The callback where the bitmap will be returned when fetched.
     * @return The request which will be used to fetch the image.
     */
    public DownloadThumbnailRequest makeDownloadThumbnailRequest(
            SnippetArticle suggestion, int thumbnailSizePx, Callback<Bitmap> imageCallback) {
        if (mIsDestroyed) return null;

        return new DownloadThumbnailRequest(suggestion, imageCallback, thumbnailSizePx);
    }

    /**
     * Creates a request for an article thumbnail.
     *
     * If there is an error while fetching the thumbnail, the callback will not be called.
     *
     * @param suggestion The article for which a thumbnail is needed.
     * @param callback The callback where the bitmap will be returned when fetched.
     * @return  The request which will be used to fetch the image.
     */
    public ArticleThumbnailRequest makeArticleThumbnailRequest(
            SnippetArticle suggestion, Callback<Bitmap> callback) {
        if (mIsDestroyed) return null;

        return new ArticleThumbnailRequest(suggestion, callback);
    }

    /**
     * Creates a request for favicon for the URL of a suggestion.
     *
     * If there is an error while fetching the favicon, the callback will not be called.
     *
     * @param suggestion The suggestion whose URL needs a favicon.
     * @param faviconSizePx The required size for the favicon.
     * @param faviconCallback The callback where the bitmap will be returned when fetched.
     * @return The request which will be used to fetch the image.
     */
    public FaviconRequest makeFaviconRequest(SnippetArticle suggestion, final int faviconSizePx,
            final Callback<Bitmap> faviconCallback) {
        if (mIsDestroyed) return null;

        return new FaviconRequest(suggestion, faviconCallback, faviconSizePx);
    }

    /**
     * Gets the large icon (e.g. favicon or touch icon) for a given URL.
     *
     * If there is an error while fetching the icon, the callback will not be called.
     *
     * @param url The URL of the site whose icon is being requested.
     * @param size The desired size of the icon in pixels.
     * @param callback The callback to be notified when the icon is available.
     * @return The request which will be used to fetch the image.
     */
    public LargeIconRequest makeLargeIconRequest(
            String url, int size, LargeIconBridge.LargeIconCallback callback) {
        if (mIsDestroyed) return null;

        return new LargeIconRequest(url, size, callback);
    }

    public void onDestroy() {
        if (mFaviconHelper != null) {
            mFaviconHelper.destroy();
            mFaviconHelper = null;
        }

        if (mLargeIconBridge != null) {
            mLargeIconBridge.destroy();
            mLargeIconBridge = null;
        }

        if (mThumbnailProvider != null) {
            mThumbnailProvider.destroy();
            mThumbnailProvider = null;
        }

        mIsDestroyed = true;
    }

    private void fetchFaviconFromLocalCache(final URI snippetUri, final boolean fallbackToService,
            final long faviconFetchStartTimeMs, final int faviconSizePx,
            final SnippetArticle suggestion, final Callback<Bitmap> faviconCallback) {
        if (mIsDestroyed) return;

        getFaviconHelper().getLocalFaviconImageForURL(mProfile, getSnippetDomain(snippetUri),
                faviconSizePx, new FaviconHelper.FaviconImageCallback() {
                    @Override
                    public void onFaviconAvailable(Bitmap image, String iconUrl) {
                        if (image != null) {
                            assert faviconCallback != null;

                            faviconCallback.onResult(image);
                            SuggestionsMetrics.recordFaviconFetchResult(suggestion,
                                    fallbackToService ? FaviconFetchResult.SUCCESS_CACHED
                                                      : FaviconFetchResult.SUCCESS_FETCHED,
                                    faviconFetchStartTimeMs);
                        } else if (fallbackToService) {
                            if (!fetchFaviconFromService(suggestion, snippetUri,
                                        faviconFetchStartTimeMs, faviconSizePx, faviconCallback)) {
                                SuggestionsMetrics.recordFaviconFetchResult(suggestion,
                                        FaviconFetchResult.FAILURE, faviconFetchStartTimeMs);
                            }
                        }
                    }
                });
    }

    private void fetchFaviconFromLocalCacheOrGoogleServer(SnippetArticle suggestion,
            final long faviconFetchStartTimeMs, final Callback<Bitmap> faviconCallback) {
        // Set the desired size to 0 to specify we do not want to resize in c++, we'll resize here.
        mUiDelegate.getSuggestionsSource().fetchSuggestionFavicon(suggestion,
                PUBLISHER_FAVICON_MINIMUM_SIZE_PX, /* desiredSizePx */ 0, new Callback<Bitmap>() {
                    @Override
                    public void onResult(Bitmap image) {
                        SuggestionsMetrics.recordFaviconFetchTime(faviconFetchStartTimeMs);
                        if (image == null) return;
                        faviconCallback.onResult(image);
                    }
                });
    }

    // TODO(crbug.com/635567): Fix this properly.
    @SuppressLint("DefaultLocale")
    private boolean fetchFaviconFromService(final SnippetArticle suggestion, final URI snippetUri,
            final long faviconFetchStartTimeMs, final int faviconSizePx,
            final Callback<Bitmap> faviconCallback) {
        if (!mUseFaviconService) return false;
        int sizePx = getFaviconServiceSupportedSize(faviconSizePx);
        if (sizePx == 0) return false;

        // Replace the default icon by another one from the service when it is fetched.
        ensureIconIsAvailable(
                getSnippetDomain(snippetUri), // Store to the cache for the whole domain.
                String.format(FAVICON_SERVICE_FORMAT, snippetUri.getHost(), sizePx),
                /* useLargeIcon = */ false, /* isTemporary = */ true,
                new FaviconHelper.IconAvailabilityCallback() {
                    @Override
                    public void onIconAvailabilityChecked(boolean newlyAvailable) {
                        if (!newlyAvailable) {
                            SuggestionsMetrics.recordFaviconFetchResult(suggestion,
                                    FaviconFetchResult.FAILURE, faviconFetchStartTimeMs);
                            return;
                        }
                        // The download succeeded, the favicon is in the cache; fetch it.
                        fetchFaviconFromLocalCache(snippetUri, /* fallbackToService = */ false,
                                faviconFetchStartTimeMs, faviconSizePx, suggestion,
                                faviconCallback);
                    }
                });
        return true;
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
        if (mUiDelegate.getWebContents() != null) {
            getFaviconHelper().ensureIconIsAvailable(mProfile, mUiDelegate.getWebContents(),
                    pageUrl, iconUrl, isLargeIcon, isTemporary, callback);
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
     * Utility method to lazily create the {@link ThumbnailProvider}, and avoid unnecessary native
     * calls in tests.
     */
    private ThumbnailProvider getThumbnailProvider() {
        assert !mIsDestroyed;
        if (mThumbnailProvider == null) mThumbnailProvider = new ThumbnailProviderImpl();
        return mThumbnailProvider;
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

    private interface ImageRequest { void cancel(); }

    /**
     * Request for a download thumbnail.
     */
    public class DownloadThumbnailRequest
            implements ImageRequest, ThumbnailProvider.ThumbnailRequest {
        private final SnippetArticle mSuggestion;
        private final Callback<Bitmap> mCallback;
        private final int mSize;

        DownloadThumbnailRequest(SnippetArticle suggestion, Callback<Bitmap> callback, int size) {
            mSuggestion = suggestion;
            mCallback = callback;
            mSize = size;

            // Fetch the download thumbnail.
            getThumbnailProvider().getThumbnail(this);
        }

        @Override
        public String getFilePath() {
            return mSuggestion.getAssetDownloadFile().getAbsolutePath();
        }

        @Override
        public void onThumbnailRetrieved(String filePath, Bitmap thumbnail) {
            mCallback.onResult(thumbnail);
        }

        @Override
        public int getIconSize() {
            return mSize;
        }

        @Override
        public void cancel() {
            if (mIsDestroyed) return;
            getThumbnailProvider().cancelRetrieval(this);
        }
    }

    /**
     * Request for an article thumbnail.
     */
    public class ArticleThumbnailRequest implements ImageRequest {
        private final Callback<Bitmap> mCallback;
        private final SnippetArticle mSuggestion;

        ArticleThumbnailRequest(SnippetArticle suggestion, Callback<Bitmap> callback) {
            mSuggestion = suggestion;
            mCallback = callback;

            // Fetch the article thumbnail.
            mUiDelegate.getSuggestionsSource().fetchSuggestionImage(mSuggestion, mCallback);
        }

        @Override
        public void cancel() {
            if (mIsDestroyed) return;
            // TODO(galinap): Propagate cancellation to image source.
        }
    }

    /**
     * Request for a favicon for the URL of a suggestion.
     */
    public class FaviconRequest implements ImageRequest {
        private final SnippetArticle mSuggestion;
        private final Callback<Bitmap> mCallback;
        private final int mSize;

        FaviconRequest(SnippetArticle suggestion, Callback<Bitmap> callback, int size) {
            mSuggestion = suggestion;
            mCallback = callback;
            mSize = size;

            // Fetch the favicon.
            long faviconFetchStartTimeMs = SystemClock.elapsedRealtime();
            URI pageUrl;

            try {
                pageUrl = new URI(mSuggestion.mUrl);
            } catch (URISyntaxException e) {
                e.printStackTrace();
                return;
            }

            if (!mSuggestion.isArticle() || !SnippetsConfig.isFaviconsFromNewServerEnabled()) {
                // The old code path. Remove when the experiment is successful.
                // Currently, we have to use this for non-articles, due to privacy.
                fetchFaviconFromLocalCache(
                        pageUrl, true, faviconFetchStartTimeMs, mSize, mSuggestion, mCallback);
            } else {
                // The new code path.
                fetchFaviconFromLocalCacheOrGoogleServer(
                        mSuggestion, faviconFetchStartTimeMs, mCallback);
            }
        }

        @Override
        public void cancel() {
            if (mIsDestroyed) return;
            // TODO(galinap): Propagate cancellation to image source.
        }
    }

    /**
     * Request for a large icon for a URL.
     */
    public class LargeIconRequest implements ImageRequest {
        private final LargeIconBridge.LargeIconCallback mCallback;
        private final String mUrl;
        private final int mSize;

        LargeIconRequest(String url, int size, LargeIconBridge.LargeIconCallback callback) {
            mCallback = callback;
            mUrl = url;
            mSize = size;

            // Fetch the large icon.
            getLargeIconBridge().getLargeIconForUrl(mUrl, mSize, mCallback);
        }

        @Override
        public void cancel() {
            if (mIsDestroyed) return;
            // TODO(galinap): Propagate cancellation to image source.
        }
    }
}
