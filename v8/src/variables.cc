// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#include "v8.h"

#include "ast.h"
#include "scopes.h"
#include "variables.h"

namespace v8 { namespace internal {

// ----------------------------------------------------------------------------
// Implementation UseCount.

UseCount::UseCount()
  : nreads_(0),
    nwrites_(0) {
}


void UseCount::RecordRead(int weight) {
  ASSERT(weight > 0);
  nreads_ += weight;
  // We must have a positive nreads_ here. Handle
  // any kind of overflow by setting nreads_ to
  // some large-ish value.
  if (nreads_ <= 0) nreads_ = 1000000;
  ASSERT(is_read() & is_used());
}


void UseCount::RecordWrite(int weight) {
  ASSERT(weight > 0);
  nwrites_ += weight;
  // We must have a positive nwrites_ here. Handle
  // any kind of overflow by setting nwrites_ to
  // some large-ish value.
  if (nwrites_ <= 0) nwrites_ = 1000000;
  ASSERT(is_written() && is_used());
}


void UseCount::RecordAccess(int weight) {
  RecordRead(weight);
  RecordWrite(weight);
}


void UseCount::RecordUses(UseCount* uses) {
  if (uses->nreads() > 0) RecordRead(uses->nreads());
  if (uses->nwrites() > 0) RecordWrite(uses->nwrites());
}


#ifdef DEBUG
void UseCount::Print() {
  // PrintF("r = %d, w = %d", nreads_, nwrites_);
  PrintF("%du = %dr + %dw", nuses(), nreads(), nwrites());
}
#endif


// ----------------------------------------------------------------------------
// Implementation Variable.


const char* Variable::Mode2String(Mode mode) {
  switch (mode) {
    case VAR: return "VAR";
    case CONST: return "CONST";
    case DYNAMIC: return "DYNAMIC";
    case INTERNAL: return "INTERNAL";
    case TEMPORARY: return "TEMPORARY";
  }
  UNREACHABLE();
  return NULL;
}


Property* Variable::AsProperty() {
  return rewrite_ == NULL ? NULL : rewrite_->AsProperty();
}


Variable* Variable::AsVariable()  {
  return rewrite_ == NULL || rewrite_->AsSlot() != NULL ? this : NULL;
}


Slot* Variable::slot() const {
  return rewrite_ != NULL ? rewrite_->AsSlot() : NULL;
}


Variable::Variable(Scope* scope,
                   Handle<String> name,
                   Mode mode,
                   bool is_valid_LHS,
                   bool is_this)
  : scope_(scope),
    name_(name),
    mode_(mode),
    is_valid_LHS_(is_valid_LHS),
    is_this_(is_this),
    is_accessed_from_inner_scope_(false),
    rewrite_(NULL) {
  // names must be canonicalized for fast equality checks
  ASSERT(name->IsSymbol());
}


bool Variable::is_global() const {
  // Temporaries are never global, they must always be allocated in the
  // activation frame.
  return mode_ != TEMPORARY && scope_ != NULL && scope_->is_global_scope();
}


} }  // namespace v8::internal
