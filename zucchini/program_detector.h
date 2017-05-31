// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_PROGRAM_DETECTOR_H_
#define ZUCCHINI_PROGRAM_DETECTOR_H_

#include <memory>

#include "base/macros.h"
#include "zucchini/disassembler.h"
#include "zucchini/region.h"

namespace zucchini {

// Attempts to detect an executable located at start of |image|. If found,
// returns the corresponding disassembler. Otherwise returns null.
std::unique_ptr<Disassembler> MakeDisassemblerWithoutFallback(Region image);

// Similar to MakeDisassemblerWithoutFallback(), but on failure, returns an
// instance of DisassemblerNoOp instead of null.
std::unique_ptr<Disassembler> MakeDisassembler(Region image);

// Returns a new instance of DisassemblerNoOp.
std::unique_ptr<Disassembler> MakeNoOpDisassembler(Region image);

// A stateful class to detect a program possibly found at the start of a Region.
class SingleProgramDetector {
 public:
  SingleProgramDetector() : exe_type_(EXE_TYPE_UNKNOWN) {}
  virtual ~SingleProgramDetector() {}

  // Attempts to detect a program at the start of |sub_image|. If successful,
  // stores results and returns true. Otherwise returns false. In this case the
  // stored results are undefined and should not be accessed.
  virtual bool Run(Region sub_image) = 0;

  // Accessors for stored results, valid only after successful Run().
  ExecutableType exe_type() const { return exe_type_; }
  Region region() const { return region_; }

 protected:
  // The type of the detected executable.
  ExecutableType exe_type_;

  // The region containing the detected exectuable.
  Region region_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SingleProgramDetector);
};

// SingleProgramDetector implemented using Disassemblers. Reusable.
class DisassemblerSingleProgramDetector : public SingleProgramDetector {
 public:
  DisassemblerSingleProgramDetector();
  ~DisassemblerSingleProgramDetector() override;

  bool Run(Region sub_image) override;

  // Accessor for underlying Disassembler, valid only after successful Run().
  const Disassembler& dis() const { return *dis_; }

 private:
  std::unique_ptr<Disassembler> dis_;

  DISALLOW_COPY_AND_ASSIGN(DisassemblerSingleProgramDetector);
};

// A class to scan through an image and iteratively detect embedded executables.
class ProgramDetector {
 public:
  explicit ProgramDetector(Region image);
  ~ProgramDetector();

  // Scans for the next exectuable using |detector|. On success, returns true
  // and asserts that |detector| contains results. Otherwise returns false.
  // Typical this should be used in a while loop:
  //   Implementation_Of_SingleProgramDetector detector;
  //   while (program_detector.GetNext(&detector)) { ... }.
  bool GetNext(SingleProgramDetector* detector);

 private:
  Region image_;
  size_t pos_ = 0;

  DISALLOW_COPY_AND_ASSIGN(ProgramDetector);
};

}  // namespace zucchini

#endif  // ZUCCHINI_PROGRAM_DETECTOR_H_
