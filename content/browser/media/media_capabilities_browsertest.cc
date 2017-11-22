// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "media/base/media_switches.h"
#include "media/base/test_data_util.h"

namespace {

const char kDecodeTestFile[] = "decode_capabilities_test.html";
const char kSupported[] = "SUPPORTED";
const char kUnsupported[] = "UNSUPPORTED";
const char kError[] = "ERROR";
const char kFile[] = "file";
const char kMediaSource[] = "media-source";

#if BUILDFLAG(USE_PROPRIETARY_CODECS)
const char* kPropSupported = kSupported;
#else
const char* kPropSupported = kUnsupported;
#endif  // USE_PROPRIETARY_CODECS

enum StreamType { AUDIO, VIDEO };

enum ConfigType { FILE, MEDIA_SOURCE };

}  // namespace

namespace content {

class MediaCapabilitiesTest : public ContentBrowserTest {
 public:
  MediaCapabilitiesTest() = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(switches::kEnableBlinkFeatures,
                                    "MediaCapabilities");
  }

  std::string CanDecodeAudio(const std::string& config_type,
                             const std::string& content_type) {
    return CanDecode(config_type, content_type, StreamType::AUDIO);
  }

  std::string CanDecodeVideo(const std::string& config_type,
                             const std::string& content_type) {
    return CanDecode(config_type, content_type, StreamType::VIDEO);
  }

  std::string CanDecode(const std::string& config_type,
                        const std::string& content_type,
                        StreamType stream_type) {
    std::string command;
    if (stream_type == StreamType::AUDIO) {
      command.append("testAudioConfig(");
    } else {
      command.append("testVideoConfig(");
    }

    command.append("\"");
    command.append(config_type);
    command.append("\"");
    command.append(", ");
    command.append(content_type);
    command.append(");");

    EXPECT_TRUE(ExecuteScript(shell(), command));

    TitleWatcher title_watcher(shell()->web_contents(),
                               base::ASCIIToUTF16(kSupported));
    title_watcher.AlsoWaitForTitle(base::ASCIIToUTF16(kUnsupported));
    title_watcher.AlsoWaitForTitle(base::ASCIIToUTF16(kError));
    return base::UTF16ToASCII(title_watcher.WaitAndGetTitle());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MediaCapabilitiesTest);
};

// Adds param for query type (file vs media-source) to
class MediaCapabilitiesTestWithConfigType
    : public MediaCapabilitiesTest,
      public testing::WithParamInterface<ConfigType> {
 public:
  std::string GetTypeString() const {
    switch (GetParam()) {
      case ConfigType::FILE:
        return kFile;
      case ConfigType::MEDIA_SOURCE:
        return kMediaSource;
      default:
        NOTREACHED();
        return "";
    }
  }
};

// Cover basic codec support of content types where the answer of support
// (or not) should be common to both "media-source" and "file" query types.
// for more exhaustive codec string testing.
IN_PROC_BROWSER_TEST_P(MediaCapabilitiesTestWithConfigType,
                       CommonVideoDecodeTypes) {
  base::FilePath file_path = media::GetTestDataFilePath(kDecodeTestFile);

  const std::string& config_type = GetTypeString();

  EXPECT_TRUE(
      NavigateToURL(shell(), content::GetFileUrlWithQuery(file_path, "")));

  EXPECT_EQ(kSupported,
            CanDecodeVideo(config_type, "'video/webm; codecs=\"vp8\"'"));

  // Only support the new vp09 format which provides critical profile
  // information.
  EXPECT_EQ(kUnsupported,
            CanDecodeVideo(config_type, "'video/webm; codecs=\"vp9\"'"));
  // Requires command line flag switches::kEnableNewVp9CodecString
  EXPECT_EQ(
      kSupported,
      CanDecodeVideo(config_type, "'video/webm; codecs=\"vp09.00.10.08\"'"));

  // Supported when built with USE_PROPRIETARY_CODECS
  EXPECT_EQ(kPropSupported,
            CanDecodeVideo(config_type, "'video/mp4; codecs=\"avc1.42E01E\"'"));
  EXPECT_EQ(kPropSupported,
            CanDecodeVideo(config_type, "'video/mp4; codecs=\"avc1.42101E\"'"));
  EXPECT_EQ(kPropSupported,
            CanDecodeVideo(config_type, "'video/mp4; codecs=\"avc1.42701E\"'"));
  EXPECT_EQ(kPropSupported,
            CanDecodeVideo(config_type, "'video/mp4; codecs=\"avc1.42F01E\"'"));
  EXPECT_EQ(
      kPropSupported,
      CanDecodeVideo(config_type, "'video/mp4; codecs=\"vp09.00.10.08\"'"));

  // Test a handful of invalid strings.
  EXPECT_EQ(kUnsupported,
            CanDecodeVideo(config_type, "'video/webm; codecs=\"theora\"'"));
  EXPECT_EQ(
      kUnsupported,
      CanDecodeVideo(config_type, "'video/webm; codecs=\"avc1.42E01E\"'"));
  // Only new vp09 format is supported with MP4.
  EXPECT_EQ(kUnsupported,
            CanDecodeVideo(config_type, "'video/mp4; codecs=\"vp9\"'"));
}

// Cover basic codec support. See media_canplaytype_browsertest.cc for more
// exhaustive codec string testing.
IN_PROC_BROWSER_TEST_P(MediaCapabilitiesTestWithConfigType,
                       CommonAudioDecodeTypes) {
  base::FilePath file_path = media::GetTestDataFilePath(kDecodeTestFile);

  const std::string& config_type = GetTypeString();

  EXPECT_TRUE(
      NavigateToURL(shell(), content::GetFileUrlWithQuery(file_path, "")));

  EXPECT_EQ(kSupported,
            CanDecodeAudio(config_type, "'audio/ogg; codecs=\"flac\"'"));
  EXPECT_EQ(kSupported,
            CanDecodeAudio(config_type, "'audio/ogg; codecs=\"vorbis\"'"));
  EXPECT_EQ(kSupported,
            CanDecodeAudio(config_type, "'audio/ogg; codecs=\"opus\"'"));

  EXPECT_EQ(kSupported,
            CanDecodeAudio(config_type, "'audio/webm; codecs=\"opus\"'"));
  EXPECT_EQ(kSupported,
            CanDecodeAudio(config_type, "'audio/webm; codecs=\"vorbis\"'"));

  EXPECT_EQ(kSupported, CanDecodeAudio(config_type, "'audio/flac'"));

  // Supported when built with USE_PROPRIETARY_CODECS
  EXPECT_EQ(kPropSupported,
            CanDecodeAudio(config_type, "'audio/mpeg; codecs=\"mp4a.69\"'"));
  EXPECT_EQ(kPropSupported,
            CanDecodeAudio(config_type, "'audio/mp4; codecs=\"mp4a.40.02\"'"));
  EXPECT_EQ(kPropSupported, CanDecodeAudio(config_type, "'audio/aac'"));

  // TODO(chcunningham): Differentiate config_type vs kMediaSource support for
  // FLAC-in-MP4, making MSE support for this contingent upon at least
  // base::FeatureList::IsEnabled(kMseFlacInIsobmff) and kPropSupported.
  EXPECT_EQ(kPropSupported,
            CanDecodeAudio(config_type, "'audio/mp4; codecs=\"flac\"'"));

  // Test a handful of invalid strings.
  EXPECT_EQ(kUnsupported,
            CanDecodeAudio(config_type, "'audio/wav; codecs=\"mp3\"'"));
  EXPECT_EQ(kUnsupported,
            CanDecodeAudio(config_type, "'audio/webm; codecs=\"vp8\"'"));
}

INSTANTIATE_TEST_CASE_P(,
                        MediaCapabilitiesTestWithConfigType,
                        ::testing::Values(ConfigType::FILE,
                                          ConfigType::MEDIA_SOURCE));

}  // namespace content
