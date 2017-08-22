// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_offer.h"

#include <fcntl.h>
#include <stdio.h>

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "components/exo/data_device.h"
#include "components/exo/data_offer_delegate.h"
#include "components/exo/file_helper.h"
#include "components/exo/test/exo_test_base.h"
#include "components/exo/test/test_data_offer_delegate.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "url/gurl.h"

namespace exo {
namespace {

using DataOfferTest = test::ExoTestBase;

class TestFileHelper : public FileHelper {
 public:
  TestFileHelper() = default;

  // Overridden from FileHelper:
  std::string GetMimeTypeForUriList() const override { return "text/uri-list"; }
  bool ConvertPathToUrl(const base::FilePath& path, GURL* out) override {
    *out = GURL("file://" + path.AsUTF8Unsafe());
    return true;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestFileHelper);
};

struct PipePair {
  base::ScopedFD read;
  base::ScopedFD write;
};

bool CreatePipe(base::ScopedFD out[2]) {
  int fd[2];
  const int result = pipe2(fd, O_CLOEXEC);
  if (result == -1) {
    ADD_FAILURE() << "Cannot create pipe: " << strerror(errno);
    return false;
  }

  out[0] = base::ScopedFD(fd[0]);
  out[1] = base::ScopedFD(fd[1]);
  return true;
}

bool ReadString16(base::ScopedFD fd, base::string16* out) {
  std::array<char, 128> buffer;
  char* it = buffer.begin();
  while (it != buffer.end()) {
    const int result = read(fd.get(), it, buffer.end() - it);
    if (result == -1) {
      ADD_FAILURE() << strerror(errno);
      return false;
    }
    if (result == 0)
      break;
    it += result;
  }
  *out = base::string16(reinterpret_cast<base::char16*>(buffer.data()),
                        (it - buffer.begin()) / sizeof(base::char16));
  return true;
}

TEST_F(DataOfferTest, SendEvents) {
  base::flat_set<DndAction> source_actions;
  source_actions.insert(DndAction::kCopy);
  source_actions.insert(DndAction::kMove);

  ui::OSExchangeData data;
  data.SetString(base::string16(base::ASCIIToUTF16("Test data")));

  TestDataOfferDelegate delegate;
  DataOffer data_offer(&delegate);

  EXPECT_EQ(0u, delegate.mime_types().size());
  EXPECT_EQ(0u, delegate.source_actions().size());
  EXPECT_EQ(DndAction::kNone, delegate.dnd_action());

  TestFileHelper file_helper;
  data_offer.SetDropData(&file_helper, data);
  data_offer.SetSourceActions(source_actions);
  data_offer.SetActions(base::flat_set<DndAction>(), DndAction::kMove);

  EXPECT_EQ(1u, delegate.mime_types().size());
  EXPECT_EQ("text/plain", delegate.mime_types()[0]);
  EXPECT_EQ(2u, delegate.source_actions().size());
  EXPECT_EQ(1u, delegate.source_actions().count(DndAction::kCopy));
  EXPECT_EQ(1u, delegate.source_actions().count(DndAction::kMove));
  EXPECT_EQ(DndAction::kMove, delegate.dnd_action());
}

TEST_F(DataOfferTest, SetFileDropData) {
  TestDataOfferDelegate delegate;
  DataOffer data_offer(&delegate);

  TestFileHelper file_helper;
  ui::OSExchangeData data;
  data.SetFilename(base::FilePath("/test/downloads/file"));
  data_offer.SetDropData(&file_helper, data);

  EXPECT_EQ(1u, delegate.mime_types().size());
  EXPECT_EQ("text/uri-list", delegate.mime_types()[0]);
}

TEST_F(DataOfferTest, ReceiveString) {
  TestDataOfferDelegate delegate;
  DataOffer data_offer(&delegate);

  TestFileHelper file_helper;
  ui::OSExchangeData data;
  data.SetString(base::ASCIIToUTF16("Test data"));
  data_offer.SetDropData(&file_helper, data);

  base::ScopedFD pipe[2];
  ASSERT_TRUE(CreatePipe(pipe));

  data_offer.Receive("text/plain", std::move(pipe[1]));
  base::string16 result;
  ASSERT_TRUE(ReadString16(std::move(pipe[0]), &result));
  EXPECT_EQ(base::ASCIIToUTF16("Test data"), result);
}

TEST_F(DataOfferTest, ReceiveUriList) {
  TestDataOfferDelegate delegate;
  DataOffer data_offer(&delegate);

  TestFileHelper file_helper;
  ui::OSExchangeData data;
  data.SetFilename(base::FilePath("/test/downloads/file"));
  data_offer.SetDropData(&file_helper, data);

  base::ScopedFD pipe[2];
  ASSERT_TRUE(CreatePipe(pipe));

  data_offer.Receive("text/uri-list", std::move(pipe[1]));
  base::string16 result;
  ASSERT_TRUE(ReadString16(std::move(pipe[0]), &result));
  EXPECT_EQ(base::ASCIIToUTF16("file:///test/downloads/file"), result);
}

}  // namespace
}  // namespace exo
