// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/weak_ptr_object.h"

#import <Foundation/Foundation.h>

@interface WeakPtrObject : NSObject {
 @public
  void* weak_ptr;
}
@end
@implementation WeakPtrObject
@end

namespace ui {
namespace internal {

WeakPtrObject* WeakPtrObjectFactoryBase::Create(void* owner) {
  WeakPtrObject* object = [[WeakPtrObject alloc] init];
  object->weak_ptr = owner;
  return object;
}

void* WeakPtrObjectFactoryBase::UnWrap(WeakPtrObject* handle) {
  return handle->weak_ptr;
}

void WeakPtrObjectFactoryBase::InvalidateAndRelease(WeakPtrObject* handle) {
  handle->weak_ptr = nullptr;
  [handle release];
}

}  // namespace internal
}  // namespace ui
