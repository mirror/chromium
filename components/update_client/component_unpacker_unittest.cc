// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iterator>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/task_scheduler/post_task.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "components/crx_file/crx_verifier.h"
#include "components/update_client/component_unpacker.h"
#include "components/update_client/test_configurator.h"
#include "components/update_client/test_installer.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_ANDROID)
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "jni/ApkUtils_jni.h"
#endif

using base::android::ClearException;
using base::android::ConvertUTF8ToJavaString;
using base::android::GetClass;
using base::android::MethodID;
using base::android::ScopedJavaLocalRef;

namespace {

class TestCallback {
 public:
  TestCallback();
  virtual ~TestCallback() {}
  void Set(update_client::UnpackerError error, int extra_code);

  update_client::UnpackerError error_;
  int extra_code_;
  bool called_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestCallback);
};

TestCallback::TestCallback()
    : error_(update_client::UnpackerError::kNone),
      extra_code_(-1),
      called_(false) {}

void TestCallback::Set(update_client::UnpackerError error, int extra_code) {
  error_ = error;
  extra_code_ = extra_code;
  called_ = true;
}

base::FilePath test_file(const char* file) {
  base::FilePath path;
  PathService::Get(base::DIR_SOURCE_ROOT, &path);
  return path.AppendASCII("components")
      .AppendASCII("test")
      .AppendASCII("data")
      .AppendASCII("update_client")
      .AppendASCII(file);
}

base::FilePath apk_test_file(const char* file) {
  return test_file("apk").AppendASCII(file);
}

}  // namespace

namespace update_client {

class ComponentUnpackerTest : public testing::Test {
 public:
  ComponentUnpackerTest();
  ~ComponentUnpackerTest() override;

  void UnpackComplete(const ComponentUnpacker::Result& result);

 protected:
  void RunThreads();

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  const scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_ =
      base::ThreadTaskRunnerHandle::Get();
  base::RunLoop runloop_;
  const base::Closure quit_closure_ = runloop_.QuitClosure();

  ComponentUnpacker::Result result_;
};

ComponentUnpackerTest::ComponentUnpackerTest() = default;

ComponentUnpackerTest::~ComponentUnpackerTest() = default;

void ComponentUnpackerTest::RunThreads() {
  runloop_.Run();
}

void ComponentUnpackerTest::UnpackComplete(
    const ComponentUnpacker::Result& result) {
  result_ = result;
  main_thread_task_runner_->PostTask(FROM_HERE, quit_closure_);
}

TEST_F(ComponentUnpackerTest, UnpackFullCrx) {
  scoped_refptr<ComponentUnpacker> component_unpacker =
      base::MakeRefCounted<ComponentUnpacker>(
          std::vector<uint8_t>(std::begin(jebg_hash), std::end(jebg_hash)),
          test_file("jebgalgnebhfojomionfpkfelancnnkf.crx"), nullptr, nullptr);
  component_unpacker->Unpack(base::Bind(&ComponentUnpackerTest::UnpackComplete,
                                        base::Unretained(this)));
  RunThreads();

  EXPECT_EQ(UnpackerError::kNone, result_.error);
  EXPECT_EQ(0, result_.extended_error);

  base::FilePath unpack_path = result_.unpack_path;
  EXPECT_FALSE(unpack_path.empty());
  EXPECT_TRUE(base::DirectoryExists(unpack_path));

  int64_t file_size = 0;
  EXPECT_TRUE(
      base::GetFileSize(unpack_path.AppendASCII("component1.dll"), &file_size));
  EXPECT_EQ(1024, file_size);
  EXPECT_TRUE(
      base::GetFileSize(unpack_path.AppendASCII("flashtest.pem"), &file_size));
  EXPECT_EQ(911, file_size);
  EXPECT_TRUE(
      base::GetFileSize(unpack_path.AppendASCII("manifest.json"), &file_size));
  EXPECT_EQ(144, file_size);

  EXPECT_TRUE(base::DeleteFile(unpack_path, true));
}

TEST_F(ComponentUnpackerTest, UnpackFileNotFound) {
  scoped_refptr<ComponentUnpacker> component_unpacker =
      base::MakeRefCounted<ComponentUnpacker>(
          std::vector<uint8_t>(std::begin(jebg_hash), std::end(jebg_hash)),
          test_file("file-not-found.crx"), nullptr, nullptr);
  component_unpacker->Unpack(base::Bind(&ComponentUnpackerTest::UnpackComplete,
                                        base::Unretained(this)));
  RunThreads();

  EXPECT_EQ(UnpackerError::kInvalidFile, result_.error);
  EXPECT_EQ(static_cast<int>(crx_file::VerifierResult::ERROR_FILE_NOT_READABLE),
            result_.extended_error);

  EXPECT_TRUE(result_.unpack_path.empty());
}

// Tests a mismatch between the public key hash and the id of the component.
TEST_F(ComponentUnpackerTest, UnpackFileHashMismatch) {
  scoped_refptr<ComponentUnpacker> component_unpacker =
      base::MakeRefCounted<ComponentUnpacker>(
          std::vector<uint8_t>(std::begin(abag_hash), std::end(abag_hash)),
          test_file("jebgalgnebhfojomionfpkfelancnnkf.crx"), nullptr, nullptr);
  component_unpacker->Unpack(base::Bind(&ComponentUnpackerTest::UnpackComplete,
                                        base::Unretained(this)));
  RunThreads();

  EXPECT_EQ(UnpackerError::kInvalidFile, result_.error);
  EXPECT_EQ(
      static_cast<int>(crx_file::VerifierResult::ERROR_REQUIRED_PROOF_MISSING),
      result_.extended_error);

  EXPECT_TRUE(result_.unpack_path.empty());
}

#if defined(OS_ANDROID)
TEST_F(ComponentUnpackerTest, LoadPublicKeyFromX509Cert) {
  JNIEnv* env = base::android::AttachCurrentThread();

  auto cert1_path =
      ConvertUTF8ToJavaString(env, apk_test_file("1.crt").MaybeAsASCII());
  auto cert2_path =
      ConvertUTF8ToJavaString(env, apk_test_file("2.crt").MaybeAsASCII());

  // Loading a public key from a valid certificate doesn't throw an exception.

  Java_ApkUtils_loadPublicKey(env, cert1_path);
  EXPECT_FALSE(ClearException(env));

  Java_ApkUtils_loadPublicKey(env, cert2_path);
  EXPECT_FALSE(ClearException(env));

  // Trying to open a file that doesn't exist causes a FileNotFoundException to
  // be thrown.
  auto file_not_found_exception =
      GetClass(env, "java/io/FileNotFoundException");
  Java_ApkUtils_loadPublicKey(env, ConvertUTF8ToJavaString(env, "nonexistent"));
  auto* exception = env->ExceptionOccurred();
  EXPECT_TRUE(ClearException(env));
  EXPECT_TRUE(env->IsInstanceOf(exception, file_not_found_exception.obj()));
}

TEST_F(ComponentUnpackerTest, VerifyAPKSignature) {
  JNIEnv* env = base::android::AttachCurrentThread();

  // Get reference to java.io.RandomAccessFile class and constructor so we can
  // instantiate the class below.
  auto raf_class = GetClass(env, "java/io/RandomAccessFile");
  auto* raf_constructor = MethodID::Get<MethodID::TYPE_INSTANCE>(
      env, raf_class.obj(), "<init>",
      "(Ljava/lang/String;Ljava/lang/String;)V");
  // Java string "r" meaning read-only permission.
  auto perm_string = ConvertUTF8ToJavaString(env, "r");
  // Function to instantiate java.io.RandomAccessFile.
  auto new_raf = [&](const char* file) {
    return env->NewObject(
        raf_class.obj(), raf_constructor,
        ConvertUTF8ToJavaString(env, apk_test_file(file).MaybeAsASCII()).obj(),
        perm_string.obj());
  };

  // An APK signed with certificate #1.
  ScopedJavaLocalRef<jobject> signed1_apk(env, new_raf("signed-1.apk"));
  // Signed with #2.
  ScopedJavaLocalRef<jobject> signed2_apk(env, new_raf("signed-2.apk"));
  // Signed with both certificates.
  ScopedJavaLocalRef<jobject> signed_both_apk(env, new_raf("signed-1-2.apk"));
  // Not signed.
  ScopedJavaLocalRef<jobject> unsigned_apk(env, new_raf("unsigned.apk"));

  auto cert1_path =
      ConvertUTF8ToJavaString(env, apk_test_file("1.crt").MaybeAsASCII());
  auto cert2_path =
      ConvertUTF8ToJavaString(env, apk_test_file("2.crt").MaybeAsASCII());

  auto pk1 = Java_ApkUtils_loadPublicKey(env, cert1_path);
  auto pk2 = Java_ApkUtils_loadPublicKey(env, cert2_path);

  // Signed APKs should verify with the corresponding public key.
  Java_ApkUtils_verify(env, signed1_apk, pk1);
  EXPECT_FALSE(ClearException(env));

  Java_ApkUtils_verify(env, signed2_apk, pk2);
  EXPECT_FALSE(ClearException(env));

  // APKs signed with multiple certificates should verify with either
  // corresponding public key.
  Java_ApkUtils_verify(env, signed_both_apk, pk1);
  EXPECT_FALSE(ClearException(env));

  Java_ApkUtils_verify(env, signed_both_apk, pk2);
  EXPECT_FALSE(ClearException(env));

  // InvalidKeyException should be thrown when none of the signing certificates
  // match the given public key.
  auto invalid_key_exception =
      GetClass(env, "java/security/InvalidKeyException");
  // ApkSignatureSchemeV2Verifier.SignatureNotFoundException should be thrown
  // when an APK isn't signed at all.
  auto signature_not_found_exception =
      GetClass(env,
               "org/chromium/third_party/android/util/apk/"
               "ApkSignatureSchemeV2Verifier$SignatureNotFoundException");

  {
    // Signed APKs shouldn't verify given the wrong public key.
    Java_ApkUtils_verify(env, signed1_apk, pk2);
    auto* exception = env->ExceptionOccurred();
    EXPECT_TRUE(ClearException(env));
    EXPECT_TRUE(env->IsInstanceOf(exception, invalid_key_exception.obj()));
  }

  {
    Java_ApkUtils_verify(env, signed2_apk, pk1);
    auto* exception = env->ExceptionOccurred();
    EXPECT_TRUE(ClearException(env));
    EXPECT_TRUE(env->IsInstanceOf(exception, invalid_key_exception.obj()));
  }

  {
    // Unsigned APKs shouldn't verify.
    Java_ApkUtils_verify(env, unsigned_apk, pk1);
    auto* exception = env->ExceptionOccurred();
    EXPECT_TRUE(ClearException(env));
    EXPECT_TRUE(
        env->IsInstanceOf(exception, signature_not_found_exception.obj()));
  }
}
#endif

}  // namespace update_client
