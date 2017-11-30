// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.webcontents;

/**
 * Marker interface for user data associated with a {@link WebContents}.
 * Implement this interface and populate it in {@link UserDataMap} for user data
 * object that shares the lifetime of WebContents but need not be held directly.
 * Convention is to refer them through {@link WebContentsImpl.getUserData()}
 * in their {@code fromWebContents(WebContents)} static method.
 */
public interface UserData {}
