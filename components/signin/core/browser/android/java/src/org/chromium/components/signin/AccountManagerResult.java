// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.signin;

import android.support.annotation.Nullable;

/**
 * AccountManagerResult encapsulates result of {@link AccountManagerFacade} method call. It is a
 * holder that contains either a value or an AccountManagerDelegateException that occured during
 * the call.
 *
 * @param <T> The type of value this class should contain.
 */
public class AccountManagerResult<T> {
    @Nullable
    private final T mValue;
    @Nullable
    private final AccountManagerDelegateException mException;

    public AccountManagerResult(T value) {
        mValue = value;
        mException = null;
    }

    public AccountManagerResult(AccountManagerDelegateException ex) {
        assert ex != null;
        mValue = null;
        mException = ex;
    }

    public T get() throws AccountManagerDelegateException {
        if (mException != null) {
            throw mException;
        }
        return mValue;
    }

    public boolean hasValue() {
        return mException == null;
    }

    public boolean hasException() {
        return mException != null;
    }

    public T getValue() {
        assert hasValue();
        return mValue;
    }

    public AccountManagerDelegateException getException() {
        assert hasException();
        return mException;
    }
}
