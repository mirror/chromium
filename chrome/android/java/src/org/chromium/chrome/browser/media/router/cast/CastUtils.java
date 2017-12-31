package org.chromium.chrome.browser.media.router.cast;

import com.google.android.gms.cast.framework.CastContext;

import org.chromium.base.ContextUtils;

public class CastUtils {
    public static CastContext getCastContext() {
        return CastContext.getSharedInstance(ContextUtils.getApplicationContext());
    }
}
