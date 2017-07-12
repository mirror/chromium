package com.android.support_library_prototype;

interface WebResourceError {
    /**
     * Gets the error code of the error. The code corresponds to one
     * of the ERROR_* constants in {@link WebViewClient}.
     *
     * @return The error code of the error
     */
    int getErrorCode();

    /**
     * Gets the string describing the error. Descriptions are localized,
     * and thus can be used for communicating the problem to the user.
     *
     * @return The description of the error
     */
    CharSequence getDescription();
}