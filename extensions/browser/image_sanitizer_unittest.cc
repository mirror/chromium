// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/image_sanitizer.h"

#include <map>
#include <memory>

#include "base/base64.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "services/data_decoder/data_decoder_service.h"
#include "services/service_manager/public/cpp/test/test_connector_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {

constexpr char kBase64edValidPng[] =
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAACQd"
    "1PeAAAADElEQVQI12P4//8/AAX+Av7czFnnAAAAAElFTkSuQmCC";

constexpr char kBase64edInvalidPng[] =
    "Rw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAACQd"
    "1PeAAAADElEQVQI12P4//8/AAX+Av7czFnnAAAAAElFTkSuQmCC";

class ImageSanitizerTest : public testing::Test {
 public:
  ImageSanitizerTest()
      : thread_bundle_(content::TestBrowserThreadBundle::DEFAULT) {}

 protected:
  void CreateValidImage(const std::string& file_name) {
    ASSERT_TRUE(WriteBase64DataToFile(kBase64edValidPng, file_name));
  }

  void CreateInvalidImage(const std::string& file_name) {
    ASSERT_TRUE(WriteBase64DataToFile(kBase64edInvalidPng, file_name));
  }

  service_manager::Connector* GetConnector() const { return connector_.get(); }

  const base::FilePath& GetImagePath() const { return temp_dir_.GetPath(); }

  void WaitForSanitizationDone() {
    ASSERT_FALSE(done_callback_);
    base::RunLoop run_loop;
    done_callback_ = run_loop.QuitClosure();
    run_loop.Run();
  }

  void CreateAndStartSanitizer(
      const std::set<base::FilePath>& image_relative_paths) {
    sanitizer_ = ImageSanitizer::CreateAndStart(
        connector_.get(), service_manager::Identity(), temp_dir_.GetPath(),
        image_relative_paths,
        base::BindOnce(&ImageSanitizerTest::ImageSanitizationDone,
                       base::Unretained(this)),
        base::BindRepeating(&ImageSanitizerTest::ImageSanitizerDecodedImage,
                            base::Unretained(this)));
  }

  ImageSanitizer::Status last_reported_status() const { return last_status_; }

  const base::FilePath& last_reported_path() const {
    return last_reported_path_;
  }

  std::map<base::FilePath, SkBitmap>* decoded_images() {
    return &decoded_images_;
  }

 private:
  bool WriteBase64DataToFile(const std::string& base64_data,
                             const std::string& file_name) {
    std::string binary;
    if (!base::Base64Decode(base64_data, &binary))
      return false;

    base::FilePath path = temp_dir_.GetPath().Append(file_name);
    return base::WriteFile(path, binary.data(), binary.size());
  }

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    connector_factory_ =
        service_manager::TestConnectorFactory::CreateForUniqueService(
            data_decoder::DataDecoderService::Create());
    connector_ = connector_factory_->CreateConnector();
  }

  void ImageSanitizationDone(ImageSanitizer::Status status,
                             const base::FilePath& path) {
    last_status_ = status;
    last_reported_path_ = path;
    if (done_callback_)
      std::move(done_callback_).Run();
  }

  void ImageSanitizerDecodedImage(const base::FilePath& path, SkBitmap image) {
    EXPECT_EQ(decoded_images()->find(path), decoded_images()->end());
    (*decoded_images())[path] = image;
  }

  content::TestBrowserThreadBundle thread_bundle_;

  ImageSanitizer::Status last_status_;

  base::FilePath last_reported_path_;

  base::OnceClosure done_callback_;

  std::unique_ptr<ImageSanitizer> sanitizer_;

  base::ScopedTempDir temp_dir_;

  std::unique_ptr<service_manager::TestConnectorFactory> connector_factory_;

  std::unique_ptr<service_manager::Connector> connector_;

  std::map<base::FilePath, SkBitmap> decoded_images_;

  DISALLOW_COPY_AND_ASSIGN(ImageSanitizerTest);
};

}  // namespace

TEST_F(ImageSanitizerTest, NoImagesProvided) {
  CreateAndStartSanitizer(std::set<base::FilePath>());
  WaitForSanitizationDone();
  EXPECT_EQ(last_reported_status(), ImageSanitizer::Status::kSuccess);
  EXPECT_TRUE(last_reported_path().empty());
}

TEST_F(ImageSanitizerTest, InvalidPathAbsolute) {
  base::FilePath normal_path(FILE_PATH_LITERAL("hello.png"));
  base::FilePath absolute_path(FILE_PATH_LITERAL("/usr/bin/root"));
  CreateAndStartSanitizer({normal_path, absolute_path});
  WaitForSanitizationDone();
  EXPECT_EQ(last_reported_status(), ImageSanitizer::Status::kImagePathError);
  EXPECT_EQ(last_reported_path(), absolute_path);
}

TEST_F(ImageSanitizerTest, InvalidPathReferenceParent) {
  base::FilePath good_path(FILE_PATH_LITERAL("hello.png"));
  base::FilePath bad_path(FILE_PATH_LITERAL("../../usr/bin/root"));
  CreateAndStartSanitizer({good_path, bad_path});
  WaitForSanitizationDone();
  EXPECT_EQ(last_reported_status(), ImageSanitizer::Status::kImagePathError);
  EXPECT_EQ(last_reported_path(), bad_path);
}

TEST_F(ImageSanitizerTest, ValidCase) {
  std::set<base::FilePath> paths;
  for (int i = 0; i < 10; i++) {
    std::string name("image");
    name.append(base::NumberToString(i));
    name.append(".png");
    CreateValidImage(name);
    paths.insert(base::FilePath::FromUTF8Unsafe(name));
  }
  CreateAndStartSanitizer(paths);
  WaitForSanitizationDone();
  EXPECT_EQ(last_reported_status(), ImageSanitizer::Status::kSuccess);
  EXPECT_TRUE(last_reported_path().empty());
  // Make sure the image files are there and non empty, and that the
  // ImageSanitizerDecodedImage callback was invoked for every image.
  for (const auto& path : paths) {
    int64_t file_size = 0;
    base::FilePath full_path = GetImagePath().Append(path);
    EXPECT_TRUE(base::GetFileSize(full_path, &file_size));
    EXPECT_GT(file_size, 0);

    ASSERT_NE(decoded_images()->find(path), decoded_images()->end());
    EXPECT_FALSE((*decoded_images())[path].drawsNothing());
  }
  // No extra images should have been reported.
  EXPECT_EQ(decoded_images()->size(), 10U);
}

TEST_F(ImageSanitizerTest, MissingImage) {
  constexpr char kGoodPngName[] = "image.png";
  constexpr char kNonExistingName[] = "i_don_t_exist.png";
  CreateValidImage(kGoodPngName);
  base::FilePath good_png(FILE_PATH_LITERAL(kGoodPngName));
  base::FilePath bad_png(FILE_PATH_LITERAL(kNonExistingName));
  CreateAndStartSanitizer(
      {good_png, base::FilePath(FILE_PATH_LITERAL(kNonExistingName))});
  WaitForSanitizationDone();
  EXPECT_EQ(last_reported_status(), ImageSanitizer::Status::kFileReadError);
  EXPECT_EQ(last_reported_path(), bad_png);
}

TEST_F(ImageSanitizerTest, InvalidImage) {
  constexpr char kGoodPngName[] = "good.png";
  constexpr char kBadPngName[] = "bad.png";
  CreateValidImage(kGoodPngName);
  CreateInvalidImage(kBadPngName);
  base::FilePath good_png(FILE_PATH_LITERAL(kGoodPngName));
  base::FilePath bad_png(FILE_PATH_LITERAL(kBadPngName));
  CreateAndStartSanitizer({good_png, bad_png});
  WaitForSanitizationDone();
  EXPECT_EQ(last_reported_status(), ImageSanitizer::Status::kDecodingError);
  EXPECT_EQ(last_reported_path(), bad_png);
}

}  // namespace extensions
