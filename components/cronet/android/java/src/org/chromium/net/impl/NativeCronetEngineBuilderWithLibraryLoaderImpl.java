// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net.impl;

import android.content.Context;

import org.chromium.net.CronetEngine.Builder.LibraryLoader;
import org.chromium.net.ICronetEngineBuilder;

/**
 * Implementation of {@link ICronetEngineBuilder} that builds native Cronet engine, including
 * the implementation of {@link ICronetEngineBuilder#setLibraryLoader}.
 */
public class NativeCronetEngineBuilderWithLibraryLoaderImpl extends NativeCronetEngineBuilderImpl {
    /**
     * Builder for Native Cronet Engine. It implements {@link #setLibraryLoader} method.
     * Default config enables SPDY, disables QUIC, SDCH and HTTP cache.
     *
     * @param context Android {@link Context} for engine to use.
     */
    public NativeCronetEngineBuilderWithLibraryLoaderImpl(Context context) {
        super(context);
    }

    @Override
    public CronetEngineBuilderImpl setLibraryLoader(LibraryLoader loader) {
        mLibraryLoader = new VersionSafeCallbacks.LibraryLoader(loader);
        return this;
    }
}
