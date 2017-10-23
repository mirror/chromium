// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CODEC_WEBRTC_VIDEO_ENCODER_SELECTOR_H_
#define REMOTING_CODEC_WEBRTC_VIDEO_ENCODER_SELECTOR_H_

#include <map>
#include <memory>
#include <vector>

#include "base/callback.h"
#include "remoting/codec/webrtc_video_encoder.h"
#include "third_party/webrtc/common_types.h"
#include "ui/gfx/geometry/size.h"

namespace webrtc {
class DesktopFrame;
}  // namespace webrtc

namespace remoting {

// A container of creators of various video encoder implementations. It selects
// qualified video encoders one by one according to the codec from SDP and the
// target frame.
//
// Clients expect to use this class in the following way:
// 1. RegisterEncoder
// 2. SetPreferredCodec
// 3. SetDesktopFrame
// 4. CreateEncoder: Selector will return the preferred codec if it supports the
//    |profile_|.
// 5. CreateEncoder
// 6. CreateEncoder
// ...
// ?. CreateEncoder: Selector will return other codecs which support |profile_|
//    one-by-one. If there is not more codec, CreateEncoder() returns nullptr.
// Goes back to 3 for next frame.
class WebrtcVideoEncoderSelector final {
 public:
  struct Profile {
    gfx::Size resolution;
    int frame_rate;  // Always > 0
  };

  using IsProfileSupportedFunction = base::Callback<bool(const Profile&)>;
  using CreateEncoderFunction =
      base::Callback<std::unique_ptr<WebrtcVideoEncoder>()>;

  WebrtcVideoEncoderSelector();
  ~WebrtcVideoEncoderSelector();

  // Creates a WebrtcVideoEncoder according to the |profile_|.
  std::unique_ptr<WebrtcVideoEncoder> CreateEncoder();

  // Extracts information from |frame| and sets |profile_|. If the information
  // extracted from |frame| is not consistent with the values in |profile_|,
  // this function also resets |last_codec_|, so next CreateEncoder() function
  // call returns the first codec which supports the |frame|.
  void SetDesktopFrame(const webrtc::DesktopFrame& frame);

  // Sets the index of the preferred codec. The |codec| is the returned index
  // from RegisterEncoder() function. This function asserts |codec| >= 0 and
  // < encoders_.size().
  void SetPreferredCodec(int codec);

  // Registers an encoder to the selector. Returns the index of the encoder.
  // Clients can use the index returned by this function to set the preferred
  // codec. This class does not check the return value of |creator|, it may
  // return |nullptr| even if |is_supported| returns true.
  int RegisterEncoder(IsProfileSupportedFunction is_supported,
                      CreateEncoderFunction creator);

 private:
  std::vector<std::pair<IsProfileSupportedFunction, CreateEncoderFunction>>
      encoders_;
  Profile profile_;
  int preferred_codec_ = -1;
  int last_codec_ = -1;
};

}  // namespace remoting

#endif  // REMOTING_CODEC_WEBRTC_VIDEO_ENCODER_SELECTOR_H_
