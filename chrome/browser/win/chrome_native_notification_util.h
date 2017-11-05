// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WIN_CHROME_NATIVE_NOTIFICATION_UTIL_H_
#define CHROME_BROWSER_WIN_CHROME_NATIVE_NOTIFICATION_UTIL_H_

#include "windows.h"

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/win/chrome_native_notification_util.h"

#include <NotificationActivationCallback.h>
#include <wrl.h>

namespace mswr = Microsoft::WRL;
namespace mswrd = Microsoft::WRL::Details;

// The three macros below are redefinitions of CoCreatableClass,
// CoCreatableClassWithFactory, and InternalWrlCreateCreatorMap in
// winrt/wrl/module.h
//
// The original definitions caused win_clang compile errors. To fix the errors,
// we removed __declspec(selectany) in InternalWrlCreateCreatorMap, and added
// braces to "0" and "runtimeClassName" in InternalWrlCreateCreatorMap.

#define CoCreatableClassClangFormat(className)      \
  CoCreatableClassWithFactoryClangFormat(className, \
                                         mswr::SimpleClassFactory<className>)

#define CoCreatableClassWithFactoryClangFormat(className, factory) \
  InternalWrlCreateCreatorMapClangFormat(                          \
      className##_COM, &__uuidof(className), nullptr,              \
                                                                   \
      mswrd::CreateClassFactory<factory>, "minATL$__f")

#define InternalWrlCreateCreatorMapClangFormat(className, runtimeClassName, \
                                               trustLevel,                  \
                                                                            \
                                               creatorFunction, section)    \
  mswrd::FactoryCache __objectFactory__##className = {nullptr, {0}};        \
  extern const mswrd::CreatorMap __object_##className = {                   \
      creatorFunction,                                                      \
      {runtimeClassName},                                                   \
      trustLevel,                                                           \
      &__objectFactory__##className,                                        \
      nullptr};                                                             \
  extern "C" __declspec(allocate(section))                                  \
      const mswrd::CreatorMap* const __minATLObjMap_##className =           \
          &__object_##className;                                            \
  WrlCreatorMapIncludePragma(className)

// A Win32 component that participates with Action Center will need to create a
// COM component that exposes the INotificationActivationCallback interface.
//
// TODO(chengx): Different id for different browser distribution.
#define CLSID_KEY "635EFA6F-08D6-4EC9-BD14-8A0FDE975159"

class DECLSPEC_UUID(CLSID_KEY) NotificationActivator
    : public mswr::RuntimeClass<mswr::RuntimeClassFlags<mswr::ClassicCom>,
                                INotificationActivationCallback> {
 public:
  HRESULT STDMETHODCALLTYPE Activate(
      _In_ LPCWSTR /*appUserModelId*/,
      _In_ LPCWSTR invokedArgs,
      /*_In_reads_(dataCount)*/ const NOTIFICATION_USER_INPUT_DATA* /*data*/,
      ULONG /*dataCount*/) override {
    // TODO(chengx): Implement.
    return S_OK;
  }
};
CoCreatableClassClangFormat(NotificationActivator);

#endif  // CHROME_BROWSER_WIN_CHROME_NATIVE_NOTIFICATION_UTIL_H_
