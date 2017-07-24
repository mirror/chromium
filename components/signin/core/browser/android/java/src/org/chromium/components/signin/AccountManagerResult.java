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
    private final T mResult;
    @Nullable
    private final AccountManagerDelegateException mException;

    private AccountManagerResult(
            @Nullable T result, @Nullable AccountManagerDelegateException exception) {
        mResult = result;
        mException = exception;
    }

    public AccountManagerResult(T result) {
        this(result, null);
    }

    public AccountManagerResult(AccountManagerDelegateException ex) {
        this(null, ex);
    }

    public T get() throws AccountManagerDelegateException {
        if (mException != null) {
            throw mException.wrapAsCause();
        }
        return mResult;
    }
}
