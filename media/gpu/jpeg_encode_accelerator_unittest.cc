// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This has to be included first.
// See http://code.google.com/p/googletest/issues/detail?id=371
#include "testing/gtest/include/gtest/gtest.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <memory>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "media/base/test_data_util.h"
#include "media/filters/jpeg_parser.h"
#include "media/gpu/features.h"
#include "media/gpu/vaapi_jpeg_encode_accelerator.h"
#include "media/gpu/video_accelerator_unittest_helpers.h"
#include "media/video/jpeg_encode_accelerator.h"
#include "third_party/libyuv/include/libyuv.h"
#include "ui/gfx/codec/jpeg_codec.h"

#if BUILDFLAG(USE_VAAPI)
#include "media/gpu/vaapi_wrapper.h"
#endif

namespace media {
namespace {

// Default test image file.
const base::FilePath::CharType* kDefaultYuvFilename =
    FILE_PATH_LITERAL("bali_640x360_P420.yuv");
// Decide to save encode results to files or not. Output files will be saved
// in the same directory with unittest. File name is like input file but
// changing the extension to "jpg".
bool g_save_to_file = false;

// Environment to create test data for all test cases.
class JpegEncodeAcceleratorTestEnvironment;
JpegEncodeAcceleratorTestEnvironment* g_env;

struct TestImageFile {
  explicit TestImageFile(const base::FilePath::StringType& filename)
      : filename(filename) {}

  base::FilePath::StringType filename;

  // The input content of |filename|.
  std::string data_str;

  gfx::Size visible_size;
  size_t output_size;
};

enum ClientState {
  CS_CREATED,
  CS_INITIALIZED,
  CS_ENCODE_PASS,
  CS_ERROR,
};

class JpegClient : public JpegEncodeAccelerator::Client {
 public:
  JpegClient(const std::vector<TestImageFile*>& test_image_files,
             ClientStateNotification<ClientState>* note);
  ~JpegClient() override;
  void CreateJpegEncoder();
  void DestroyJpegEncoder();
  void StartEncode(int32_t bitstream_buffer_id);

  // JpegEncodeAccelerator::Client implementation.
  void VideoFrameReady(int video_frame_id) override;
  void NotifyError(int video_frame_id,
                   JpegEncodeAccelerator::Error error) override;

 private:
  void PrepareMemory(int32_t bitstream_buffer_id);
  void SetState(ClientState new_state);
  void SaveToFile(int32_t bitstream_buffer_id);
  //bool GetSoftwareEncodeResult(int32_t bitstream_buffer_id);

  // Calculate mean absolute difference of hardware and software encode results
  // to check the similarity.
  //double GetMeanAbsoluteDifference(int32_t bitstream_buffer_id);

  // JpegClient doesn't own |test_image_files_|.
  const std::vector<TestImageFile*>& test_image_files_;

  std::unique_ptr<JpegEncodeAccelerator> encoder_;
  ClientState state_;

  // Used to notify another thread about the state. JpegClient does not own
  // this.
  ClientStateNotification<ClientState>* note_;

  std::map<int, TestImageFile*> video_frame_id_to_image_;

  // Mapped memory of input file.
  std::unique_ptr<base::SharedMemory> in_shm_;
  // Mapped memory of output buffer from hardware encoder.
  std::unique_ptr<base::SharedMemory> hw_out_shm_;
  // Mapped memory of output buffer from software encoder.
  //std::unique_ptr<base::SharedMemory> sw_out_shm_;

  DISALLOW_COPY_AND_ASSIGN(JpegClient);
};

JpegClient::JpegClient(const std::vector<TestImageFile*>& test_image_files,
                       ClientStateNotification<ClientState>* note)
    : test_image_files_(test_image_files), state_(CS_CREATED), note_(note),
    video_frame_id_to_image_() {}

JpegClient::~JpegClient() {}

void JpegClient::CreateJpegEncoder() {
  encoder_ = nullptr;

#if BUILDFLAG(USE_VAAPI)
  encoder_ = base::MakeUnique<VaapiJpegEncodeAccelerator>(
      base::ThreadTaskRunnerHandle::Get());
#endif

  if (!encoder_) {
    LOG(ERROR) << "Failed to create JpegEncodeAccelerator.";
    SetState(CS_ERROR);
    return;
  }

  if (!encoder_->Initialize(this)) {
    LOG(ERROR) << "JpegEncodeAccelerator::Initialize() failed";
    SetState(CS_ERROR);
    return;
  }
  SetState(CS_INITIALIZED);
}

void JpegClient::DestroyJpegEncoder() {
  encoder_.reset();
}

void JpegClient::VideoFrameReady(int video_frame_id) {
  /*
  if (!GetSoftwareDecodeResult(bitstream_buffer_id)) {
    SetState(CS_ERROR);
    return;
  }
*/

  if (g_save_to_file) {
    SaveToFile(video_frame_id);
  }
  SetState(CS_ENCODE_PASS);

/*
  double difference = GetMeanAbsoluteDifference(bitstream_buffer_id);
  if (difference <= kDecodeSimilarityThreshold) {
    SetState(CS_DECODE_PASS);
  } else {
    LOG(ERROR) << "The mean absolute difference between software and hardware "
               << "decode is " << difference;
    SetState(CS_ERROR);
  }
  */
}

void JpegClient::NotifyError(int video_frame_id,
                             JpegEncodeAccelerator::Error error) {
  LOG(ERROR) << "Notifying of error " << error << " for video frame id "
             << video_frame_id;
  SetState(CS_ERROR);
}

void JpegClient::PrepareMemory(int32_t bitstream_buffer_id) {
  TestImageFile* image_file = test_image_files_[bitstream_buffer_id];

  size_t input_size = image_file->data_str.size();
  if (!in_shm_.get() || input_size > in_shm_->mapped_size()) {
    in_shm_.reset(new base::SharedMemory);
    LOG_ASSERT(in_shm_->CreateAndMapAnonymous(input_size));
  }
  memcpy(in_shm_->memory(), image_file->data_str.data(), input_size);

  if (!hw_out_shm_.get() ||
      image_file->output_size > hw_out_shm_->mapped_size()) {
    hw_out_shm_.reset(new base::SharedMemory);
    LOG_ASSERT(hw_out_shm_->CreateAndMapAnonymous(image_file->output_size));
  }
  memset(hw_out_shm_->memory(), 0, image_file->output_size);

/*
  if (!sw_out_shm_.get() ||
      image_file->output_size > sw_out_shm_->mapped_size()) {
    sw_out_shm_.reset(new base::SharedMemory);
    LOG_ASSERT(sw_out_shm_->CreateAndMapAnonymous(image_file->output_size));
  }
  memset(sw_out_shm_->memory(), 0, image_file->output_size);
  */
}

void JpegClient::SetState(ClientState new_state) {
  DVLOG(2) << "Changing state " << state_ << "->" << new_state;
  note_->Notify(new_state);
  state_ = new_state;
}

void JpegClient::SaveToFile(int video_frame_id) {
  TestImageFile* image_file = video_frame_id_to_image_[video_frame_id];
  DCHECK_NE(nullptr, image_file);

  base::FilePath in_filename(image_file->filename);
  base::FilePath out_filename = in_filename.ReplaceExtension(".jpg");
  int size = base::checked_cast<int>(image_file->output_size);
  ASSERT_EQ(size,
            base::WriteFile(out_filename,
                            static_cast<char*>(hw_out_shm_->memory()), size));
}

void JpegClient::StartEncode(int32_t bitstream_buffer_id) {
  DCHECK_LT(static_cast<size_t>(bitstream_buffer_id), test_image_files_.size());
  TestImageFile* image_file = test_image_files_[bitstream_buffer_id];

  image_file->output_size = encoder_->GetMaxCodedBufferSize(
      image_file->visible_size.width(),
      image_file->visible_size.height());
  PrepareMemory(bitstream_buffer_id);

  base::SharedMemoryHandle dup_handle;
  dup_handle = base::SharedMemory::DuplicateHandle(hw_out_shm_->handle());
  BitstreamBuffer bitstream_buffer(bitstream_buffer_id, dup_handle,
                                   image_file->output_size);
  scoped_refptr<VideoFrame> input_frame_ = VideoFrame::WrapExternalSharedMemory(
      PIXEL_FORMAT_I420, image_file->visible_size,
      gfx::Rect(image_file->visible_size), image_file->visible_size,
      static_cast<uint8_t*>(in_shm_->memory()), image_file->data_str.size(),
      in_shm_->handle(), 0, base::TimeDelta());

  LOG_ASSERT(input_frame_.get());
  video_frame_id_to_image_[input_frame_->unique_id()] = image_file;

  encoder_->Encode(input_frame_, bitstream_buffer);
}

class JpegEncodeAcceleratorTestEnvironment : public ::testing::Environment {
 public:
  JpegEncodeAcceleratorTestEnvironment(
      const base::FilePath::CharType* yuv_filenames) {
    user_yuv_filenames_ =
        yuv_filenames ? yuv_filenames : kDefaultYuvFilename;
  }
  void SetUp() override;
  void TearDown() override;

  // Create all black test image with |width| and |height| size.
  //bool CreateTestYuvImage(int width, int height, base::FilePath* filename);

  // Read image from |filename| to |image_data|.
  void ReadTestYuvImage(base::FilePath& filename, TestImageFile* image_data);

  // Returns a file path for a file in what name specified or media/test/data
  // directory.  If the original file path is existed, returns it first.
  base::FilePath GetOriginalOrTestDataFilePath(const std::string& name);

  // Parsed data of |test_640x360_yuv_file_|.
  //std::unique_ptr<TestImageFile> image_data_640x360_black_;

  // Parsed data of failure image.
  //std::unique_ptr<TestImageFile> image_data_invalid_;

  // Parsed data from command line.
  std::vector<std::unique_ptr<TestImageFile>> image_data_user_;

 private:
  const base::FilePath::CharType* user_yuv_filenames_;

  // Used for testing some drivers which will align the output resolution to a
  // multiple of 16. 640x360 will be aligned to 640x368.
  //base::FilePath test_640x360_yuv_file_;
};

void JpegEncodeAcceleratorTestEnvironment::SetUp() {
  /*
  ASSERT_TRUE(CreateTestYuvImage(640, 360, &test_640x360_yuv_file_));

  image_data_640x360_black_.reset(
      new TestImageFile(test_640x360_yuv_file_.value()));
  ASSERT_NO_FATAL_FAILURE(ReadTestYuvImage(test_640x360_yuv_file_,
                                            image_data_640x360_black_.get()));

  base::FilePath default_yuv_file =
      GetOriginalOrTestDataFilePath(kDefaultJpegFilename);
  image_data_1280x720_default_.reset(new TestImageFile(kDefaultJpegFilename));
  ASSERT_NO_FATAL_FAILURE(
      ReadTestYuvImage(default_yuv_file, image_data_1280x720_default_.get()));


  image_data_invalid_.reset(new TestImageFile("failure.jpg"));
  image_data_invalid_->data_str.resize(100, 0);
  image_data_invalid_->visible_size.SetSize(1280, 720);
  image_data_invalid_->output_size = VideoFrame::AllocationSize(
      PIXEL_FORMAT_I420, image_data_invalid_->visible_size);
*/

  // |user_yuv_filenames_| may include many files and use ';' as delimiter.
  std::vector<base::FilePath::StringType> filenames = base::SplitString(
      user_yuv_filenames_, base::FilePath::StringType(1, ';'),
      base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  for (const auto& filename : filenames) {
    base::FilePath input_file = GetOriginalOrTestDataFilePath(filename);
    auto image_data = base::MakeUnique<TestImageFile>(filename);
    ASSERT_NO_FATAL_FAILURE(ReadTestYuvImage(input_file, image_data.get()));
    image_data_user_.push_back(std::move(image_data));
  }
}

void JpegEncodeAcceleratorTestEnvironment::TearDown() {
  //base::DeleteFile(test_640x360_jpeg_file_, false);
}

void JpegEncodeAcceleratorTestEnvironment::ReadTestYuvImage(
    base::FilePath& input_file,
    TestImageFile* image_data) {
  ASSERT_TRUE(base::ReadFileToString(input_file, &image_data->data_str));

  // TODO(shenghao): read the size from command line.
  image_data->visible_size.SetSize(
      640,
      360);
  // This is just a placeholder. We will compute the real output size when we
  // have encoder instance.
  image_data->output_size =
      VideoFrame::AllocationSize(PIXEL_FORMAT_I420, image_data->visible_size);
}

base::FilePath
JpegEncodeAcceleratorTestEnvironment::GetOriginalOrTestDataFilePath(
    const std::string& name) {
  base::FilePath original_file_path = base::FilePath(name);
  base::FilePath return_file_path = GetTestDataFilePath(name);

  if (PathExists(original_file_path))
    return_file_path = original_file_path;

  VLOG(3) << "Use file path " << return_file_path.value();
  return return_file_path;
}

class JpegEncodeAcceleratorTest : public ::testing::Test {
 protected:
  JpegEncodeAcceleratorTest() {}

  void TestEncode(size_t num_concurrent_encoders);

  // The elements of |test_image_files_| are owned by
  // JpegEncodeAcceleratorTestEnvironment.
  std::vector<TestImageFile*> test_image_files_;
  std::vector<ClientState> expected_status_;

 protected:
  DISALLOW_COPY_AND_ASSIGN(JpegEncodeAcceleratorTest);
};

void JpegEncodeAcceleratorTest::TestEncode(size_t num_concurrent_encoders) {
  LOG_ASSERT(test_image_files_.size() >= expected_status_.size());
  base::Thread encoder_thread("EncoderThread");
  ASSERT_TRUE(encoder_thread.Start());

  std::vector<std::unique_ptr<ClientStateNotification<ClientState>>> notes;
  std::vector<std::unique_ptr<JpegClient>> clients;

  for (size_t i = 0; i < num_concurrent_encoders; i++) {
    notes.push_back(base::MakeUnique<ClientStateNotification<ClientState>>());
    clients.push_back(
        base::MakeUnique<JpegClient>(test_image_files_, notes.back().get()));
    encoder_thread.task_runner()->PostTask(
        FROM_HERE, base::Bind(&JpegClient::CreateJpegEncoder,
                              base::Unretained(clients.back().get())));
    ASSERT_EQ(notes[i]->Wait(), CS_INITIALIZED);
  }

  for (size_t index = 0; index < test_image_files_.size(); index++) {
    for (size_t i = 0; i < num_concurrent_encoders; i++) {
      encoder_thread.task_runner()->PostTask(
          FROM_HERE, base::Bind(&JpegClient::StartEncode,
                                base::Unretained(clients[i].get()), index));
    }
    if (index < expected_status_.size()) {
      for (size_t i = 0; i < num_concurrent_encoders; i++) {
        ASSERT_EQ(notes[i]->Wait(), expected_status_[index]);
      }
    }
  }

  for (size_t i = 0; i < num_concurrent_encoders; i++) {
    encoder_thread.task_runner()->PostTask(
        FROM_HERE, base::Bind(&JpegClient::DestroyJpegEncoder,
                              base::Unretained(clients[i].get())));
  }
  encoder_thread.Stop();
}

TEST_F(JpegEncodeAcceleratorTest, SimpleEncode) {
  for (auto& image : g_env->image_data_user_) {
    test_image_files_.push_back(image.get());
    expected_status_.push_back(CS_ENCODE_PASS);
  }
  TestEncode(1);
}


} // namespace
} // namespace media

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  base::CommandLine::Init(argc, argv);
  base::ShadowingAtExitManager at_exit_manager;

  // Needed to enable DVLOG through --vmodule.
  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  LOG_ASSERT(logging::InitLogging(settings));

  const base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  DCHECK(cmd_line);

  const base::FilePath::CharType* yuv_filenames = nullptr;
  base::CommandLine::SwitchMap switches = cmd_line->GetSwitches();
  for (base::CommandLine::SwitchMap::const_iterator it = switches.begin();
       it != switches.end(); ++it) {
    // yuv_filenames can include one or many files and use ';' as delimiter.
    if (it->first == "yuv_filenames") {
      yuv_filenames = it->second.c_str();
      continue;
    }
    if (it->first == "save_to_file") {
      media::g_save_to_file = true;
      continue;
    }
    if (it->first == "v" || it->first == "vmodule")
      continue;
    if (it->first == "h" || it->first == "help")
      continue;
    LOG(ERROR) << "Unexpected switch: " << it->first << ":" << it->second;
    return -EINVAL;
  }
#if BUILDFLAG(USE_VAAPI)
  media::VaapiWrapper::PreSandboxInitialization();
#endif

  media::g_env = reinterpret_cast<media::JpegEncodeAcceleratorTestEnvironment*>(
      testing::AddGlobalTestEnvironment(
          new media::JpegEncodeAcceleratorTestEnvironment(yuv_filenames)));

  return RUN_ALL_TESTS();
}
