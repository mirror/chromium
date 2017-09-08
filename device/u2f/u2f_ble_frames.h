// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_U2F_BLE_FRAMES_
#define DEVICE_U2F_U2F_BLE_FRAMES_

#include "base/macros.h"

#include <stdint.h>
#include <vector>

namespace device {

class U2fBleFrameInitializationFragment;
class U2fBleFrameContinuationFragment;

// Encapsulates a frame, i.e., a single request to or response from a U2F
// authenticator, designed to be transported via BLE. The frame is further split
// into fragments (see U2fBleFrameFragment class).
//
// The specification of what constitues a frame can be found here:
// https://fidoalliance.org/specs/fido-u2f-v1.2-ps-20170411/fido-u2f-bt-protocol-v1.2-ps-20170411.html#h2_framing
class U2fBleFrame {
 public:
  // PING and MSG commands are used with requests to U2F authenticators and the
  // corresponding responses. U2F raw messages are packed as MSG commands,
  // whereas PING is only aimed at checking the device's availability and can
  // encapsulate arbitrary data.
  // KEEPALIVE and ERROR commands are used in responses only.
  enum class Command : uint8_t {
    UNDEFINED = 0x00,
    PING = 0x81,
    KEEPALIVE = 0x82,
    MSG = 0x83,
    ERROR = 0xBF,
  };

  // The values which can be carried in the |data| section of a KEEPALIVE
  // message sent from an authenticator.
  enum class KeepaliveCode : uint8_t {
    // The request is still being processed. The authenticator will be sending
    // this message every |kKeepAliveMillis| milliseconds until completion.
    PROCESSING = 0x01,
    // The authenticator is waiting for the Test of User Presence to complete.
    TUP_NEEDED = 0x02,
  };

  // The types of errors an authenticator can return to the client. Carried in
  // the |data| section of an ERROR command.
  enum class ErrorCode : uint8_t {
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

  U2fBleFrame();
  U2fBleFrame(Command command, std::vector<uint8_t> data);

  U2fBleFrame(U2fBleFrame&&);
  U2fBleFrame& operator=(U2fBleFrame&&);

  ~U2fBleFrame();

  Command command() const { return command_; }

  bool IsValid() const;
  KeepaliveCode GetKeepaliveCode() const;
  ErrorCode GetErrorCode() const;

  const std::vector<uint8_t>& data() const { return data_; }
  std::vector<uint8_t>& data() { return data_; }

  // Splits the frame into fragments suitable for sending over BLE. Returns the
  // first fragment via |initial_fragment|, and pushes the remaining ones back
  // to the |other_fragments| vector.
  //
  // The |max_fragment_size| parameter ought to be at least 3. The resulting
  // fragments' binary sizes will not exceed this value.
  void ToFragments(
      size_t max_fragment_size,
      U2fBleFrameInitializationFragment* initial_fragment,
      std::vector<U2fBleFrameContinuationFragment>* other_fragments) const;

 private:
  Command command_ = Command::UNDEFINED;
  std::vector<uint8_t> data_;

  DISALLOW_COPY_AND_ASSIGN(U2fBleFrame);
};

// A single frame sent over BLE may be split over multiple writes and
// notifications because the technology was not designed for large messages.
// This class represents a single fragment. Not to be used directly.
//
// A frame is divided into an initialization fragment and zero, one or more
// continuation fragments. See the below section of the spec for the details:
// https://fidoalliance.org/specs/fido-u2f-v1.2-ps-20170411/fido-u2f-bt-protocol-v1.2-ps-20170411.html#h2_framing-fragmentation
//
// Note: This class and its subclasses don't own the |data|.
class U2fBleFrameFragment {
 public:
  const uint8_t* data() const { return data_; }
  size_t size() const { return size_; }

 private:
  friend class U2fBleFrameInitializationFragment;
  friend class U2fBleFrameContinuationFragment;

  U2fBleFrameFragment(const uint8_t* data = nullptr, size_t size = 0)
      : data_(data), size_(size) {}

  const uint8_t* data_;
  size_t size_;
};

// An initialization fragment of a frame.
class U2fBleFrameInitializationFragment : public U2fBleFrameFragment {
 public:
  static bool Parse(const std::vector<uint8_t>& data,
                    U2fBleFrameInitializationFragment* fragment);

  U2fBleFrameInitializationFragment() = default;
  U2fBleFrameInitializationFragment(U2fBleFrame::Command command,
                                    size_t data_length,
                                    const uint8_t* fragment_data,
                                    size_t fragment_size)
      : U2fBleFrameFragment(fragment_data, fragment_size),
        command_(command),
        data_length_(data_length) {}

  U2fBleFrame::Command command() const { return command_; }
  uint16_t data_length() const { return data_length_; }

  size_t Serialize(std::vector<uint8_t>* buffer) const;

 private:
  U2fBleFrame::Command command_ = U2fBleFrame::Command::UNDEFINED;
  uint16_t data_length_ = 0;
};

// A continuation fragment of a frame.
class U2fBleFrameContinuationFragment : public U2fBleFrameFragment {
 public:
  static bool Parse(const std::vector<uint8_t>& data,
                    U2fBleFrameContinuationFragment* fragment);

  U2fBleFrameContinuationFragment() = default;
  U2fBleFrameContinuationFragment(const uint8_t* data,
                                  size_t size,
                                  uint8_t sequence)
      : U2fBleFrameFragment(data, size), sequence_(sequence) {}

  uint8_t sequence() const { return sequence_; }

  size_t Serialize(std::vector<uint8_t>* buffer) const;

 private:
  uint8_t sequence_ = 0;
};

// The helper used to construct a U2fBleFrame from a sequence of its fragments.
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

  DISALLOW_COPY_AND_ASSIGN(U2fBleFrameAssembler);
};

}  // namespace device

#endif  // DEVICE_U2F_U2F_BLE_FRAMES_
