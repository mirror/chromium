// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <utility>
#include <vector>

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/test_file_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "components/download/public/common/download_interrupt_reasons.h"
#include "content/browser/byte_stream.h"
#include "content/browser/download/download_create_info.h"
#include "content/browser/download/download_destination_observer.h"
#include "content/browser/download/download_file_impl.h"
#include "content/browser/download/download_request_handle.h"
#include "content/public/browser/download_manager.h"
#include "content/public/test/mock_download_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/file_stream.h"
#include "net/base/mock_file_stream.h"
#include "net/base/net_errors.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::DoAll;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::Sequence;
using ::testing::SetArgPointee;
using ::testing::StrictMock;

namespace content {
namespace {

// Struct for SourceStream states verification.
struct SourceStreamTestData {
  SourceStreamTestData(int64_t offset, int64_t bytes_written, bool finished)
      : offset(offset), bytes_written(bytes_written), finished(finished) {}
  int64_t offset;
  int64_t bytes_written;
  bool finished;
};

int64_t GetBuffersLength(const char** buffers, size_t num_buffer) {
  int64_t result = 0;
  for (size_t i = 0; i < num_buffer; ++i)
    result += static_cast<int64_t>(strlen(buffers[i]));
  return result;
}

std::string GetHexEncodedHashValue(crypto::SecureHash* hash_state) {
  if (!hash_state)
    return std::string();
  std::vector<char> hash_value(hash_state->GetHashLength());
  hash_state->Finish(&hash_value.front(), hash_value.size());
  return base::HexEncode(&hash_value.front(), hash_value.size());
}

class MockByteStreamReader : public ByteStreamReader {
 public:
  MockByteStreamReader() {}
  ~MockByteStreamReader() {}

  // ByteStream functions
  MOCK_METHOD2(Read, ByteStreamReader::StreamState(
      scoped_refptr<net::IOBuffer>*, size_t*));
  MOCK_CONST_METHOD0(GetStatus, int());
  MOCK_METHOD1(RegisterCallback, void(const base::Closure&));
};

class MockDownloadDestinationObserver : public DownloadDestinationObserver {
 public:
  MOCK_METHOD3(DestinationUpdate, void(
      int64_t, int64_t, const std::vector<DownloadItem::ReceivedSlice>&));
  void DestinationError(
      download::DownloadInterruptReason reason,
      int64_t bytes_so_far,
      std::unique_ptr<crypto::SecureHash> hash_state) override {
    MockDestinationError(
        reason, bytes_so_far, GetHexEncodedHashValue(hash_state.get()));
  }
  void DestinationCompleted(
      int64_t total_bytes,
      std::unique_ptr<crypto::SecureHash> hash_state) override {
    MockDestinationCompleted(total_bytes,
                             GetHexEncodedHashValue(hash_state.get()));
  }

  MOCK_METHOD3(MockDestinationError,
               void(download::DownloadInterruptReason,
                    int64_t,
                    const std::string&));
  MOCK_METHOD2(MockDestinationCompleted, void(int64_t, const std::string&));

  // Doesn't override any methods in the base class.  Used to make sure
  // that the last DestinationUpdate before a Destination{Completed,Error}
  // had the right values.
  MOCK_METHOD2(CurrentUpdateStatus, void(int64_t, int64_t));
};

MATCHER(IsNullCallback, "") { return (arg.is_null()); }

enum DownloadFileRenameMethodType { RENAME_AND_UNIQUIFY, RENAME_AND_ANNOTATE };

// This is a test DownloadFileImpl that has no retry delay and, on Posix,
// retries renames failed due to ACCESS_DENIED.
class TestDownloadFileImpl : public DownloadFileImpl {
 public:
  TestDownloadFileImpl(std::unique_ptr<download::DownloadSaveInfo> save_info,
                       const base::FilePath& default_downloads_directory,
                       std::unique_ptr<DownloadManager::InputStream> stream,
                       uint32_t download_id,
                       base::WeakPtr<DownloadDestinationObserver> observer)
      : DownloadFileImpl(std::move(save_info),
                         default_downloads_directory,
                         std::move(stream),
                         download_id,
                         observer) {}

 protected:
  base::TimeDelta GetRetryDelayForFailedRename(int attempt_count) override {
    return base::TimeDelta::FromMilliseconds(0);
  }

#if !defined(OS_WIN)
  // On Posix, we don't encounter transient errors during renames, except
  // possibly EAGAIN, which is difficult to replicate reliably. So we resort to
  // simulating a transient error using ACCESS_DENIED instead.
  bool ShouldRetryFailedRename(
      download::DownloadInterruptReason reason) override {
    return reason == download::DOWNLOAD_INTERRUPT_REASON_FILE_ACCESS_DENIED;
  }
#endif
};

}  // namespace

class DownloadFileTest : public testing::Test {
 public:
  static const char kTestData1[];
  static const char kTestData2[];
  static const char kTestData3[];
  static const char kTestData4[];
  static const char kTestData5[];
  static const char* kTestData6[];
  static const char* kTestData7[];
  static const char* kTestData8[];
  static const char kDataHash[];
  static const char kEmptyHash[];
  static const uint32_t kDummyDownloadId;
  static const int kDummyChildId;
  static const int kDummyRequestId;

  DownloadFileTest()
      : observer_(new StrictMock<MockDownloadDestinationObserver>),
        observer_factory_(observer_.get()),
        input_stream_(nullptr),
        additional_streams_(
            std::vector<StrictMock<MockByteStreamReader>*>{nullptr, nullptr}),
        bytes_(-1),
        bytes_per_sec_(-1) {}

  ~DownloadFileTest() override {}

  void SetUpdateDownloadInfo(
      int64_t bytes, int64_t bytes_per_sec,
      const std::vector<DownloadItem::ReceivedSlice>& received_slices) {
    bytes_ = bytes;
    bytes_per_sec_ = bytes_per_sec;
  }

  void ConfirmUpdateDownloadInfo() {
    observer_->CurrentUpdateStatus(bytes_, bytes_per_sec_);
  }

  void SetUp() override {
    EXPECT_CALL(*(observer_.get()), DestinationUpdate(_, _, _))
        .Times(AnyNumber())
        .WillRepeatedly(Invoke(this, &DownloadFileTest::SetUpdateDownloadInfo));
  }

  // Mock calls to this function are forwarded here.
  void RegisterCallback(const base::Closure& sink_callback) {
    sink_callback_ = sink_callback;
  }

  void SetInterruptReasonCallback(const base::Closure& closure,
                                  download::DownloadInterruptReason* reason_p,
                                  download::DownloadInterruptReason reason) {
    *reason_p = reason;
    closure.Run();
  }

  bool CreateDownloadFile(int offset, bool calculate_hash) {
    return CreateDownloadFile(offset, 0, calculate_hash,
                              DownloadItem::ReceivedSlices());
  }

  bool CreateDownloadFile(int offset,
                          int length,
                          bool calculate_hash,
                          const DownloadItem::ReceivedSlices& received_slices) {
    // There can be only one.
    DCHECK(!download_file_.get());

    input_stream_ = new StrictMock<MockByteStreamReader>();

    // TODO: Need to actually create a function that'll set the variables
    // based on the inputs from the callback.
    EXPECT_CALL(*input_stream_, RegisterCallback(_))
        .WillOnce(Invoke(this, &DownloadFileTest::RegisterCallback))
        .RetiresOnSaturation();

    std::unique_ptr<download::DownloadSaveInfo> save_info(
        new download::DownloadSaveInfo());
    save_info->offset = offset;
    save_info->length = length;

    download_file_.reset(new TestDownloadFileImpl(
        std::move(save_info), base::FilePath(),
        std::make_unique<DownloadManager::InputStream>(
            std::unique_ptr<ByteStreamReader>(input_stream_)),
        DownloadItem::kInvalidId, observer_factory_.GetWeakPtr()));

    EXPECT_CALL(*input_stream_, Read(_, _))
        .WillOnce(Return(ByteStreamReader::STREAM_EMPTY))
        .RetiresOnSaturation();

    base::WeakPtrFactory<DownloadFileTest> weak_ptr_factory(this);
    download::DownloadInterruptReason result =
        download::DOWNLOAD_INTERRUPT_REASON_NONE;
    base::RunLoop loop_runner;
    download_file_->Initialize(
        base::Bind(&DownloadFileTest::SetInterruptReasonCallback,
                   weak_ptr_factory.GetWeakPtr(), loop_runner.QuitClosure(),
                   &result),
        DownloadFile::CancelRequestCallback(), received_slices, true);
    loop_runner.Run();

    ::testing::Mock::VerifyAndClearExpectations(input_stream_);
    return result == download::DOWNLOAD_INTERRUPT_REASON_NONE;
  }

  void DestroyDownloadFile(int offset, bool compare_disk_data = true) {
    EXPECT_FALSE(download_file_->InProgress());

    // Make sure the data has been properly written to disk.
    if (compare_disk_data) {
      std::string disk_data;
      EXPECT_TRUE(
          base::ReadFileToString(download_file_->FullPath(), &disk_data));
      EXPECT_EQ(expected_data_, disk_data);
    }

    // Make sure the Browser and File threads outlive the DownloadFile
    // to satisfy thread checks inside it.
    download_file_.reset();
  }

  // Setup the stream to append data or write from |offset| to the file.
  // Don't actually trigger the callback or do verifications.
  void SetupDataAppend(const char** data_chunks,
                       size_t num_chunks,
                       MockByteStreamReader* stream_reader,
                       ::testing::Sequence s,
                       int64_t offset = -1) {
    DCHECK(stream_reader);
    size_t current_pos = static_cast<size_t>(offset);
    for (size_t i = 0; i < num_chunks; i++) {
      const char *source_data = data_chunks[i];
      size_t length = strlen(source_data);
      scoped_refptr<net::IOBuffer> data = new net::IOBuffer(length);
      memcpy(data->data(), source_data, length);
      EXPECT_CALL(*stream_reader, Read(_, _))
          .InSequence(s)
          .WillOnce(DoAll(SetArgPointee<0>(data), SetArgPointee<1>(length),
                          Return(ByteStreamReader::STREAM_HAS_DATA)))
          .RetiresOnSaturation();

      if (offset < 0) {
        // Append data.
        expected_data_ += source_data;
        continue;
      }

      // Write from offset. May fill holes with '\0'.
      size_t new_len = current_pos + length;
      if (new_len > expected_data_.size())
        expected_data_.append(new_len - expected_data_.size(), '\0');
      expected_data_.replace(current_pos, length, source_data);
      current_pos += length;
    }
  }

  void VerifyStreamAndSize() {
    ::testing::Mock::VerifyAndClearExpectations(input_stream_);
    int64_t size;
    EXPECT_TRUE(base::GetFileSize(download_file_->FullPath(), &size));
    EXPECT_EQ(expected_data_.size(), static_cast<size_t>(size));
  }

  // TODO(rdsmith): Manage full percentage issues properly.
  void AppendDataToFile(const char **data_chunks, size_t num_chunks) {
    ::testing::Sequence s1;
    SetupDataAppend(data_chunks, num_chunks, input_stream_, s1);
    EXPECT_CALL(*input_stream_, Read(_, _))
        .InSequence(s1)
        .WillOnce(Return(ByteStreamReader::STREAM_EMPTY))
        .RetiresOnSaturation();
    sink_callback_.Run();
    VerifyStreamAndSize();
  }

  void SetupFinishStream(download::DownloadInterruptReason interrupt_reason,
                         MockByteStreamReader* stream_reader,
                         ::testing::Sequence s) {
    EXPECT_CALL(*stream_reader, Read(_, _))
        .InSequence(s)
        .WillOnce(Return(ByteStreamReader::STREAM_COMPLETE))
        .RetiresOnSaturation();
    EXPECT_CALL(*stream_reader, GetStatus())
        .InSequence(s)
        .WillOnce(Return(interrupt_reason))
        .RetiresOnSaturation();
    EXPECT_CALL(*stream_reader, RegisterCallback(_)).RetiresOnSaturation();
  }

  void FinishStream(download::DownloadInterruptReason interrupt_reason,
                    bool check_observer,
                    const std::string& expected_hash) {
    ::testing::Sequence s1;
    SetupFinishStream(interrupt_reason, input_stream_, s1);
    sink_callback_.Run();
    VerifyStreamAndSize();
    if (check_observer) {
      EXPECT_CALL(*(observer_.get()),
                  MockDestinationCompleted(_, expected_hash));
      base::RunLoop().RunUntilIdle();
      ::testing::Mock::VerifyAndClearExpectations(observer_.get());
      EXPECT_CALL(*(observer_.get()), DestinationUpdate(_, _, _))
          .Times(AnyNumber())
          .WillRepeatedly(
              Invoke(this, &DownloadFileTest::SetUpdateDownloadInfo));
    }
  }

  download::DownloadInterruptReason RenameAndUniquify(
      const base::FilePath& full_path,
      base::FilePath* result_path_p) {
    return InvokeRenameMethodAndWaitForCallback(
        RENAME_AND_UNIQUIFY, full_path, result_path_p);
  }

  download::DownloadInterruptReason RenameAndAnnotate(
      const base::FilePath& full_path,
      base::FilePath* result_path_p) {
    return InvokeRenameMethodAndWaitForCallback(
        RENAME_AND_ANNOTATE, full_path, result_path_p);
  }

  void ExpectPermissionError(download::DownloadInterruptReason err) {
    EXPECT_TRUE(err ==
                    download::DOWNLOAD_INTERRUPT_REASON_FILE_TRANSIENT_ERROR ||
                err == download::DOWNLOAD_INTERRUPT_REASON_FILE_ACCESS_DENIED)
        << "Interrupt reason = " << err;
  }

 protected:
  void InvokeRenameMethod(
      DownloadFileRenameMethodType method,
      const base::FilePath& full_path,
      const DownloadFile::RenameCompletionCallback& completion_callback) {
    switch (method) {
      case RENAME_AND_UNIQUIFY:
        download_file_->RenameAndUniquify(full_path, completion_callback);
        break;

      case RENAME_AND_ANNOTATE:
        download_file_->RenameAndAnnotate(
            full_path,
            "12345678-ABCD-1234-DCBA-123456789ABC",
            GURL(),
            GURL(),
            completion_callback);
        break;
    }
  }

  download::DownloadInterruptReason InvokeRenameMethodAndWaitForCallback(
      DownloadFileRenameMethodType method,
      const base::FilePath& full_path,
      base::FilePath* result_path_p) {
    download::DownloadInterruptReason result_reason(
        download::DOWNLOAD_INTERRUPT_REASON_NONE);
    base::FilePath result_path;
    base::RunLoop loop_runner;
    DownloadFile::RenameCompletionCallback completion_callback =
        base::Bind(&DownloadFileTest::SetRenameResult,
                   base::Unretained(this),
                   loop_runner.QuitClosure(),
                   &result_reason,
                   result_path_p);
    InvokeRenameMethod(method, full_path, completion_callback);
    loop_runner.Run();
    return result_reason;
  }

  // Prepare a byte stream to write to the file sink.
  void PrepareStream(StrictMock<MockByteStreamReader>** stream,
                     int64_t offset,
                     bool create_stream,
                     bool will_finish,
                     const char** buffers,
                     size_t num_buffer) {
    if (create_stream)
      *stream = new StrictMock<MockByteStreamReader>();

    // Expectation on MockByteStreamReader for MultipleStreams tests:
    // 1. RegisterCallback: Must called twice. One to set the callback, the
    // other to release the stream.
    // 2. Read: If filled with N buffer, called (N+1) times, where the last Read
    // call doesn't read any data but returns STRAM_COMPLETE.
    // The stream may terminate in the middle and less Read calls are expected.
    // 3. GetStatus: Only called if the stream is completed and last Read call
    // returns STREAM_COMPLETE.
    Sequence seq;
    SetupDataAppend(buffers, num_buffer, *stream, seq, offset);
    if (will_finish)
      SetupFinishStream(download::DOWNLOAD_INTERRUPT_REASON_NONE, *stream, seq);
  }

  void VerifySourceStreamsStates(const SourceStreamTestData& data) {
    DCHECK(download_file_->source_streams_.find(data.offset) !=
           download_file_->source_streams_.end())
        << "Can't find stream at offset : " << data.offset;
    DownloadFileImpl::SourceStream* stream =
        download_file_->source_streams_[data.offset].get();
    DCHECK(stream);
    EXPECT_EQ(data.offset, stream->offset());
    EXPECT_EQ(data.bytes_written, stream->bytes_written());
    EXPECT_EQ(data.finished, stream->is_finished());
  }

  size_t source_streams_count() const {
    DCHECK(download_file_);
    return download_file_->source_streams_.size();
  }

  int64_t TotalBytesReceived() const {
    DCHECK(download_file_);
    return download_file_->TotalBytesReceived();
  }

  std::unique_ptr<StrictMock<MockDownloadDestinationObserver>> observer_;
  base::WeakPtrFactory<DownloadDestinationObserver> observer_factory_;

  // DownloadFile instance we are testing.
  std::unique_ptr<DownloadFileImpl> download_file_;

  // Stream for sending data into the download file.
  // Owned by download_file_; will be alive for lifetime of download_file_.
  StrictMock<MockByteStreamReader>* input_stream_;

  // Additional streams to test multiple stream write.
  std::vector<StrictMock<MockByteStreamReader>*> additional_streams_;

  // Sink callback data for stream.
  base::Closure sink_callback_;

  // Latest update sent to the observer.
  int64_t bytes_;
  int64_t bytes_per_sec_;

 private:
  void SetRenameResult(const base::Closure& closure,
                       download::DownloadInterruptReason* reason_p,
                       base::FilePath* result_path_p,
                       download::DownloadInterruptReason reason,
                       const base::FilePath& result_path) {
    if (reason_p)
      *reason_p = reason;
    if (result_path_p)
      *result_path_p = result_path;
    closure.Run();
  }

  TestBrowserThreadBundle thread_bundle_;

  // Keep track of what data should be saved to the disk file.
  std::string expected_data_;
};

// DownloadFile::RenameAndAnnotate and DownloadFile::RenameAndUniquify have a
// considerable amount of functional overlap. In order to re-use test logic, we
// are going to introduce this value parameterized test fixture. It will take a
// DownloadFileRenameMethodType value which can be either of the two rename
// methods.
class DownloadFileTestWithRename
    : public DownloadFileTest,
      public ::testing::WithParamInterface<DownloadFileRenameMethodType> {
 protected:
  download::DownloadInterruptReason InvokeSelectedRenameMethod(
      const base::FilePath& full_path,
      base::FilePath* result_path_p) {
    return InvokeRenameMethodAndWaitForCallback(
        GetParam(), full_path, result_path_p);
  }
};

// And now instantiate all DownloadFileTestWithRename tests using both
// DownloadFile rename methods. Each test of the form
// DownloadFileTestWithRename.<FooTest> will be instantiated once with
// RenameAndAnnotate as the value parameter and once with RenameAndUniquify as
// the value parameter.
INSTANTIATE_TEST_CASE_P(DownloadFile,
                        DownloadFileTestWithRename,
                        ::testing::Values(RENAME_AND_ANNOTATE,
                                          RENAME_AND_UNIQUIFY));

const char DownloadFileTest::kTestData1[] =
    "Let's write some data to the file!\n";
const char DownloadFileTest::kTestData2[] = "Writing more data.\n";
const char DownloadFileTest::kTestData3[] = "Final line.";
const char DownloadFileTest::kTestData4[] = "abcdefg";
const char DownloadFileTest::kTestData5[] = "01234";
const char* DownloadFileTest::kTestData6[] = {kTestData1, kTestData2};
const char* DownloadFileTest::kTestData7[] = {kTestData4, kTestData5};
const char* DownloadFileTest::kTestData8[] = {kTestData1, kTestData2,
                                              kTestData4, kTestData5};

const char DownloadFileTest::kDataHash[] =
    "CBF68BF10F8003DB86B31343AFAC8C7175BD03FB5FC905650F8C80AF087443A8";
const char DownloadFileTest::kEmptyHash[] =
    "E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855";

const uint32_t DownloadFileTest::kDummyDownloadId = 23;
const int DownloadFileTest::kDummyChildId = 3;
const int DownloadFileTest::kDummyRequestId = 67;

// Rename the file before any data is downloaded, after some has, after it all
// has, and after it's closed.
TEST_P(DownloadFileTestWithRename, RenameFileFinal) {
  ASSERT_TRUE(CreateDownloadFile(0, true));
  base::FilePath initial_path(download_file_->FullPath());
  EXPECT_TRUE(base::PathExists(initial_path));
  base::FilePath path_1(initial_path.InsertBeforeExtensionASCII("_1"));
  base::FilePath path_2(initial_path.InsertBeforeExtensionASCII("_2"));
  base::FilePath path_3(initial_path.InsertBeforeExtensionASCII("_3"));
  base::FilePath path_4(initial_path.InsertBeforeExtensionASCII("_4"));
  base::FilePath output_path;

  // Rename the file before downloading any data.
  EXPECT_EQ(download::DOWNLOAD_INTERRUPT_REASON_NONE,
            InvokeSelectedRenameMethod(path_1, &output_path));
  base::FilePath renamed_path = download_file_->FullPath();
  EXPECT_EQ(path_1, renamed_path);
  EXPECT_EQ(path_1, output_path);

  // Check the files.
  EXPECT_FALSE(base::PathExists(initial_path));
  EXPECT_TRUE(base::PathExists(path_1));

  // Download the data.
  const char* chunks1[] = { kTestData1, kTestData2 };
  AppendDataToFile(chunks1, 2);

  // Rename the file after downloading some data.
  EXPECT_EQ(download::DOWNLOAD_INTERRUPT_REASON_NONE,
            InvokeSelectedRenameMethod(path_2, &output_path));
  renamed_path = download_file_->FullPath();
  EXPECT_EQ(path_2, renamed_path);
  EXPECT_EQ(path_2, output_path);

  // Check the files.
  EXPECT_FALSE(base::PathExists(path_1));
  EXPECT_TRUE(base::PathExists(path_2));

  const char* chunks2[] = { kTestData3 };
  AppendDataToFile(chunks2, 1);

  // Rename the file after downloading all the data.
  EXPECT_EQ(download::DOWNLOAD_INTERRUPT_REASON_NONE,
            InvokeSelectedRenameMethod(path_3, &output_path));
  renamed_path = download_file_->FullPath();
  EXPECT_EQ(path_3, renamed_path);
  EXPECT_EQ(path_3, output_path);

  // Check the files.
  EXPECT_FALSE(base::PathExists(path_2));
  EXPECT_TRUE(base::PathExists(path_3));

  FinishStream(download::DOWNLOAD_INTERRUPT_REASON_NONE, true, kDataHash);
  base::RunLoop().RunUntilIdle();

  // Rename the file after downloading all the data and closing the file.
  EXPECT_EQ(download::DOWNLOAD_INTERRUPT_REASON_NONE,
            InvokeSelectedRenameMethod(path_4, &output_path));
  renamed_path = download_file_->FullPath();
  EXPECT_EQ(path_4, renamed_path);
  EXPECT_EQ(path_4, output_path);

  // Check the files.
  EXPECT_FALSE(base::PathExists(path_3));
  EXPECT_TRUE(base::PathExists(path_4));

  DestroyDownloadFile(0);
}

// Test to make sure the rename overwrites when requested. This is separate from
// the above test because it only applies to RenameAndAnnotate().
// RenameAndUniquify() doesn't overwrite by design.
TEST_F(DownloadFileTest, RenameOverwrites) {
  ASSERT_TRUE(CreateDownloadFile(0, true));
  base::FilePath initial_path(download_file_->FullPath());
  EXPECT_TRUE(base::PathExists(initial_path));
  base::FilePath path_1(initial_path.InsertBeforeExtensionASCII("_1"));

  ASSERT_FALSE(base::PathExists(path_1));
  static const char file_data[] = "xyzzy";
  ASSERT_EQ(static_cast<int>(sizeof(file_data)),
            base::WriteFile(path_1, file_data, sizeof(file_data)));
  ASSERT_TRUE(base::PathExists(path_1));

  base::FilePath new_path;
  EXPECT_EQ(download::DOWNLOAD_INTERRUPT_REASON_NONE,
            RenameAndAnnotate(path_1, &new_path));
  EXPECT_EQ(path_1.value(), new_path.value());

  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(new_path, &file_contents));
  EXPECT_NE(std::string(file_data), file_contents);

  FinishStream(download::DOWNLOAD_INTERRUPT_REASON_NONE, true, kEmptyHash);
  base::RunLoop().RunUntilIdle();
  DestroyDownloadFile(0);
}

// Test to make sure the rename uniquifies if we aren't overwriting
// and there's a file where we're aiming. As above, not a
// DownloadFileTestWithRename test because this only applies to
// RenameAndUniquify().
TEST_F(DownloadFileTest, RenameUniquifies) {
  ASSERT_TRUE(CreateDownloadFile(0, true));
  base::FilePath initial_path(download_file_->FullPath());
  EXPECT_TRUE(base::PathExists(initial_path));
  base::FilePath path_1(initial_path.InsertBeforeExtensionASCII("_1"));
  base::FilePath path_1_suffixed(path_1.InsertBeforeExtensionASCII(" (1)"));

  ASSERT_FALSE(base::PathExists(path_1));
  static const char file_data[] = "xyzzy";
  ASSERT_EQ(static_cast<int>(sizeof(file_data)),
            base::WriteFile(path_1, file_data, sizeof(file_data)));
  ASSERT_TRUE(base::PathExists(path_1));

  EXPECT_EQ(download::DOWNLOAD_INTERRUPT_REASON_NONE,
            RenameAndUniquify(path_1, nullptr));
  EXPECT_TRUE(base::PathExists(path_1_suffixed));

  FinishStream(download::DOWNLOAD_INTERRUPT_REASON_NONE, true, kEmptyHash);
  base::RunLoop().RunUntilIdle();
  DestroyDownloadFile(0);
}

// Test that RenameAndUniquify doesn't try to uniquify in the case where the
// target filename is the same as the current filename.
TEST_F(DownloadFileTest, RenameRecognizesSelfConflict) {
  ASSERT_TRUE(CreateDownloadFile(0, true));
  base::FilePath initial_path(download_file_->FullPath());
  EXPECT_TRUE(base::PathExists(initial_path));

  base::FilePath new_path;
  EXPECT_EQ(download::DOWNLOAD_INTERRUPT_REASON_NONE,
            RenameAndUniquify(initial_path, &new_path));
  EXPECT_TRUE(base::PathExists(initial_path));

  FinishStream(download::DOWNLOAD_INTERRUPT_REASON_NONE, true, kEmptyHash);
  base::RunLoop().RunUntilIdle();
  DestroyDownloadFile(0);
  EXPECT_EQ(initial_path.value(), new_path.value());
}

// Test to make sure we get the proper error on failure.
TEST_P(DownloadFileTestWithRename, RenameError) {
  ASSERT_TRUE(CreateDownloadFile(0, true));
  base::FilePath initial_path(download_file_->FullPath());

  // Create a subdirectory.
  base::FilePath target_dir(
      initial_path.DirName().Append(FILE_PATH_LITERAL("TargetDir")));
  ASSERT_FALSE(base::DirectoryExists(target_dir));
  ASSERT_TRUE(base::CreateDirectory(target_dir));
  base::FilePath target_path(target_dir.Append(initial_path.BaseName()));

  // Targets
  base::FilePath target_path_suffixed(
      target_path.InsertBeforeExtensionASCII(" (1)"));
  ASSERT_FALSE(base::PathExists(target_path));
  ASSERT_FALSE(base::PathExists(target_path_suffixed));

  // Make the directory unwritable and try to rename within it.
  {
    base::FilePermissionRestorer restorer(target_dir);
    ASSERT_TRUE(base::MakeFileUnwritable(target_dir));

    // Expect nulling out of further processing.
    EXPECT_CALL(*input_stream_, RegisterCallback(IsNullCallback()));
    ExpectPermissionError(InvokeSelectedRenameMethod(target_path, nullptr));
    EXPECT_FALSE(base::PathExists(target_path_suffixed));
  }

  FinishStream(download::DOWNLOAD_INTERRUPT_REASON_NONE, true, kEmptyHash);
  base::RunLoop().RunUntilIdle();
  DestroyDownloadFile(0);
}

namespace {

void TestRenameCompletionCallback(
    const base::Closure& closure,
    bool* did_run_callback,
    download::DownloadInterruptReason interrupt_reason,
    const base::FilePath& new_path) {
  EXPECT_EQ(download::DOWNLOAD_INTERRUPT_REASON_NONE, interrupt_reason);
  *did_run_callback = true;
  closure.Run();
}

}  // namespace

// Test that the retry logic works. This test assumes that DownloadFileImpl will
// post tasks to the current message loop (acting as the download sequence)
// asynchronously to retry the renames. We will stuff RunLoop::QuitClosures()
// in between the retry tasks to stagger them and then allow the rename to
// succeed.
//
// Note that there is only one queue of tasks to run, and that is in the tests'
// base::MessageLoop::current(). Each RunLoop processes that queue until it sees
// a QuitClosure() targeted at itself, at which point it stops processing.
TEST_P(DownloadFileTestWithRename, RenameWithErrorRetry) {
  ASSERT_TRUE(CreateDownloadFile(0, true));
  base::FilePath initial_path(download_file_->FullPath());

  // Create a subdirectory.
  base::FilePath target_dir(
      initial_path.DirName().Append(FILE_PATH_LITERAL("TargetDir")));
  ASSERT_FALSE(base::DirectoryExists(target_dir));
  ASSERT_TRUE(base::CreateDirectory(target_dir));
  base::FilePath target_path(target_dir.Append(initial_path.BaseName()));

  bool did_run_callback = false;

  // Each RunLoop can be used the run the MessageLoop until the corresponding
  // QuitClosure() is run. This one is used to produce the QuitClosure() that
  // will be run when the entire rename operation is complete.
  base::RunLoop succeeding_run;
  {
    // (Scope for the base::File or base::FilePermissionRestorer below.)
#if defined(OS_WIN)
    // On Windows we test with an actual transient error, a sharing violation.
    // The rename will fail because we are holding the file open for READ. On
    // Posix this doesn't cause a failure.
    base::File locked_file(initial_path,
                           base::File::FLAG_OPEN | base::File::FLAG_READ);
    ASSERT_TRUE(locked_file.IsValid());
#else
    // Simulate a transient failure by revoking write permission for target_dir.
    // The TestDownloadFileImpl class treats this error as transient even though
    // DownloadFileImpl itself doesn't.
    base::FilePermissionRestorer restore_permissions_for(target_dir);
    ASSERT_TRUE(base::MakeFileUnwritable(target_dir));
#endif

    // The Rename() should fail here and enqueue a retry task without invoking
    // the completion callback.
    InvokeRenameMethod(GetParam(),
                       target_path,
                       base::Bind(&TestRenameCompletionCallback,
                                  succeeding_run.QuitClosure(),
                                  &did_run_callback));
    EXPECT_FALSE(did_run_callback);

    base::RunLoop first_failing_run;
    // Queue the QuitClosure() on the MessageLoop now. Any tasks queued by the
    // Rename() will be in front of the QuitClosure(). Running the message loop
    // now causes the just the first retry task to be run. The rename still
    // fails, so another retry task would get queued behind the QuitClosure().
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, first_failing_run.QuitClosure());
    first_failing_run.Run();
    EXPECT_FALSE(did_run_callback);

    // Running another loop should have the same effect as the above as long as
    // kMaxRenameRetries is greater than 2.
    base::RunLoop second_failing_run;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, second_failing_run.QuitClosure());
    second_failing_run.Run();
    EXPECT_FALSE(did_run_callback);
  }

  // This time the QuitClosure from succeeding_run should get executed.
  succeeding_run.Run();
  EXPECT_TRUE(did_run_callback);

  FinishStream(download::DOWNLOAD_INTERRUPT_REASON_NONE, true, kEmptyHash);
  base::RunLoop().RunUntilIdle();
  DestroyDownloadFile(0);
}

// Various tests of the StreamActive method.
TEST_F(DownloadFileTest, StreamEmptySuccess) {
  ASSERT_TRUE(CreateDownloadFile(0, true));
  base::FilePath initial_path(download_file_->FullPath());
  EXPECT_TRUE(base::PathExists(initial_path));

  // Test that calling the sink_callback_ on an empty stream shouldn't
  // do anything.
  AppendDataToFile(nullptr, 0);

  // Finish the download this way and make sure we see it on the observer.
  FinishStream(download::DOWNLOAD_INTERRUPT_REASON_NONE, true, kEmptyHash);
  base::RunLoop().RunUntilIdle();

  DestroyDownloadFile(0);
}

TEST_F(DownloadFileTest, StreamEmptyError) {
  ASSERT_TRUE(CreateDownloadFile(0, true));
  base::FilePath initial_path(download_file_->FullPath());
  EXPECT_TRUE(base::PathExists(initial_path));

  // Finish the download in error and make sure we see it on the
  // observer.
  EXPECT_CALL(*(observer_.get()),
              MockDestinationError(
                  download::DOWNLOAD_INTERRUPT_REASON_NETWORK_DISCONNECTED, 0,
                  kEmptyHash))
      .WillOnce(InvokeWithoutArgs(
          this, &DownloadFileTest::ConfirmUpdateDownloadInfo));

  // If this next EXPECT_CALL fails flakily, it's probably a real failure.
  // We'll be getting a stream of UpdateDownload calls from the timer, and
  // the last one may have the correct information even if the failure
  // doesn't produce an update, as the timer update may have triggered at the
  // same time.
  EXPECT_CALL(*(observer_.get()), CurrentUpdateStatus(0, _));

  FinishStream(download::DOWNLOAD_INTERRUPT_REASON_NETWORK_DISCONNECTED, false,
               kEmptyHash);

  base::RunLoop().RunUntilIdle();

  DestroyDownloadFile(0);
}

TEST_F(DownloadFileTest, StreamNonEmptySuccess) {
  ASSERT_TRUE(CreateDownloadFile(0, true));
  base::FilePath initial_path(download_file_->FullPath());
  EXPECT_TRUE(base::PathExists(initial_path));

  const char* chunks1[] = { kTestData1, kTestData2 };
  ::testing::Sequence s1;
  SetupDataAppend(chunks1, 2, input_stream_, s1);
  SetupFinishStream(download::DOWNLOAD_INTERRUPT_REASON_NONE, input_stream_,
                    s1);
  EXPECT_CALL(*(observer_.get()), MockDestinationCompleted(_, _));
  sink_callback_.Run();
  VerifyStreamAndSize();
  base::RunLoop().RunUntilIdle();
  DestroyDownloadFile(0);
}

TEST_F(DownloadFileTest, StreamNonEmptyError) {
  ASSERT_TRUE(CreateDownloadFile(0, true));
  base::FilePath initial_path(download_file_->FullPath());
  EXPECT_TRUE(base::PathExists(initial_path));

  const char* chunks1[] = { kTestData1, kTestData2 };
  ::testing::Sequence s1;
  SetupDataAppend(chunks1, 2, input_stream_, s1);
  SetupFinishStream(download::DOWNLOAD_INTERRUPT_REASON_NETWORK_DISCONNECTED,
                    input_stream_, s1);

  EXPECT_CALL(
      *(observer_.get()),
      MockDestinationError(
          download::DOWNLOAD_INTERRUPT_REASON_NETWORK_DISCONNECTED, _, _))
      .WillOnce(InvokeWithoutArgs(
          this, &DownloadFileTest::ConfirmUpdateDownloadInfo));

  // If this next EXPECT_CALL fails flakily, it's probably a real failure.
  // We'll be getting a stream of UpdateDownload calls from the timer, and
  // the last one may have the correct information even if the failure
  // doesn't produce an update, as the timer update may have triggered at the
  // same time.
  EXPECT_CALL(*(observer_.get()),
              CurrentUpdateStatus(strlen(kTestData1) + strlen(kTestData2), _));

  sink_callback_.Run();
  base::RunLoop().RunUntilIdle();
  VerifyStreamAndSize();
  DestroyDownloadFile(0);
}

// Tests for concurrent streams handling, used for parallel download.
//
// Activate both streams at the same time.
TEST_F(DownloadFileTest, MultipleStreamsWrite) {
  int64_t stream_0_length = GetBuffersLength(kTestData6, 2);
  int64_t stream_1_length = GetBuffersLength(kTestData7, 2);

  ASSERT_TRUE(CreateDownloadFile(0, stream_0_length, true,
                                 DownloadItem::ReceivedSlices()));

  PrepareStream(&input_stream_, 0, false, true, kTestData6, 2);
  PrepareStream(&additional_streams_[0], stream_0_length, true, true,
                kTestData7, 2);

  EXPECT_CALL(*additional_streams_[0], RegisterCallback(_))
      .RetiresOnSaturation();
  EXPECT_CALL(*(observer_.get()), MockDestinationCompleted(_, _));

  // Activate the streams.
  download_file_->AddInputStream(
      std::make_unique<DownloadManager::InputStream>(
          std::unique_ptr<ByteStreamReader>(additional_streams_[0])),
      stream_0_length, download::DownloadSaveInfo::kLengthFullContent);
  sink_callback_.Run();
  base::RunLoop().RunUntilIdle();

  SourceStreamTestData stream_data_0(0, stream_0_length, true);
  SourceStreamTestData stream_data_1(stream_0_length, stream_1_length, true);
  VerifySourceStreamsStates(stream_data_0);
  VerifySourceStreamsStates(stream_data_1);
  EXPECT_EQ(stream_0_length + stream_1_length, TotalBytesReceived());

  DestroyDownloadFile(0);
}

// 3 streams write to one sink, the second stream has a limited length.
TEST_F(DownloadFileTest, MutipleStreamsLimitedLength) {
  int64_t stream_0_length = GetBuffersLength(kTestData6, 2);

  // The second stream has a limited length and should be partially written
  // to disk. When we prepare the stream, we fill the stream with 2 full buffer.
  int64_t stream_1_length = GetBuffersLength(kTestData7, 2) - 1;

  // The last stream can't have length limit, it's a half open request, e.g
  // "Range:50-".
  int64_t stream_2_length = GetBuffersLength(kTestData6, 2);

  ASSERT_TRUE(CreateDownloadFile(0, stream_0_length, true,
                                 DownloadItem::ReceivedSlices()));

  PrepareStream(&input_stream_, 0, false, true, kTestData6, 2);
  PrepareStream(&additional_streams_[0], stream_0_length, true, false,
                kTestData7, 2);
  PrepareStream(&additional_streams_[1], stream_0_length + stream_1_length,
                true, true, kTestData6, 2);

  EXPECT_CALL(*additional_streams_[0], RegisterCallback(_))
      .Times(2)
      .RetiresOnSaturation();

  EXPECT_CALL(*additional_streams_[1], RegisterCallback(_))
      .RetiresOnSaturation();

  EXPECT_CALL(*(observer_.get()), MockDestinationCompleted(_, _));

  // Activate all the streams.
  download_file_->AddInputStream(
      std::make_unique<DownloadManager::InputStream>(
          std::unique_ptr<ByteStreamReader>(additional_streams_[0])),
      stream_0_length, stream_1_length);
  download_file_->AddInputStream(
      std::make_unique<DownloadManager::InputStream>(
          std::unique_ptr<ByteStreamReader>(additional_streams_[1])),
      stream_0_length + stream_1_length,
      download::DownloadSaveInfo::kLengthFullContent);
  sink_callback_.Run();
  base::RunLoop().RunUntilIdle();

  SourceStreamTestData stream_data_0(0, stream_0_length, true);
  SourceStreamTestData stream_data_1(stream_0_length, stream_1_length, true);
  SourceStreamTestData stream_data_2(stream_0_length + stream_1_length,
                                     stream_2_length, true);

  VerifySourceStreamsStates(stream_data_0);
  VerifySourceStreamsStates(stream_data_1);
  VerifySourceStreamsStates(stream_data_2);

  EXPECT_EQ(stream_0_length + stream_1_length + stream_2_length,
            TotalBytesReceived());

  download_file_->Cancel();
  DestroyDownloadFile(0, false);
}

// Activate and deplete one stream, later add the second stream.
TEST_F(DownloadFileTest, MultipleStreamsFirstStreamWriteAllData) {
  int64_t stream_0_length = GetBuffersLength(kTestData8, 4);

  ASSERT_TRUE(CreateDownloadFile(0,
                                 download::DownloadSaveInfo::kLengthFullContent,
                                 true, DownloadItem::ReceivedSlices()));

  PrepareStream(&input_stream_, 0, false, true, kTestData8, 4);

  EXPECT_CALL(*(observer_.get()), MockDestinationCompleted(_, _));

  sink_callback_.Run();
  base::RunLoop().RunUntilIdle();

  // Add another stream, the file is already closed, so nothing should be
  // called.
  EXPECT_FALSE(download_file_->InProgress());

  additional_streams_[0] = new StrictMock<MockByteStreamReader>();
  download_file_->AddInputStream(
      std::make_unique<DownloadManager::InputStream>(
          std::unique_ptr<ByteStreamReader>(additional_streams_[0])),
      stream_0_length - 1, download::DownloadSaveInfo::kLengthFullContent);
  base::RunLoop().RunUntilIdle();

  SourceStreamTestData stream_data_0(0, stream_0_length, true);
  VerifySourceStreamsStates(stream_data_0);
  EXPECT_EQ(stream_0_length, TotalBytesReceived());
  EXPECT_EQ(1u, source_streams_count());

  DestroyDownloadFile(0);
}

// While one stream is writing, kick off another stream with an offset that has
// been written by the first one.
TEST_F(DownloadFileTest, SecondStreamStartingOffsetAlreadyWritten) {
  int64_t stream_0_length = GetBuffersLength(kTestData6, 2);

  ASSERT_TRUE(CreateDownloadFile(0, stream_0_length, true,
                                 DownloadItem::ReceivedSlices()));

  Sequence seq;
  SetupDataAppend(kTestData6, 2, input_stream_, seq, 0);

  EXPECT_CALL(*input_stream_, Read(_, _))
      .InSequence(seq)
      .WillOnce(Return(ByteStreamReader::STREAM_EMPTY))
      .RetiresOnSaturation();
  sink_callback_.Run();
  base::RunLoop().RunUntilIdle();

  additional_streams_[0] = new StrictMock<MockByteStreamReader>();
  EXPECT_CALL(*additional_streams_[0], RegisterCallback(_))
      .WillRepeatedly(Invoke(this, &DownloadFileTest::RegisterCallback))
      .RetiresOnSaturation();
  EXPECT_CALL(*additional_streams_[0], Read(_, _))
      .WillOnce(Return(ByteStreamReader::STREAM_EMPTY))
      .RetiresOnSaturation();

  download_file_->AddInputStream(
      std::make_unique<DownloadManager::InputStream>(
          std::unique_ptr<ByteStreamReader>(additional_streams_[0])),
      0, download::DownloadSaveInfo::kLengthFullContent);

  // The stream should get terminated and reset the callback.
  EXPECT_TRUE(sink_callback_.is_null());
  download_file_->Cancel();
  DestroyDownloadFile(0, false);
}

}  // namespace content
