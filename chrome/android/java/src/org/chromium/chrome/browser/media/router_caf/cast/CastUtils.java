package org.chromium.chrome.browser.media.router_caf.cast;

import com.google.android.gms.cast.framework.CastContext;

import org.chromium.base.ContextUtils;

import com.google.android.gms.cast.CastDevice;

import java.util.ArrayList;
import java.util.List;

public class CastUtils {
    public static CastContext getCastContext() {
        return CastContext.getSharedInstance(ContextUtils.getApplicationContext());
    }

    public static List<String> getCapabilities(CastDevice device) {
        List<String> capabilities = new ArrayList<String>();
        if (device.hasCapability(CastDevice.CAPABILITY_AUDIO_IN)) {
            capabilities.add("audio_in");
        }
        if (device.hasCapability(CastDevice.CAPABILITY_AUDIO_OUT)) {
            capabilities.add("audio_out");
        }
        if (device.hasCapability(CastDevice.CAPABILITY_VIDEO_IN)) {
            capabilities.add("video_in");
        }
        if (device.hasCapability(CastDevice.CAPABILITY_VIDEO_OUT)) {
            capabilities.add("video_out");
        }
        return capabilities;
    }
}
