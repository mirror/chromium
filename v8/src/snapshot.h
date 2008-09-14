// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_SNAPSHOT_H_
#define V8_SNAPSHOT_H_

namespace v8 { namespace internal {

class Snapshot {
 public:
  // Initialize the VM from the given snapshot file. If snapshot_file is
  // NULL, use the internal snapshot instead. Returns false if no snapshot
  // could be found.
  static bool Initialize(const char* snapshot_file = NULL);

  // Disable the use of the internal snapshot.
  static void DisableInternal() { size_ = 0; }

  // Write snapshot to the given file. Returns true if snapshot was written
  // successfully.
  static bool WriteToFile(const char* snapshot_file);

 private:
  static const char data_[];
  static int size_;

  static bool Deserialize(const char* content, int len);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Snapshot);
};

} }  // namespace v8::internal

#endif  // V8_SNAPSHOT_H_
