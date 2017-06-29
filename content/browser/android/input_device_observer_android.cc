// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/input_device_observer_android.h"

#include "base/memory/singleton.h"
#include "jni/InputDeviceObserver_jni.h"

using base::android::JavaParamRef;

// This macro provides the implementation for the observer notification methods.
#define NOTIFY_OBSERVERS(method_decl, observer_call)           \
  void InputDeviceObserverAndroid::method_decl {               \
    for (ui::InputDeviceEventObserver & observer : observers_) \
      observer.observer_call;                                  \
  }

namespace content {

bool RegisterInputDeviceObserver(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

InputDeviceObserverAndroid::InputDeviceObserverAndroid() {}

InputDeviceObserverAndroid::~InputDeviceObserverAndroid() {}

InputDeviceObserverAndroid* InputDeviceObserverAndroid::GetInstance() {
  return base::Singleton<
      InputDeviceObserverAndroid,
      base::LeakySingletonTraits<InputDeviceObserverAndroid>>::get();
}

void InputDeviceObserverAndroid::AddObserver(
    ui::InputDeviceEventObserver* observer) {
  observers_.AddObserver(observer);
}

void InputDeviceObserverAndroid::RemoveObserver(
    ui::InputDeviceEventObserver* observer) {
  observers_.RemoveObserver(observer);
}

static void InputConfigurationChanged(JNIEnv* env,
                                      const JavaParamRef<jobject>& obj) {
  InputDeviceObserverAndroid::GetInstance()
      ->NotifyObserversTouchpadDeviceConfigurationChanged();
  InputDeviceObserverAndroid::GetInstance()
      ->NotifyObserversKeyboardDeviceConfigurationChanged();
  InputDeviceObserverAndroid::GetInstance()
      ->NotifyObserversMouseDeviceConfigurationChanged();
}

NOTIFY_OBSERVERS(NotifyObserversMouseDeviceConfigurationChanged(),
                 OnMouseDeviceConfigurationChanged());
NOTIFY_OBSERVERS(NotifyObserversTouchpadDeviceConfigurationChanged(),
                 OnTouchpadDeviceConfigurationChanged());
NOTIFY_OBSERVERS(NotifyObserversKeyboardDeviceConfigurationChanged(),
                 OnKeyboardDeviceConfigurationChanged());

}  // namespace content