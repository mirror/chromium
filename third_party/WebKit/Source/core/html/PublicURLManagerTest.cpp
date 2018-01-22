// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/PublicURLManager.h"

#include "core/html/URLRegistry.h"
#include "core/testing/NullExecutionContext.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "platform/testing/RuntimeEnabledFeaturesTestHelpers.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
namespace {

using mojom::blink::Blob;
using mojom::blink::BlobPtr;
using mojom::blink::BlobRequest;

class TestURLRegistrable : public URLRegistrable {
 public:
  TestURLRegistrable(URLRegistry* registry, BlobPtr blob = nullptr)
      : registry_(registry), blob_(std::move(blob)) {}

  URLRegistry& Registry() const override { return *registry_; }

  BlobPtr AsMojoBlob() override {
    if (!blob_)
      return nullptr;
    BlobPtr result;
    blob_->Clone(MakeRequest(&result));
    return result;
  }

 private:
  URLRegistry* const registry_;
  BlobPtr blob_;
};

class FakeURLRegistry : public URLRegistry {
 public:
  void RegisterURL(SecurityOrigin* origin,
                   const KURL& url,
                   URLRegistrable* registrable) override {
    registrations.push_back(Registration{origin, url, registrable});
  }
  void UnregisterURL(const KURL&) override {}

  struct Registration {
    SecurityOrigin* origin;
    KURL url;
    URLRegistrable* registrable;
  };
  Vector<Registration> registrations;
};

class MockBlob : public Blob {
 public:
  explicit MockBlob(const String& uuid) : uuid_(uuid) {}

  void Clone(BlobRequest request) override {
    mojo::MakeStrongBinding(std::make_unique<MockBlob>(uuid_),
                            std::move(request));
  }

  void ReadRange(uint64_t offset,
                 uint64_t length,
                 mojo::ScopedDataPipeProducerHandle,
                 mojom::blink::BlobReaderClientPtr) override {
    NOTREACHED();
  }

  void ReadAll(mojo::ScopedDataPipeProducerHandle,
               mojom::blink::BlobReaderClientPtr) override {
    NOTREACHED();
  }

  void GetInternalUUID(GetInternalUUIDCallback callback) override {
    std::move(callback).Run(uuid_);
  }

 private:
  String uuid_;
};

}  // namespace

class PublicURLManagerTest : public ::testing::Test {
 public:
  void SetUp() override {
    execution_context_ = new NullExecutionContext;
    execution_context_->SetUpSecurityContext();
  }

  PublicURLManager& url_manager() {
    return execution_context_->GetPublicURLManager();
  }

  BlobPtr CreateMojoBlob(const String& uuid) {
    BlobPtr result;
    mojo::MakeStrongBinding(std::make_unique<MockBlob>(uuid),
                            MakeRequest(&result));
    return result;
  }

 protected:
  ScopedMojoBlobURLsForTest mojo_blob_urls_ = true;
  Persistent<NullExecutionContext> execution_context_;
};

TEST_F(PublicURLManagerTest, RegisterNonMojoBlob) {
  FakeURLRegistry registry;
  TestURLRegistrable registrable(&registry);
  String url = url_manager().RegisterURL(&registrable);
  ASSERT_EQ(1u, registry.registrations.size());
  EXPECT_EQ(execution_context_->GetSecurityOrigin(),
            registry.registrations[0].origin);
  EXPECT_EQ(url, registry.registrations[0].url);
  EXPECT_EQ(&registrable, registry.registrations[0].registrable);

  EXPECT_EQ(execution_context_->GetSecurityOrigin(),
            SecurityOrigin::CreateFromString(url));
}

TEST_F(PublicURLManagerTest, RegisterMojoBlob) {
  FakeURLRegistry registry;
  TestURLRegistrable registrable(&registry, CreateMojoBlob("id"));
  String url = url_manager().RegisterURL(&registrable);
  EXPECT_EQ(0u, registry.registrations.size());
}

}  // namespace blink
