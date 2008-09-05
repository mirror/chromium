// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

// Used for building without snapshots.

#include "v8.h"

#include "snapshot.h"

namespace v8 { namespace internal {

const char Snapshot::data_[] = { 0 };
int Snapshot::size_ = 0;

} }  // namespace v8::internal
