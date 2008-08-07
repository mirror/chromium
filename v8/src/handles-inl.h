// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>
//

#ifndef V8_HANDLES_INL_H_
#define V8_HANDLES_INL_H_

#include "handles.h"
#include "api.h"

namespace v8 { namespace internal {

template<class T>
Handle<T>::Handle(T* obj) {
  location_ = reinterpret_cast<T**>(HandleScope::CreateHandle(obj));
}


template <class T>
inline T* Handle<T>::operator*() const {
  ASSERT(location_ != NULL);
  ASSERT(reinterpret_cast<Address>(*location_) != kHandleZapValue);
  return *location_;
}


#ifdef DEBUG
inline NoHandleAllocation::NoHandleAllocation() {
  ImplementationUtilities::HandleScopeData* current =
      ImplementationUtilities::CurrentHandleScope();
  extensions_ = current->extensions;
  // Shrink the current handle scope to make it impossible to do
  // handle allocations without an explicit handle scope.
  current->limit = current->next;
  current->extensions = -1;
}


inline NoHandleAllocation::~NoHandleAllocation() {
  // Restore state in current handle scope to re-enable handle
  // allocations.
  ImplementationUtilities::CurrentHandleScope()->extensions = extensions_;
}
#endif


} }  // namespace v8::internal

#endif  // V8_HANDLES_INL_H_
