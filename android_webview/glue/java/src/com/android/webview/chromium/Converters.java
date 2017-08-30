// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.content.Intent;

/** A utility that converts between AwContents types and gwebkit types. */
final class Converters {
    private Converters() {}

    public static org.chromium.android_webview.AwSettings.ZoomDensity toAwType(
            android.webkit.WebSettings.ZoomDensity value) {
        if (value == null) {
            return null;
        }
        switch (value) {
            case CLOSE:
                return org.chromium.android_webview.AwSettings.ZoomDensity.CLOSE;
            case FAR:
                return org.chromium.android_webview.AwSettings.ZoomDensity.FAR;
            case MEDIUM:
                return org.chromium.android_webview.AwSettings.ZoomDensity.MEDIUM;
            default:
                throw new IllegalArgumentException("Unsupported value: " + value);
        }
    }

    public static android.webkit.WebSettings.ZoomDensity fromAwType(
            org.chromium.android_webview.AwSettings.ZoomDensity value) {
        if (value == null) {
            return null;
        }
        switch (value) {
            case CLOSE:
                return android.webkit.WebSettings.ZoomDensity.CLOSE;
            case FAR:
                return android.webkit.WebSettings.ZoomDensity.FAR;
            case MEDIUM:
                return android.webkit.WebSettings.ZoomDensity.MEDIUM;
            default:
                throw new IllegalArgumentException("Unsupported value: " + value);
        }
    }

    public static org.chromium.android_webview.AwSettings.LayoutAlgorithm toAwType(
            android.webkit.WebSettings.LayoutAlgorithm value) {
        if (value == null) {
            return null;
        }
        switch (value) {
            case NARROW_COLUMNS:
                return org.chromium.android_webview.AwSettings.LayoutAlgorithm.NARROW_COLUMNS;
            case NORMAL:
                return org.chromium.android_webview.AwSettings.LayoutAlgorithm.NORMAL;
            case SINGLE_COLUMN:
                return org.chromium.android_webview.AwSettings.LayoutAlgorithm.SINGLE_COLUMN;
            case TEXT_AUTOSIZING:
                return org.chromium.android_webview.AwSettings.LayoutAlgorithm.TEXT_AUTOSIZING;
            default:
                throw new IllegalArgumentException("Unsupported value: " + value);
        }
    }

    public static android.webkit.WebSettings.LayoutAlgorithm fromAwType(
            org.chromium.android_webview.AwSettings.LayoutAlgorithm value) {
        if (value == null) {
            return null;
        }
        switch (value) {
            case NARROW_COLUMNS:
                return android.webkit.WebSettings.LayoutAlgorithm.NARROW_COLUMNS;
            case NORMAL:
                return android.webkit.WebSettings.LayoutAlgorithm.NORMAL;
            case SINGLE_COLUMN:
                return android.webkit.WebSettings.LayoutAlgorithm.SINGLE_COLUMN;
            case TEXT_AUTOSIZING:
                return android.webkit.WebSettings.LayoutAlgorithm.TEXT_AUTOSIZING;
            default:
                throw new IllegalArgumentException("Unsupported value: " + value);
        }
    }

    public static org.chromium.android_webview.AwSettings.PluginState toAwType(
            android.webkit.WebSettings.PluginState value) {
        if (value == null) {
            return null;
        }
        switch (value) {
            case OFF:
                return org.chromium.android_webview.AwSettings.PluginState.OFF;
            case ON:
                return org.chromium.android_webview.AwSettings.PluginState.ON;
            case ON_DEMAND:
                return org.chromium.android_webview.AwSettings.PluginState.ON_DEMAND;
            default:
                throw new IllegalArgumentException("Unsupported value: " + value);
        }
    }

    public static android.webkit.WebSettings.PluginState fromAwType(
            org.chromium.android_webview.AwSettings.PluginState value) {
        if (value == null) {
            return null;
        }
        switch (value) {
            case OFF:
                return android.webkit.WebSettings.PluginState.OFF;
            case ON:
                return android.webkit.WebSettings.PluginState.ON;
            case ON_DEMAND:
                return android.webkit.WebSettings.PluginState.ON_DEMAND;
            default:
                throw new IllegalArgumentException("Unsupported value: " + value);
        }
    }

    public static <T> org.chromium.android_webview.AwValueCallback<T> toAwType(
            final android.webkit.ValueCallback<T> callback) {
        if (callback == null) {
            return null;
        }
        return new org.chromium.android_webview.AwValueCallback<T>() {
            @Override
            public void onReceiveValue(T value) {
                callback.onReceiveValue(value);
            }
        };
    }

    public static <T> android.webkit.ValueCallback<T> fromAwType(
            final org.chromium.android_webview.AwValueCallback<T> callback) {
        if (callback == null) {
            return null;
        }
        return new android.webkit.ValueCallback<T>() {
            @Override
            public void onReceiveValue(T value) {
                callback.onReceiveValue(value);
            }
        };
    }

    public static org.chromium.android_webview.AwConsoleMessage toAwType(
            android.webkit.ConsoleMessage value) {
        if (value == null) {
            return null;
        }
        return new org.chromium.android_webview.AwConsoleMessage(value.message(), value.sourceId(),
                value.lineNumber(), toAwType(value.messageLevel()));
    }

    public static android.webkit.ConsoleMessage fromAwType(
            org.chromium.android_webview.AwConsoleMessage value) {
        if (value == null) {
            return null;
        }
        return new android.webkit.ConsoleMessage(value.message(), value.sourceId(),
                value.lineNumber(), fromAwType(value.messageLevel()));
    }

    public static org.chromium.android_webview.AwConsoleMessage.MessageLevel toAwType(
            android.webkit.ConsoleMessage.MessageLevel value) {
        if (value == null) {
            return null;
        }
        switch (value) {
            case TIP:
                return org.chromium.android_webview.AwConsoleMessage.MessageLevel.TIP;
            case LOG:
                return org.chromium.android_webview.AwConsoleMessage.MessageLevel.LOG;
            case WARNING:
                return org.chromium.android_webview.AwConsoleMessage.MessageLevel.WARNING;
            case ERROR:
                return org.chromium.android_webview.AwConsoleMessage.MessageLevel.ERROR;
            case DEBUG:
                return org.chromium.android_webview.AwConsoleMessage.MessageLevel.DEBUG;
            default:
                throw new IllegalArgumentException("Unsupported value: " + value);
        }
    }

    public static android.webkit.ConsoleMessage.MessageLevel fromAwType(
            org.chromium.android_webview.AwConsoleMessage.MessageLevel value) {
        if (value == null) {
            return null;
        }
        switch (value) {
            case TIP:
                return android.webkit.ConsoleMessage.MessageLevel.TIP;
            case LOG:
                return android.webkit.ConsoleMessage.MessageLevel.LOG;
            case WARNING:
                return android.webkit.ConsoleMessage.MessageLevel.WARNING;
            case ERROR:
                return android.webkit.ConsoleMessage.MessageLevel.ERROR;
            case DEBUG:
                return android.webkit.ConsoleMessage.MessageLevel.DEBUG;
            default:
                throw new IllegalArgumentException("Unsupported value: " + value);
        }
    }

    public static org.chromium.android_webview.AwGeolocationPermissions.Callback toAwType(
            final android.webkit.GeolocationPermissions.Callback callback) {
        if (callback == null) {
            return null;
        }
        return new org.chromium.android_webview.AwGeolocationPermissions.Callback() {
            @Override
            public void invoke(String origin, boolean allow, boolean retain) {
                callback.invoke(origin, allow, retain);
            }
        };
    }

    public static android.webkit.GeolocationPermissions.Callback fromAwType(
            final org.chromium.android_webview.AwGeolocationPermissions.Callback callback) {
        if (callback == null) {
            return null;
        }
        return new android.webkit.GeolocationPermissions.Callback() {
            @Override
            public void invoke(String origin, boolean allow, boolean retain) {
                callback.invoke(origin, allow, retain);
            }
        };
    }

    public static org.chromium.android_webview.AwContentsClient.CustomViewCallback toAwType(
            final android.webkit.WebChromeClient.CustomViewCallback callback) {
        if (callback == null) {
            return null;
        }
        return new org.chromium.android_webview.AwContentsClient.CustomViewCallback() {
            @Override
            public void onCustomViewHidden() {
                callback.onCustomViewHidden();
            }
        };
    }

    public static android.webkit.WebChromeClient.CustomViewCallback fromAwType(
            final org.chromium.android_webview.AwContentsClient.CustomViewCallback callback) {
        if (callback == null) {
            return null;
        }
        return new android.webkit.WebChromeClient.CustomViewCallback() {
            @Override
            public void onCustomViewHidden() {
                callback.onCustomViewHidden();
            }
        };
    }

    public static org.chromium.android_webview.AwContentsClient.FileChooserParams toAwType(
            final android.webkit.WebChromeClient.FileChooserParams value) {
        if (value == null) {
            return null;
        }
        return new org.chromium.android_webview.AwContentsClient.FileChooserParams() {
            @Override
            public int getMode() {
                return value.getMode();
            }

            @Override
            public String[] getAcceptTypes() {
                return value.getAcceptTypes();
            }

            @Override
            public boolean isCaptureEnabled() {
                return value.isCaptureEnabled();
            }

            @Override
            public CharSequence getTitle() {
                return value.getTitle();
            }

            @Override
            public String getFilenameHint() {
                return value.getFilenameHint();
            }

            @Override
            public Intent createIntent() {
                return value.createIntent();
            }
        };
    }

    public static android.webkit.WebChromeClient.FileChooserParams fromAwType(
            final org.chromium.android_webview.AwContentsClient.FileChooserParams value) {
        if (value == null) {
            return null;
        }
        return new android.webkit.WebChromeClient.FileChooserParams() {
            @Override
            public int getMode() {
                return value.getMode();
            }

            @Override
            public String[] getAcceptTypes() {
                return value.getAcceptTypes();
            }

            @Override
            public boolean isCaptureEnabled() {
                return value.isCaptureEnabled();
            }

            @Override
            public CharSequence getTitle() {
                return value.getTitle();
            }

            @Override
            public String getFilenameHint() {
                return value.getFilenameHint();
            }

            @Override
            public Intent createIntent() {
                return value.createIntent();
            }
        };
    }
}
