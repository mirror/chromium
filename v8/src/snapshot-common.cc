// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

// The common functionality when building with or without snapshots.

#include "v8.h"

#include "api.h"
#include "serialize.h"
#include "snapshot.h"

namespace v8 { namespace internal {

bool Snapshot::Deserialize(const char* content, int len) {
  Deserializer des(content, len);
  des.GetFlags();
  return V8::Initialize(&des);
}


bool Snapshot::Initialize(const char* snapshot_file) {
  if (snapshot_file) {
    int len;
    char* str = ReadChars(snapshot_file, &len);
    if (!str) return false;
    bool result = Deserialize(str, len);
    DeleteArray(str);
    return result;
  } else if (size_ > 0) {
    return Deserialize(data_, size_);
  }
  return false;
}


bool Snapshot::WriteToFile(const char* snapshot_file) {
  Serializer ser;
  ser.Serialize();
  char* str;
  int len;
  ser.Finalize(&str, &len);

  int written = WriteChars(snapshot_file, str, len);

  DeleteArray(str);
  return written == len;
}


} }  // namespace v8::internal
