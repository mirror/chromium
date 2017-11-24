// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_CYGPROFILE_LIGHTWEIGHT_CYGPROFILE_H_
#define TOOLS_CYGPROFILE_LIGHTWEIGHT_CYGPROFILE_H_

namespace cygprofile {

// Checkpoint the instrumentation. Checkpoints move from startup, to postload,
// to null (instrumentation disabled).
void Checkpoint(const std::string& option);
void DumpCheckpoints();

// Extract instrumentation data into the specified arrays.  Each char in these
// offset arrays corresponds to a word (sizeof(int)) of address space, and is
// the ascii character '1' if that word is a return target for an mcount()
// instrumented call, or '0' if it was untouched.
void ExtractInstrumentationData(std::vector<int8_t>* startup_offsets,
                                std::vector<int8_t>* postload_offsets);

// Dump an array as specified in ExtractInstrumentationData to disk.
void DumpInstrumentationArray(const std::string& specifier,
                              int pid,
                              const std::vector<int8_t>& offset_array);

}  // namespace cygprofile
#endif  // TOOLS_CYGPROFILE_LIGHTWEIGHT_CYGPROFILE_H_
