// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_ZONE_INL_H_
#define V8_ZONE_INL_H_

#include "zone.h"

namespace v8 { namespace internal {


inline void* Zone::New(int size) {
  ASSERT(AssertNoZoneAllocation::allow_allocation());
  // Round up the requested size to fit the alignment.
  size = RoundUp(size, kAlignment);

  // Check if the requested size is available without expanding.
  Address result = position_;
  if ((position_ += size) > limit_) result = NewExpand(size);

  // Check that the result has the proper alignment and return it.
  ASSERT(IsAddressAligned(result, kAlignment, 0));
  return reinterpret_cast<void*>(result);
}


} }  // namespace v8::internal

#endif  // V8_ZONE_INL_H_
