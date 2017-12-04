// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

/**
 *
 */
public interface SupportsUserData {
    /**
     * Marker interface for user data associated with a {@link WebContents}.
     * Implement this interface and populate it in {@link UserDataMap} for user data
     * object that shares the lifetime of WebContents but need not be held directly.
     * Convention is to refer them through {@link WebContentsImpl.getUserData()}
     * in their {@code fromWebContents(WebContents)} static method.
     */
    interface Data {}

    /**
     * Sets the Data of a particular class type T, with the class type for its key.
     * @param key
     * @param data
     */
    <T extends Data> void setUserData(Class<T> key, T data);

    /**
     * Gets the Data of the key of the given type.
     * @param key
     * @return
     */
    <T extends Data> T getUserData(Class<T> key);
}
