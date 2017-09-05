// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_U2F_BLE_FRAMES_
#define DEVICE_U2F_U2F_BLE_FRAMES_

#include <stdint.h>
#include <vector>

namespace device {

class U2fBleFrameFragment;
class U2fBleFrameInitializationFragment;
class U2fBleFrameContinuationFragment;

class U2fBleFrame {
 public:
  enum class Command : uint8_t {
    PING = 0x81,
    KEEPALIVE = 0x82,
    MSG = 0x83,
    ERROR = 0xBF,
  };

  enum class Keepalive : uint8_t {
    PROCESSING = 0x01,
    TUP_NEEDED = 0x02,
  };

  enum class Error : uint8_t {
    INVALID_CMD = 0x01,  // The command in the request is unknown/invalid.
    INVALID_PAR = 0x02,  // The parameters of the command are invalid/missing.
    INVALID_LEN = 0x03,  // The length of the request is invalid.
    INVALID_SEQ = 0x04,  // The sequence number is invalid.
    REQ_TIMEOUT = 0x05,  // The request timed out.
    NA_1 = 0x06,         // Value reserved (HID).
    NA_2 = 0x0a,         // Value reserved (HID).
    NA_3 = 0x0b,         // Value reserved (HID).
    OTHER = 0x7f,        // Other, unspecified error.
  };

  U2fBleFrame(Command command = Command::MSG);
  U2fBleFrame(Command command, std::vector<uint8_t> data);

  U2fBleFrame(U2fBleFrame&&);
  U2fBleFrame& operator=(U2fBleFrame&&);

  ~U2fBleFrame();

  Command command() const { return command_; }

  const std::vector<uint8_t>& data() const { return data_; }
  std::vector<uint8_t>& data() { return data_; }

  size_t ToFragments(
      size_t max_fragment_size,
      U2fBleFrameInitializationFragment* initial_fragment,
      std::vector<U2fBleFrameContinuationFragment>* other_fragments) const;

 private:
  Command command_;
  std::vector<uint8_t> data_;
};

class U2fBleFrameFragment {
 public:
  const uint8_t* data() const { return data_; }
  size_t size() const { return size_; }

 private:
  friend class U2fBleFrameInitializationFragment;
  friend class U2fBleFrameContinuationFragment;

  U2fBleFrameFragment(const uint8_t* data, size_t size)
      : data_(data), size_(size) {}

  const uint8_t* data_;
  size_t size_;
};

class U2fBleFrameInitializationFragment : public U2fBleFrameFragment {
 public:
  static constexpr size_t kHeaderSize = 3;

  static U2fBleFrameInitializationFragment Parse(const uint8_t* data,
                                                 size_t size);

  U2fBleFrameInitializationFragment(U2fBleFrame::Command command,
                                    size_t data_length,
                                    const uint8_t* data,
                                    size_t size)
      : U2fBleFrameFragment(data, size),
        command_(command),
        data_length_(data_length) {}

  U2fBleFrameInitializationFragment(const U2fBleFrameInitializationFragment&);
  U2fBleFrameInitializationFragment& operator=(
      const U2fBleFrameInitializationFragment&);

  U2fBleFrame::Command command() const { return command_; }
  uint16_t data_length() const { return data_length_; }

  std::vector<uint8_t> Serialize() const;  // FIXME

 private:
  U2fBleFrame::Command command_;
  uint16_t data_length_;
};

class U2fBleFrameContinuationFragment : public U2fBleFrameFragment {
 public:
  static constexpr size_t kHeaderSize = 1;

  static U2fBleFrameContinuationFragment Parse(const uint8_t* data,
                                               size_t size);

  U2fBleFrameContinuationFragment(const uint8_t* data,
                                  size_t size,
                                  uint8_t sequence)
      : U2fBleFrameFragment(data, size), sequence_(sequence) {}

  uint8_t sequence() const { return sequence_; }

  std::vector<uint8_t> Serialize() const;  // FIXME

 private:
  uint8_t sequence_;
};

class U2fBleFrameAssembler {
 public:
  U2fBleFrameAssembler(const U2fBleFrameInitializationFragment& fragment);
  ~U2fBleFrameAssembler();

  bool IsDone() const;

  bool AddFragment(const U2fBleFrameContinuationFragment& fragment);
  U2fBleFrame* GetFrame();

 private:
  uint8_t sequence_number_ = 0;
  U2fBleFrame frame_;
};

}  // namespace device

#endif  // DEVICE_U2F_U2F_BLE_FRAMES_
