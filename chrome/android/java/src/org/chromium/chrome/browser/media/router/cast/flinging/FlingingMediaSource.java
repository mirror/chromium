// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.router.cast.flinging;

import android.support.v7.media.MediaRouteSelector;
import android.util.Base64;

import com.google.android.gms.cast.CastMediaControlIntent;

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;

import java.io.UnsupportedEncodingException;

import javax.annotation.Nullable;

/**
 * Abstracts parsing the Cast application id and other parameters from the source URN.
 */
public class FlingingMediaSource {
    private static final String TAG = "MediaFlinging";

    // Need to be in sync with third_party/WebKit/Source/modules/remoteplayback/RemotePlayback.cpp.
    // TODO(avayvod): Find a way to share the constants somehow.
    private static final String SOURCE_PREFIX = "chrome-media-source://";
    private static final String SOURCE_URL_KEY = "sourceUrl";

    /**
     * The original source URL that the {@link MediaSource} object was created from.
     */
    private final String mSourceId;

    /**
     * The Cast application id.
     */
    private final String mApplicationId;

    /**
     * The URI to fling to the Cast device.
     */
    private final String mContentURI;

    /**
     * Initializes the media source from the source id.
     * @param sourceId a URL containing encoded info about the media element's source.
     * @return an initialized media source if the id is valid, null otherwise.
     */
    @Nullable
    public static FlingingMediaSource from(String sourceId) {
        assert sourceId != null;

        if (!sourceId.startsWith(SOURCE_PREFIX)) return null;

        String encodedSourceInfo = sourceId.substring(SOURCE_PREFIX.length());
        String decodedSourceInfo;
        try {
            decodedSourceInfo =
                    new String(Base64.decode(encodedSourceInfo, Base64.URL_SAFE), "UTF-8");
        } catch (IllegalArgumentException | UnsupportedEncodingException e) {
            Log.e(TAG, "Couldn't parse the source id.", e);
            return null;
        }

        JSONObject jsonInfo;
        String contentURI;
        try {
            jsonInfo = new JSONObject(decodedSourceInfo);
            contentURI = jsonInfo.getString(SOURCE_URL_KEY);
            if (contentURI == null) return null;
        } catch (JSONException e) {
            Log.e(TAG, "Couldn't parse the decoded JSON data", e);
            return null;
        }

        // Only support HTTP(s) media resources.
        if (!contentURI.startsWith("http://") && !contentURI.startsWith("https://")) return null;

        // TODO(avayvod): check the content URI and override the app id if needed.
        String applicationId = CastMediaControlIntent.DEFAULT_MEDIA_RECEIVER_APPLICATION_ID;

        return new FlingingMediaSource(sourceId, applicationId, contentURI);
    }

    /**
     * Returns a new {@link MediaRouteSelector} to use for Cast device filtering for this
     * particular media source or null if the application id is invalid.
     *
     * @return an initialized route selector or null.
     */
    public MediaRouteSelector buildRouteSelector() {
        try {
            return new MediaRouteSelector.Builder()
                    .addControlCategory(CastMediaControlIntent.categoryForCast(mApplicationId))
                    .build();
        } catch (IllegalArgumentException e) {
            return null;
        }
    }

    /**
     * @return the Cast application id corresponding to the source. Can be overridden downstream.
     */
    public String getApplicationId() {
        return mApplicationId;
    }

    /**
     * @return the id identifying the media source
     */
    public String getUrn() {
        return mSourceId;
    }

    /**
     * @return the URI to fling to the Cast device.
     */
    public String getContentURI() {
        return mContentURI;
    }

    /**
     * @return if the URL needs to be resolved and checked for remote playback compatibility.
     */
    public boolean needsToBeResolved() {
        // TODO(avayvod): add hooks to override this.
        return true;
    }

    private FlingingMediaSource(String sourceId, String applicationId, String contentURI) {
        mSourceId = sourceId;
        mApplicationId = applicationId;
        mContentURI = contentURI;
    }
}
