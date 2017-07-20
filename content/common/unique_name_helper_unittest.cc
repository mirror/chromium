// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/unique_name_helper.h"

#include <map>
#include <vector>

#include "base/auto_reset.h"
#include "base/optional.h"
#include "base/strings/nullable_string16.h"
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "content/common/page_state_serialization.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace {

class TestAdapter : public UniqueNameHelper::FrameAdapter {
 public:
  explicit TestAdapter(TestAdapter* parent,
                       int index_in_parent,
                       const std::string& requested_name)
      : parent_(parent),
        index_in_parent_(index_in_parent),
        requested_name_(requested_name),
        unique_name_helper_(this) {
    unique_name_helper_.UpdateName(requested_name);
    if (parent_)
      parent_->children_.push_back(this);
  }

  ~TestAdapter() override {
    if (parent_) {
      parent_->children_.erase(std::find(parent_->children_.begin(),
                                         parent_->children_.end(), this));
    }
  }

  bool IsMainFrame() const override { return !parent_; }

  bool IsCandidateUnique(const std::string& name) const override {
    auto* top = this;
    while (top->parent_)
      top = top->parent_;
    return top->CheckUniqueness(name);
  }

  int GetSiblingCount() const override { return index_in_parent_; }

  int GetChildCount() const override {
    EXPECT_TRUE(false);
    return 0;
  }

  std::vector<base::StringPiece> CollectAncestorNames(
      BeginPoint begin_point,
      bool (*should_stop)(base::StringPiece)) const override {
    EXPECT_EQ(BeginPoint::kParentFrame, begin_point);
    std::vector<base::StringPiece> result;
    for (auto* adapter = parent_; adapter; adapter = adapter->parent_) {
      result.push_back(adapter->GetNameForCurrentMode());
      if (should_stop(result.back()))
        break;
    }
    return result;
  }

  std::vector<int> GetFramePosition(BeginPoint begin_point) const override {
    EXPECT_EQ(BeginPoint::kParentFrame, begin_point);
    std::vector<int> result;
    for (auto* adapter = this; adapter->parent_; adapter = adapter->parent_)
      result.push_back(adapter->index_in_parent_);
    return result;
  }

  const std::string& GetUniqueName() const {
    return unique_name_helper_.value();
  }

  // Calculate and return the legacy style name with no max size limit.
  const std::string& GetLegacyName() const {
    if (!legacy_name_) {
      // Always initialize with an empty name: this avoids infinite recursion
      // when checking if the legacy name candidate is unique, by preventing
      // reentrancy into GetLegacyName().
      legacy_name_.emplace();
      // Manually skip the main frame so its legacy name is always empty: this
      // is needed in the test as that logic lives at a different layer in
      // UniqueNameHelper.
      if (!IsMainFrame()) {
        base::AutoReset<bool> enable_legacy_mode(&generate_legacy_name_, true);
        legacy_name_.emplace(UniqueNameHelper::CalculateLegacyNameForTest(
            this, requested_name_));
      }
    }
    return *legacy_name_;
  }

  void PopulateLegacyFrameState(ExplodedFrameState* frame_state) const {
    frame_state->target =
        base::NullableString16(base::UTF8ToUTF16(GetLegacyName()), false);
    frame_state->children.resize(children_.size());
    for (size_t i = 0; i < children_.size(); ++i)
      children_[i]->PopulateLegacyFrameState(&frame_state->children[i]);
  }

  void VerifyUpdatedFrameState(const ExplodedFrameState& frame_state) const {
    EXPECT_EQ(GetUniqueName(), base::UTF16ToUTF8(frame_state.target.string()));

    ASSERT_EQ(children_.size(), frame_state.children.size());
    for (size_t i = 0; i < children_.size(); ++i) {
      children_[i]->VerifyUpdatedFrameState(frame_state.children[i]);
    }
  }

 private:
  // Global toggle for the style of name to generate. Used to ensure that test
  // code can consistently trigger the legacy generation path when needed.
  static bool generate_legacy_name_;

  const std::string& GetNameForCurrentMode() const {
    return generate_legacy_name_ ? GetLegacyName() : GetUniqueName();
  }

  bool CheckUniqueness(const std::string& name) const {
    if (name == GetNameForCurrentMode())
      return false;
    for (auto* child : children_) {
      if (!child->CheckUniqueness(name))
        return false;
    }
    return true;
  }

  TestAdapter* const parent_;
  std::vector<TestAdapter*> children_;
  // The virtual index of this frame in the parent's list of children. Note that
  // this may differ from the actual index in |parent_->children_|.
  const int index_in_parent_;
  std::string requested_name_;
  UniqueNameHelper unique_name_helper_;
  mutable base::Optional<std::string> legacy_name_;
};

bool TestAdapter::generate_legacy_name_ = false;

void VerifyPageStateForTargetUpdate(const TestAdapter& main_frame) {
  ExplodedPageState in_state;
  main_frame.PopulateLegacyFrameState(&in_state.top);

  // Version 24 is the last version with unlimited unique names.
  std::string encoded_state;
  EncodePageStateForTest(in_state, 24, &encoded_state);

  ExplodedPageState out_state;
  DecodePageState(encoded_state, &out_state);

  main_frame.VerifyUpdatedFrameState(out_state.top);
}

TEST(UniqueNameHelper, Basic) {
  // Main frames should always have an empty unique name.
  TestAdapter main_frame(nullptr, -1, "my main frame");
  EXPECT_EQ("", main_frame.GetUniqueName());
  EXPECT_EQ("", main_frame.GetLegacyName());

  // A child frame with a requested name that is unique should use the requested
  // name.
  TestAdapter frame_0(&main_frame, 0, "child frame with name");
  EXPECT_EQ("child frame with name", frame_0.GetUniqueName());
  EXPECT_EQ("child frame with name", frame_0.GetLegacyName());

  // A child frame with no requested name should receive a generated unique
  // name.
  TestAdapter frame_7(&main_frame, 7, "");
  EXPECT_EQ("<!--framePath //<!--frame7-->-->", frame_7.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame7-->-->", frame_7.GetLegacyName());

  // Naming collision should force a fallback to using a generated unique name.
  TestAdapter frame_2(&main_frame, 2, "child frame with name");
  EXPECT_EQ("<!--framePath //<!--frame2-->-->", frame_2.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame2-->-->", frame_2.GetLegacyName());

  // Index collision should also force a fallback to using a generated unique
  // name.
  TestAdapter frame_2a(&main_frame, 2, "");
  EXPECT_EQ("<!--framePath //<!--frame2-->--><!--framePosition-2/0-->",
            frame_2a.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame2-->--><!--framePosition-2/0-->",
            frame_2a.GetLegacyName());

  // Index and name collision should also force a fallback to using a generated
  // unique name.
  TestAdapter frame_2b(&main_frame, 2, "child frame with name");
  EXPECT_EQ("<!--framePath //<!--frame2-->--><!--framePosition-2/1-->",
            frame_2b.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame2-->--><!--framePosition-2/1-->",
            frame_2b.GetLegacyName());

  VerifyPageStateForTargetUpdate(main_frame);
}

// Verify that basic frame path generation always includes the full path from
// the root when hashing is not triggered.
TEST(UniqueNameHelper, BasicGeneratedFramePath) {
  TestAdapter main_frame(nullptr, -1, "my main frame");
  EXPECT_EQ("", main_frame.GetUniqueName());
  EXPECT_EQ("", main_frame.GetLegacyName());

  TestAdapter frame_2(&main_frame, 2, "");
  EXPECT_EQ("<!--framePath //<!--frame2-->-->", frame_2.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame2-->-->", frame_2.GetLegacyName());

  TestAdapter frame_2_3(&frame_2, 3, "named grandchild");
  EXPECT_EQ("named grandchild", frame_2_3.GetUniqueName());
  EXPECT_EQ("named grandchild", frame_2_3.GetLegacyName());

  // Even though the parent frame has a unique name, the frame path should
  // include the full path from the root.
  TestAdapter frame_2_3_5(&frame_2_3, 5, "");
  EXPECT_EQ("<!--framePath //<!--frame2-->/named grandchild/<!--frame5-->-->",
            frame_2_3_5.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame2-->/named grandchild/<!--frame5-->-->",
            frame_2_3_5.GetLegacyName());

  // Make
  TestAdapter frame_2_3_5_7(&frame_2_3_5, 7, "");
  EXPECT_EQ(
      "<!--framePath //<!--frame2-->/named "
      "grandchild/<!--frame5-->/<!--frame7-->-->",
      frame_2_3_5_7.GetUniqueName());
  EXPECT_EQ(
      "<!--framePath //<!--frame2-->/named "
      "grandchild/<!--frame5-->/<!--frame7-->-->",
      frame_2_3_5_7.GetLegacyName());

  VerifyPageStateForTargetUpdate(main_frame);
}

TEST(UniqueNameHelper, Hashing) {
  TestAdapter main_frame(nullptr, -1, "my main frame");
  EXPECT_EQ("", main_frame.GetUniqueName());
  EXPECT_EQ("", main_frame.GetLegacyName());

  TestAdapter frame_0(&main_frame, 0, "");
  EXPECT_EQ("<!--framePath //<!--frame0-->-->", frame_0.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame0-->-->", frame_0.GetLegacyName());

  // A long requested name that's already unique should trigger hashing. The
  // frame path should not be included in this case.
  const std::string too_long_name(UniqueNameHelper::kMaxSize * 2, 'a');
  TestAdapter frame_0_0(&frame_0, 0, too_long_name);
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "2EDC986847E209B4016E141A6DC8716D3207350F416969382D431539BF292E4A-->-->",
      frame_0_0.GetUniqueName());
  EXPECT_EQ(too_long_name, frame_0_0.GetLegacyName());

  // A child of a frame with a hashed unique name should incorporate it as part
  // of the generated unique name. However, it should not incorporate any of the
  // frame path past the first hashed ancestor. This prevents having to consider
  // chains of hashed names when updating legacy names.
  TestAdapter frame_0_0_0(&frame_0_0, 0, "");
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "2EDC986847E209B4016E141A6DC8716D3207350F416969382D431539BF292E4A-->/<!--"
      "frame0-->-->",
      frame_0_0_0.GetUniqueName());
  // Though the legacy name should incorporate the full frame path.
  EXPECT_EQ(
      "<!--framePath //<!--frame0-->/" + too_long_name + "/<!--frame0-->-->",
      frame_0_0_0.GetLegacyName());

  // Nest one more time, to make sure that unique name updates correctly handle
  // nested hashing. Note that this name is intentionally choosen so that
  // |too_long_name| is a prefix.
  const std::string too_long_name_2(UniqueNameHelper::kMaxSize * 2 + 1, 'a');
  TestAdapter frame_0_0_0_0(&frame_0_0_0, 0, too_long_name_2);
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "4A82297889EB505CF6B5CBDF69977AFAB4632D6557539782F657BD7DC78091A5-->--"
      ">",
      frame_0_0_0_0.GetUniqueName());
  EXPECT_EQ(too_long_name_2, frame_0_0_0_0.GetLegacyName());

  TestAdapter frame_0_0_0_0_0(&frame_0_0_0_0, 0, "");
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "4A82297889EB505CF6B5CBDF69977AFAB4632D6557539782F657BD7DC78091A5-->/"
      "<!--frame0-->-->",
      frame_0_0_0_0_0.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame0-->/" + too_long_name +
                "/<!--frame0-->/" + too_long_name_2 + "/<!--frame0-->-->",
            frame_0_0_0_0_0.GetLegacyName());

  VerifyPageStateForTargetUpdate(main_frame);
}

TEST(UniqueNameHelper, GeneratedFramePathHashing) {
  TestAdapter main_frame(nullptr, -1, "my main frame");
  EXPECT_EQ("", main_frame.GetUniqueName());
  EXPECT_EQ("", main_frame.GetLegacyName());

  TestAdapter frame_0(&main_frame, 0, "");
  EXPECT_EQ("<!--framePath //<!--frame0-->-->", frame_0.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame0-->-->", frame_0.GetLegacyName());

  // Just under the limit, so the hashing fallback should not be triggered.
  const std::string just_fits_name(UniqueNameHelper::kMaxSize - 1, 'a');
  TestAdapter frame_0_0(&frame_0, 0, just_fits_name);
  EXPECT_EQ(just_fits_name, frame_0_0.GetUniqueName());
  EXPECT_EQ(just_fits_name, frame_0_0.GetLegacyName());

  // But the generated frame path for a subsequent child frame will force it
  // over the limit, trigger the hashing fallback.
  TestAdapter frame_0_0_0(&frame_0_0, 0, "");
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "56D999B69FA6734C5ED4B7C34CC994909AA7C5BF46D6A2B9E6357CB0911CD48C-->-->",
      frame_0_0_0.GetUniqueName());
  EXPECT_EQ(
      "<!--framePath "
      "//<!--frame0-->/" +
          just_fits_name + "/<!--frame0-->-->",
      frame_0_0_0.GetLegacyName());

  // Nest one more time, to make sure that unique name updates handle nested
  // hashing correctly. Note that this name is intentionally chosen to be a
  // prefix of |just_fits_name|.
  const std::string just_fits_name_2(UniqueNameHelper::kMaxSize - 2, 'a');
  TestAdapter frame_0_0_0_0(&frame_0_0_0, 0, just_fits_name_2);
  EXPECT_EQ(just_fits_name_2, frame_0_0_0_0.GetUniqueName());
  EXPECT_EQ(just_fits_name_2, frame_0_0_0_0.GetLegacyName());

  TestAdapter frame_0_0_0_0_0(&frame_0_0_0_0, 0, "");
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "9674397B6C53B4C9DA2D5E150FB090A75E6F89E18F2BD372851E6CC38A337060-->-->",
      frame_0_0_0_0_0.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame0-->/" + just_fits_name +
                "/<!--frame0-->/" + just_fits_name_2 + "/<!--frame0-->-->",
            frame_0_0_0_0_0.GetLegacyName());

  VerifyPageStateForTargetUpdate(main_frame);
}

}  // namespace
}  // namespace content
