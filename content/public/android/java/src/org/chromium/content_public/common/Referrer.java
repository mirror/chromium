// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.common;

import org.chromium.blink_public.web.WebReferrerPolicy;

/**
 * Container that holds together a referrer URL along with the referrer policy set on the
 * originating frame. This corresponds to native content/public/common/referrer.h.
 */
public class Referrer {
    // TODO(yusufo): Remove these constants now that org.chromium.blink_public.web.WebReferrerPolicy
    // exists.
    /** @see blink::WebReferrerPolicyAlways */
    public static final int REFERRER_POLICY_ALWAYS = WebReferrerPolicy.WEB_REFERRER_POLICY_ALWAYS;

    /** @see blink::WebReferrerPolicyDefault */
    public static final int REFERRER_POLICY_DEFAULT = WebReferrerPolicy.WEB_REFERRER_POLICY_DEFAULT;

    private final String mUrl;
    private final int mPolicy;

    /**
     * Constructs a referrer with the given url and policy.
     */
    public Referrer(String url, int policy) {
        mUrl = url;
        mPolicy = policy;
    }

    public String getUrl() {
        return mUrl;
    }

    public int getPolicy() {
        return mPolicy;
    }
}
