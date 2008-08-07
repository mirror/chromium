// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_IC_INL_H_
#define V8_IC_INL_H_

#include "ic.h"
#include "debug.h"
#include "macro-assembler.h"

namespace v8 { namespace internal {


Address IC::address() {
  // Get the address of the call.
  Address result = pc() - Assembler::kTargetAddrToReturnAddrDist;

  // First check if any break points are active if not just return the address
  // of the call.
  if (!Debug::has_break_points()) return result;

  // At least one break point is active perform additional test to ensure that
  // break point locations are updated correctly.
  if (Debug::IsDebugBreak(Assembler::target_address_at(result))) {
    // If the call site is a call to debug break then return the address in
    // the original code instead of the address in the running code. This will
    // cause the original code to be updated and keeps the breakpoint active in
    // the running code.
    return OriginalCodeAddress();
  } else {
    // No break point here just return the address of the call.
    return result;
  }
}


Code* IC::GetTargetAtAddress(Address address) {
  Address target = Assembler::target_address_at(address);
  HeapObject* code = HeapObject::FromAddress(target - Code::kHeaderSize);
  // GetTargetAtAddress is called from IC::Clear which in turn is
  // called when marking objects during mark sweep. reinterpret_cast
  // is therefore used instead of the more appropriate
  // Code::cast. Code::cast does not work when the object's map is
  // marked.
  Code* result = reinterpret_cast<Code*>(code);
  ASSERT(result->is_inline_cache_stub());
  return result;
}


void IC::SetTargetAtAddress(Address address, Code* target) {
  ASSERT(target->is_inline_cache_stub());
  Assembler::set_target_address_at(address, target->instruction_start());
}


Map* IC::GetCodeCacheMapForObject(Object* object) {
  if (object->IsJSObject()) return JSObject::cast(object)->map();
  // If the object is a value, we use the prototype map for the cache.
  ASSERT(object->IsString() || object->IsNumber() || object->IsBoolean());
  return JSObject::cast(object->GetPrototype())->map();
}


} }  // namespace v8::internal

#endif  // V8_IC_INL_H_
