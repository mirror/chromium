// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.password;

import org.chromium.base.ObserverList;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.PasswordManagerHandler;
import org.chromium.chrome.browser.PasswordUIView;

/**
 * A provider for PasswordManagerHandler implementations, handling the choice of the proper one
 * (production vs. testing), its lifetime and multiple observers.
 *
 * This class is used by the code responsible for Chrome's passwords settings. There can only be one
 * instance of Chrome's passwords settings opened at a time (although more clients of
 * PasswordManagerHandler can live as nested settings pages). Therefore, the provider can be just
 * one. However, it cannot be just a collection of static data members and methods, because the
 * managed PasswordManagerHandler instances need to refer to it as an observer. For that reason, the
 * provider is a singleton.
 */
public class PasswordManagerHandlerProvider implements PasswordManagerHandler.PasswordListObserver {
    // The lazily-created instance.
    private static PasswordManagerHandlerProvider sInstance;

    /** Private constructor, use GetInstance() instead. */
    private PasswordManagerHandlerProvider() {}

    public static PasswordManagerHandlerProvider getInstance() {
        if (sInstance == null) sInstance = new PasswordManagerHandlerProvider();
        return sInstance;
    }

    // The production implementation of PasswordManagerHandler is |sPasswordUIView|, instantiated on
    // demand. Tests might want to override that by providing a fake implementation through
    // setPasswordManagerHandler, which is then kept in |sPasswordManagerHandler|.
    private PasswordUIView mPasswordUIView;
    private PasswordManagerHandler mPasswordManagerHandler;

    // This class is itself a PasswordListObserver, listening directly to a PasswordManagerHandler
    // implementation. But it also keeps a list of other observers, to which it forwards the events.
    private ObserverList<PasswordManagerHandler.PasswordListObserver> mObservers =
            new ObserverList<PasswordManagerHandler.PasswordListObserver>();

    /**
     * Sets a testing implementation of PasswordManagerHandler to be used. It overrides the
     * production one even if it exists. The caller needs to ensure that |this| becomes an observer
     * of |passwordManagerHandler|. Also, this must not be called when there are already some
     * observers in |mObservers|, because of special handling of the production implementation of
     * PasswordManagerHandler on removing the last observer.
     */
    @VisibleForTesting
    public void setPasswordManagerHandler(PasswordManagerHandler passwordManagerHandler) {
        assert mObservers.isEmpty();
        mPasswordManagerHandler = passwordManagerHandler;
    }

    /**
     * A convenience function to choose between the production and static PasswordManagerHandler
     * implementation.
     */
    public PasswordManagerHandler getPasswordManagerHandler() {
        if (mPasswordManagerHandler != null) return mPasswordManagerHandler;
        return mPasswordUIView;
    }

    /**
     * This method creates the production implementation of PasswordManagerHandler and saves it into
     * mPasswordUIView.
     */
    private void createPasswordManagerHandler() {
        assert mPasswordUIView == null;
        mPasswordUIView = new PasswordUIView(this);
    }

    /**
     * Starts forwarding events from the PasswordManagerHandler implementation to |observer|.
     */
    public void addObserver(PasswordManagerHandler.PasswordListObserver observer) {
        if (getPasswordManagerHandler() == null) createPasswordManagerHandler();
        mObservers.addObserver(observer);
    }

    public void removeObserver(PasswordManagerHandler.PasswordListObserver observer) {
        mObservers.removeObserver(observer);
        // If this was the last observer of the production implementation of PasswordManagerHandler,
        // call destroy on it to close the connection to the native C++ code.
        if (mObservers.isEmpty() && mPasswordManagerHandler == null) {
            mPasswordUIView.destroy();
            mPasswordUIView = null;
        }
    }

    @Override
    public void passwordListAvailable(int count) {
        for (PasswordManagerHandler.PasswordListObserver observer : mObservers) {
            observer.passwordListAvailable(count);
        }
    }

    @Override
    public void passwordExceptionListAvailable(int count) {
        for (PasswordManagerHandler.PasswordListObserver observer : mObservers) {
            observer.passwordExceptionListAvailable(count);
        }
    }
}
