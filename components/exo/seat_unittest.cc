// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/seat.h"

#include "base/memory/scoped_refptr.h"
#include "base/strings/utf_string_conversions.h"
#include "components/exo/data_source.h"
#include "components/exo/data_source_delegate.h"
#include "components/exo/seat_observer.h"
#include "components/exo/test/exo_test_base.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

namespace exo {
namespace {

using SeatTest = test::ExoTestBase;

class MockSeatObserver : public SeatObserver {
 public:
  int on_surface_focused_count() { return on_surface_focused_count_; }

  // Overridden from SeatObserver:
  void OnSurfaceFocusing(Surface* gaining_focus) override {
    ASSERT_EQ(on_surface_focused_count_, on_surface_pre_focused_count_);
    on_surface_pre_focused_count_++;
  }
  void OnSurfaceFocused(Surface* gained_focus) override {
    on_surface_focused_count_++;
    ASSERT_EQ(on_surface_focused_count_, on_surface_pre_focused_count_);
  }

 private:
  int on_surface_pre_focused_count_ = 0;
  int on_surface_focused_count_ = 0;
};

class TestDataSource : public DataSourceDelegate, public DataSource {
 public:
  TestDataSource() : DataSource(this) {}
  ~TestDataSource() override {}

  ReadDataCallback& callback() { return callback_; }
  bool cancelled() const { return cancelled_; }

  // Overriden from DataSource:
  void Cancelled() override { cancelled_ = true; }
  void ReadData(ReadDataCallback callback) override {
    callback_ = std::move(callback);
  }

  // Overriden from TestDataSourceDelegate:
  void OnDataSourceDestroying(DataSource* source) override {}
  void OnTarget(const std::string& mime_type) override {}
  void OnSend(const std::string& mime_type, base::ScopedFD fd) override {}
  void OnCancelled() override {}
  void OnDndDropPerformed() override {}
  void OnDndFinished() override {}
  void OnAction(DndAction dnd_action) override {}

 private:
  bool cancelled_ = false;
  ReadDataCallback callback_;
};

TEST_F(SeatTest, OnSurfaceFocused) {
  Seat seat;
  MockSeatObserver observer;

  seat.AddObserver(&observer);
  seat.OnWindowFocused(nullptr, nullptr);
  ASSERT_EQ(1, observer.on_surface_focused_count());

  seat.RemoveObserver(&observer);
  seat.OnWindowFocused(nullptr, nullptr);
  ASSERT_EQ(1, observer.on_surface_focused_count());
}

TEST_F(SeatTest, OnSetSelection) {
  Seat seat;

  TestDataSource source;
  seat.SetSelection(&source);

  std::string test_data = "TestData";
  std::vector<uint8_t> test_data_bytes(test_data.begin(), test_data.end());

  std::move(source.callback()).Run(test_data_bytes, &source);

  std::string clipboard;
  ui::Clipboard::GetForCurrentThread()->ReadAsciiText(
      ui::CLIPBOARD_TYPE_COPY_PASTE, &clipboard);

  EXPECT_EQ(clipboard, test_data);
}

TEST_F(SeatTest, OnSetSelection_TwiceSame) {
  Seat seat;

  TestDataSource source;

  seat.SetSelection(&source);
  std::move(source.callback()).Run(std::vector<uint8_t>(), &source);

  seat.SetSelection(&source);
  EXPECT_TRUE(source.callback().is_null());
}

TEST_F(SeatTest, OnSetSelection_TwiceDifferent) {
  Seat seat;

  TestDataSource source1;
  seat.SetSelection(&source1);
  std::move(source1.callback()).Run(std::vector<uint8_t>(), &source1);

  TestDataSource source2;
  seat.SetSelection(&source2);
  std::move(source2.callback()).Run(std::vector<uint8_t>(), &source2);

  EXPECT_TRUE(source1.cancelled());
}

TEST_F(SeatTest, OnSetSelection_ClipboardChanged) {
  Seat seat;

  TestDataSource source1;
  seat.SetSelection(&source1);
  std::move(source1.callback()).Run(std::vector<uint8_t>(), &source1);

  TestDataSource source2;
  seat.SetSelection(&source2);

  {
    ui::ScopedClipboardWriter writer(ui::CLIPBOARD_TYPE_COPY_PASTE);
    writer.WriteText(base::UTF8ToUTF16("Test data"));
  }

  // The previous source should be cancelled.
  EXPECT_TRUE(source1.cancelled());

  // Running callback for source2 should not set the new clipboard data because
  // the clipboard content is updated after calling SetSelection(&source2).
  std::move(source2.callback()).Run(std::vector<uint8_t>(), &source2);
  EXPECT_TRUE(source2.cancelled());
  std::string clipboard;
  ui::Clipboard::GetForCurrentThread()->ReadAsciiText(
      ui::CLIPBOARD_TYPE_COPY_PASTE, &clipboard);
  EXPECT_EQ(clipboard, "Test data");
}

TEST_F(SeatTest, OnSetSelection_SourceDestroyedDuringSetSelection) {
  Seat seat;

  DataSource::ReadDataCallback callback;
  {
    TestDataSource source;
    seat.SetSelection(&source);
    callback = std::move(source.callback());
    // source destroyed here.
  }

  std::string test_data = "TestData";
  // Pass nullptr means data source has already been destroyed.
  std::move(callback).Run({test_data.begin(), test_data.end()}, nullptr);

  // It should not affect reading data which has already started.
  std::string clipboard;
  ui::Clipboard::GetForCurrentThread()->ReadAsciiText(
      ui::CLIPBOARD_TYPE_COPY_PASTE, &clipboard);
  EXPECT_EQ(clipboard, test_data);
}

TEST_F(SeatTest, OnSetSelection_SourceDestroyedAfterSetSelection) {
  Seat seat;

  {
    TestDataSource source;
    seat.SetSelection(&source);
    std::move(source.callback()).Run(std::vector<uint8_t>(), &source);
    // source destroyed here.
  }

  TestDataSource source;
  seat.SetSelection(&source);
  std::move(source.callback()).Run(std::vector<uint8_t>(), &source);
}

}  // namespace
}  // namespace exo
