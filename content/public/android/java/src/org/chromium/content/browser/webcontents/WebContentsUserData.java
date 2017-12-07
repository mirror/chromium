// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.webcontents;

import org.chromium.content_public.browser.WebContents;

/**
 * Holds an object to be stored in {@code userDataMap} in {@link WebContents} for those
 * classes that have the lifetime of {@link WebContents} without hanging directly onto it.
 * To create an object of a class {@code MyClass}, define a static method
 * {@code fromWebContents()} where you call:
 * <code>
 * WebContentsUserData.fromWebContents(webContents, MyClass.class, MyClass::new);
 * </code>
 *
 * {@code MyClass} should have a contstructor that accepts only one parameter:
 * <code>
 * public MyClass(WebContents webContents);
 * </code>
 */
public final class WebContentsUserData {
    /**
     * @param <T> Class to instantiate.
     */
    public interface UserDataFactory<T> {
        /**
         *
         */
        T create(WebContents webContents);
    }

    private Object mObject;

    private <T> WebContentsUserData(T object) {
        mObject = object;
    }

    /**
     *
     */
    @SuppressWarnings("unchecked")
    public static <T> T fromWebContents(
            WebContents webContents, Class<T> key, UserDataFactory<T> userDataFactory) {
        // Casting to WebContentsImpl is safe since it's the actual implementation.
        WebContentsImpl webContentsImpl = (WebContentsImpl) webContents;
        WebContentsUserData data = webContentsImpl.getUserData(key);
        if (data == null) {
            T object = userDataFactory.create(webContents);
            assert object.getClass() == key;
            data = new WebContentsUserData(object);
            webContentsImpl.setUserData(key, data);
        }
        // Casting Object to T is safe since we make sure the object was of type T upon creation.
        return (T) data.mObject;
    }
}
