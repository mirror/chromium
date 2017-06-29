package com.android.support_library_prototype;

import android.net.Uri;

import java.util.Map;

public interface WebResourceRequest {
    /**
     * Gets the URL for which the resource request was made.
     *
     * @return the URL for which the resource request was made.
     */
    Uri getUrl();

    /**
     * Gets whether the request was made for the main frame.
     *
     * @return whether the request was made for the main frame. Will be false for iframes,
     *         for example.
     */
    boolean isForMainFrame();

    /**
     * Gets whether the request was a result of a server-side redirect.
     *
     * @return whether the request was a result of a server-side redirect.
     */
    boolean isRedirect();

    /**
     * Gets whether a gesture (such as a click) was associated with the request.
     * For security reasons in certain situations this method may return false even though the
     * sequence of events which caused the request to be created was initiated by a user gesture.
     *
     * @return whether a gesture was associated with the request.
     */
    boolean hasGesture();

    /**
     * Gets the method associated with the request, for example "GET".
     *
     * @return the method associated with the request.
     */
    String getMethod();

    /**
     * Gets the headers associated with the request. These are represented as a mapping of header
     * name to header value.
     *
     * @return the headers associated with the request.
     */
    Map<String, String> getRequestHeaders();
}
