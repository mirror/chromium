// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.omnibox;

import android.accessibilityservice.AccessibilityServiceInfo;
import android.content.Context;
import android.content.pm.ServiceInfo;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.IAccessibilityManager;
import com.android.server.accessibility.AccessibilityManagerService;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.annotation.HiddenApi;
import org.robolectric.util.ReflectionHelpers;
import org.robolectric.internal.Shadow;
import org.robolectric.util.ReflectionHelpers.ClassParameter;

import java.util.List;

import static android.os.Build.VERSION_CODES.KITKAT;
import static org.robolectric.RuntimeEnvironment.getApiLevel;

/**
 * Shadow for {@link android.view.accessibility.AccessibilityManager}.
 * This is a modified version of Robolectric's own shadow version to fix crbug.com/753369.
 */
@Implements(AccessibilityManager.class)
public class ChromiumShadowAccessibilityManager {
  private static AccessibilityManager sInstance;
  private static final Object sInstanceSync = new Object();

  private boolean enabled;
  private List<AccessibilityServiceInfo> installedAccessibilityServiceList;
  private List<AccessibilityServiceInfo> enabledAccessibilityServiceList;
  private List<ServiceInfo> accessibilityServiceList;
  private boolean touchExplorationEnabled;

  @HiddenApi
  @Implementation
  public static AccessibilityManager getInstance(Context context) throws Exception {
    synchronized (sInstanceSync) {
      if (sInstance == null) {
          sInstance = createInstance(context);
      }
    }
    return sInstance;
  }

  private static AccessibilityManager createInstance(Context context) throws Exception {
    if (getApiLevel() >= KITKAT) {
      AccessibilityManager accessibilityManager = Shadow.newInstance(AccessibilityManager.class, new Class[]{Context.class, IAccessibilityManager.class, int.class}, new Object[]{context, new AccessibilityManagerService(context), 0});
      ReflectionHelpers.setField(accessibilityManager, "mHandler", new MyHandler(context.getMainLooper(), accessibilityManager));
      return accessibilityManager;
    } else {
      AccessibilityManager accessibilityManager = Shadow.newInstance(AccessibilityManager.class, new Class[0], new Object[0]);
      ReflectionHelpers.setField(accessibilityManager, "mHandler", new MyHandler(context.getMainLooper(), accessibilityManager));
      return accessibilityManager;
    }
  }

  @Implementation
  public boolean addAccessibilityStateChangeListener(AccessibilityManager.AccessibilityStateChangeListener listener) {
    return true;
  }

  @Implementation
  public boolean removeAccessibilityStateChangeListener(AccessibilityManager.AccessibilityStateChangeListener listener) {
    return true;
  }

  @Implementation
  public List<ServiceInfo> getAccessibilityServiceList () {
    return accessibilityServiceList;
  }

  public void setAccessibilityServiceList(List<ServiceInfo> accessibilityServiceList) {
    this.accessibilityServiceList = accessibilityServiceList;
  }

  @Implementation
  public List<AccessibilityServiceInfo> getEnabledAccessibilityServiceList (int feedbackTypeFlags) {
    return enabledAccessibilityServiceList;
  }

  public void setEnabledAccessibilityServiceList(List<AccessibilityServiceInfo> enabledAccessibilityServiceList) {
    this.enabledAccessibilityServiceList = enabledAccessibilityServiceList;
  }

  @Implementation
  public List<AccessibilityServiceInfo> getInstalledAccessibilityServiceList () {
    return installedAccessibilityServiceList;
  }

  public void setInstalledAccessibilityServiceList(List<AccessibilityServiceInfo> installedAccessibilityServiceList) {
    this.installedAccessibilityServiceList = installedAccessibilityServiceList;
  }

  @Implementation
  public boolean isEnabled () {
    return enabled;
  }

  public void setEnabled(boolean enabled) {
    this.enabled = enabled;
  }

  @Implementation
  public boolean isTouchExplorationEnabled () {
    return touchExplorationEnabled;
  }

  public void setTouchExplorationEnabled(boolean touchExplorationEnabled) {
    this.touchExplorationEnabled = touchExplorationEnabled;
  }

  static class MyHandler extends Handler {
    private static final int DO_SET_STATE = 10;
    private final AccessibilityManager accessibilityManager;

    MyHandler(Looper mainLooper, AccessibilityManager accessibilityManager) {
      super(mainLooper);
      this.accessibilityManager = accessibilityManager;
    }

    @Override
    public void handleMessage(Message message) {
      switch (message.what) {
        case DO_SET_STATE:
          ReflectionHelpers.callInstanceMethod(accessibilityManager, "setState", ClassParameter.from(int.class, message.arg1));
          return;
        default:
          Log.w("AccessibilityManager", "Unknown message type: " + message.what);
      }
    }
  }
}
