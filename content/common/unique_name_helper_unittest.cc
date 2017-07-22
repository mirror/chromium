// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/unique_name_helper.h"

#include <map>
#include <memory>
#include <vector>

#include "base/auto_reset.h"
#include "base/memory/ptr_util.h"
#include "base/optional.h"
#include "base/strings/nullable_string16.h"
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "content/common/page_state_serialization.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace {

class TestFrameAdapter : public UniqueNameHelper::FrameAdapter {
 public:
  // |virtual_index_in_parent| is the virtual index of this frame in the
  // parent's list of children, as unique name generation should see it. Note
  // that this may differ from the actual index of this adapter in
  // |parent_->children_|.
  explicit TestFrameAdapter(TestFrameAdapter* parent,
                            int virtual_index_in_parent,
                            const std::string& requested_name)
      : parent_(parent),
        virtual_index_in_parent_(virtual_index_in_parent),
        unique_name_helper_(this) {
    if (parent_)
      parent_->children_.push_back(this);
    unique_name_helper_.UpdateName(requested_name);
    CalculateLegacyName(requested_name);
  }

  ~TestFrameAdapter() override {
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

  int GetSiblingCount() const override { return virtual_index_in_parent_; }

  int GetChildCount() const override {
    ADD_FAILURE()
        << "GetChildCount() should not be triggered by unit test code!";
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
      result.push_back(adapter->virtual_index_in_parent_);
    return result;
  }

  // Returns the new style name with a max size limit.
  const std::string& GetUniqueName() const {
    return unique_name_helper_.value();
  }

  // Calculate and return the legacy style name with no max size limit.
  const std::string& GetLegacyName() const { return legacy_name_; }

  // Populate a tree of FrameState with legacy unique names. The order of
  // FrameState children is guaranteed to match the order of TestFrameAdapter
  // children.
  void PopulateLegacyFrameState(ExplodedFrameState* frame_state) const {
    frame_state->target =
        base::NullableString16(base::UTF8ToUTF16(GetLegacyName()), false);
    frame_state->children.resize(children_.size());
    for (size_t i = 0; i < children_.size(); ++i)
      children_[i]->PopulateLegacyFrameState(&frame_state->children[i]);
  }

  // Recursively verify that FrameState and its children have matching unique
  // names to this TestFrameAdapter.
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

  void CalculateLegacyName(const std::string& requested_name) {
    // Manually skip the main frame so its legacy name is always empty: this
    // is needed in the test as that logic lives at a different layer in
    // UniqueNameHelper.
    if (!IsMainFrame()) {
      base::AutoReset<bool> enable_legacy_mode(&generate_legacy_name_, true);
      legacy_name_ =
          UniqueNameHelper::CalculateLegacyNameForTesting(this, requested_name);
    }
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

  TestFrameAdapter* const parent_;
  std::vector<TestFrameAdapter*> children_;
  const int virtual_index_in_parent_;
  UniqueNameHelper unique_name_helper_;
  std::string legacy_name_;
};

bool TestFrameAdapter::generate_legacy_name_ = false;

// Test helper that verifies that legacy unique names in versions of PageState
// prior to 25 are correctly updated when deserialized.
void VerifyPageStateForTargetUpdate(const TestFrameAdapter& main_frame) {
  ExplodedPageState in_state;
  main_frame.PopulateLegacyFrameState(&in_state.top);

  // Version 24 is the last version with unlimited size unique names.
  std::string encoded_state;
  EncodePageStateForTesting(in_state, 24, &encoded_state);

  ExplodedPageState out_state;
  DecodePageState(encoded_state, &out_state);

  main_frame.VerifyUpdatedFrameState(out_state.top);
}

// Simple holder for TreeFrameAdapter nodes that compromise a linear tree, i.e.
// each node only has one child. frames.front() represents the root of the tree,
// while frames.back() represents the deepest node in the tree.
//
// This holder class is needed because destruction order of a std::vector's
// elements is undefined. |~LinearTree()| ensures there are no dangling pointers
// by deleting from the deepest node up to the root.
struct LinearTree {
  LinearTree() = default;
  LinearTree(LinearTree&&) = default;
  LinearTree& operator=(LinearTree&&) = default;

  ~LinearTree() {
    while (!frames.empty())
      frames.pop_back();
  }

  std::vector<std::unique_ptr<TestFrameAdapter>> frames;
};

LinearTree CreateLinearTree() {
  LinearTree tree;
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(nullptr, -1, "my main frame"));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 1, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 2, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 3, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 4, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 5, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 6, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 7, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 8, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 9, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 10, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 11, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 12, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 13, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 14, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 15, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 16, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 17, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 18, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 19, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 20, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 21, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 22, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 23, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 24, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 25, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 26, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 27, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 28, ""));
  tree.frames.emplace_back(
      base::MakeUnique<TestFrameAdapter>(tree.frames.back().get(), 29, ""));
  return tree;
}

TEST(UniqueNameHelper, Basic) {
  // Main frames should always have an empty unique name.
  TestFrameAdapter main_frame(nullptr, -1, "my main frame");
  EXPECT_EQ("", main_frame.GetUniqueName());
  EXPECT_EQ("", main_frame.GetLegacyName());

  // A child frame with a requested name that is unique should use the requested
  // name.
  TestFrameAdapter frame_0(&main_frame, 0, "child frame with name");
  EXPECT_EQ("child frame with name", frame_0.GetUniqueName());
  EXPECT_EQ("child frame with name", frame_0.GetLegacyName());

  // A child frame with no requested name should receive a generated unique
  // name.
  TestFrameAdapter frame_7(&main_frame, 7, "");
  EXPECT_EQ("<!--framePath //<!--frame7-->-->", frame_7.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame7-->-->", frame_7.GetLegacyName());

  // Naming collision should force a fallback to using a generated unique name.
  TestFrameAdapter frame_2(&main_frame, 2, "child frame with name");
  EXPECT_EQ("<!--framePath //<!--frame2-->-->", frame_2.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame2-->-->", frame_2.GetLegacyName());

  // Index collision should also force a fallback to using a generated unique
  // name.
  TestFrameAdapter frame_2a(&main_frame, 2, "");
  EXPECT_EQ("<!--framePath //<!--frame2-->--><!--framePosition-2/0-->",
            frame_2a.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame2-->--><!--framePosition-2/0-->",
            frame_2a.GetLegacyName());

  // A child of a frame with a unique naming collision will incorporate the
  // frame position marker as part of its frame path, though it will look a bit
  // strange...
  TestFrameAdapter frame_2a_5(&frame_2a, 5, "");
  EXPECT_EQ(
      "<!--framePath //<!--frame2-->--><!--framePosition-2/0/<!--frame5-->-->",
      frame_2a_5.GetUniqueName());
  EXPECT_EQ(
      "<!--framePath //<!--frame2-->--><!--framePosition-2/0/<!--frame5-->-->",
      frame_2a_5.GetLegacyName());

  // Index and name collision should also force a fallback to using a generated
  // unique name.
  TestFrameAdapter frame_2b(&main_frame, 2, "child frame with name");
  EXPECT_EQ("<!--framePath //<!--frame2-->--><!--framePosition-2/1-->",
            frame_2b.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame2-->--><!--framePosition-2/1-->",
            frame_2b.GetLegacyName());

  VerifyPageStateForTargetUpdate(main_frame);
}

// Verify that basic frame path generation always includes the full path from
// the root when hashing is not triggered.
TEST(UniqueNameHelper, BasicGeneratedFramePath) {
  TestFrameAdapter main_frame(nullptr, -1, "my main frame");
  EXPECT_EQ("", main_frame.GetUniqueName());
  EXPECT_EQ("", main_frame.GetLegacyName());

  TestFrameAdapter frame_2(&main_frame, 2, "");
  EXPECT_EQ("<!--framePath //<!--frame2-->-->", frame_2.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame2-->-->", frame_2.GetLegacyName());

  TestFrameAdapter frame_2_3(&frame_2, 3, "named grandchild");
  EXPECT_EQ("named grandchild", frame_2_3.GetUniqueName());
  EXPECT_EQ("named grandchild", frame_2_3.GetLegacyName());

  // Even though the parent frame has a unique name, the frame path should
  // include the full path from the root.
  TestFrameAdapter frame_2_3_5(&frame_2_3, 5, "");
  EXPECT_EQ("<!--framePath //<!--frame2-->/named grandchild/<!--frame5-->-->",
            frame_2_3_5.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame2-->/named grandchild/<!--frame5-->-->",
            frame_2_3_5.GetLegacyName());

  VerifyPageStateForTargetUpdate(main_frame);
}

TEST(UniqueNameHelper, Hashing) {
  TestFrameAdapter main_frame(nullptr, -1, "my main frame");
  EXPECT_EQ("", main_frame.GetUniqueName());
  EXPECT_EQ("", main_frame.GetLegacyName());

  TestFrameAdapter frame_0(&main_frame, 0, "");
  EXPECT_EQ("<!--framePath //<!--frame0-->-->", frame_0.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame0-->-->", frame_0.GetLegacyName());

  // A long requested name that's already unique should trigger hashing. The
  // frame path should not be included in this case.
  const std::string too_long_name(UniqueNameHelper::kMaxSize * 2, 'a');
  TestFrameAdapter frame_0_0(&frame_0, 0, too_long_name);
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "2EDC986847E209B4016E141A6DC8716D3207350F416969382D431539BF292E4A-->-->",
      frame_0_0.GetUniqueName());
  EXPECT_EQ(too_long_name, frame_0_0.GetLegacyName());

  // A child of a frame with a hashed unique name should incorporate it as part
  // of the generated unique name. However, it should not incorporate any of the
  // frame path past the first hashed ancestor. This prevents having to consider
  // chains of hashed names when updating legacy names.
  TestFrameAdapter frame_0_0_0(&frame_0_0, 0, "");
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "2EDC986847E209B4016E141A6DC8716D3207350F416969382D431539BF292E4A-->/<!--"
      "frame0-->-->",
      frame_0_0_0.GetUniqueName());
  // Though the legacy name should incorporate the full frame path.
  EXPECT_EQ(
      "<!--framePath //<!--frame0-->/" + too_long_name + "/<!--frame0-->-->",
      frame_0_0_0.GetLegacyName());

  // Intentionally collide the frame index to make sure frame position markers
  // are preserved.
  TestFrameAdapter frame_0_0_0b(&frame_0_0, 0, "");
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "2EDC986847E209B4016E141A6DC8716D3207350F416969382D431539BF292E4A-->/"
      "<!--frame0-->--><!--framePosition-0-0-0/0-->",
      frame_0_0_0b.GetUniqueName());
  // Though the legacy name should incorporate the full frame path.
  EXPECT_EQ("<!--framePath //<!--frame0-->/" + too_long_name +
                "/<!--frame0-->--><!--framePosition-0-0-0/0-->",
            frame_0_0_0b.GetLegacyName());

  // Ensure that a legacy name with no unique suffix of its own (but with a
  // unique suffix embedded in the frame path) is handled correctly.
  // in the frame path.
  TestFrameAdapter frame_0_0_0b_0(&frame_0_0_0b, 0, "");
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "2EDC986847E209B4016E141A6DC8716D3207350F416969382D431539BF292E4A-->/"
      "<!--frame0-->--><!--framePosition-0-0-0/0/<!--frame0-->-->",
      frame_0_0_0b_0.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame0-->/" + too_long_name +
                "/<!--frame0-->--><!--framePosition-0-0-0/0/<!--frame0-->-->",
            frame_0_0_0b_0.GetLegacyName());

  // Ensure that a legacy name with a unique suffix of its own (and with a
  // unique suffix embedded in the frame path) is handled correctly.
  TestFrameAdapter frame_0_0_0b_0b(&frame_0_0_0b, 0, "");
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "2EDC986847E209B4016E141A6DC8716D3207350F416969382D431539BF292E4A-->/"
      "<!--frame0-->--><!--framePosition-0-0-0/0/"
      "<!--frame0-->--><!--framePosition-0-0-0-0/0-->",
      frame_0_0_0b_0b.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame0-->/" + too_long_name +
                "/<!--frame0-->--><!--framePosition-0-0-0/0/"
                "<!--frame0-->--><!--framePosition-0-0-0-0/0-->",
            frame_0_0_0b_0b.GetLegacyName());

  // Nest one more time, to make sure that unique name updates correctly handle
  // nested hashing. Note that this name is intentionally choosen so that
  // |too_long_name| is a prefix.
  const std::string too_long_name_2(UniqueNameHelper::kMaxSize * 2 + 1, 'a');
  TestFrameAdapter frame_0_0_0_0(&frame_0_0_0, 0, too_long_name_2);
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "4A82297889EB505CF6B5CBDF69977AFAB4632D6557539782F657BD7DC78091A5-->--"
      ">",
      frame_0_0_0_0.GetUniqueName());
  EXPECT_EQ(too_long_name_2, frame_0_0_0_0.GetLegacyName());

  TestFrameAdapter frame_0_0_0_0_0(&frame_0_0_0_0, 0, "");
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
  TestFrameAdapter main_frame(nullptr, -1, "my main frame");
  EXPECT_EQ("", main_frame.GetUniqueName());
  EXPECT_EQ("", main_frame.GetLegacyName());

  TestFrameAdapter frame_0(&main_frame, 0, "");
  EXPECT_EQ("<!--framePath //<!--frame0-->-->", frame_0.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame0-->-->", frame_0.GetLegacyName());

  // Just under the limit, so the hashing fallback should not be triggered.
  const std::string just_fits_name(UniqueNameHelper::kMaxSize - 1, 'a');
  TestFrameAdapter frame_0_0(&frame_0, 0, just_fits_name);
  EXPECT_EQ(just_fits_name, frame_0_0.GetUniqueName());
  EXPECT_EQ(just_fits_name, frame_0_0.GetLegacyName());

  // But the generated frame path for a subsequent child frame will force it
  // over the limit, trigger the hashing fallback.
  TestFrameAdapter frame_0_0_0(&frame_0_0, 0, "");
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "56D999B69FA6734C5ED4B7C34CC994909AA7C5BF46D6A2B9E6357CB0911CD48C-->-->",
      frame_0_0_0.GetUniqueName());
  EXPECT_EQ(
      "<!--framePath "
      "//<!--frame0-->/" +
          just_fits_name + "/<!--frame0-->-->",
      frame_0_0_0.GetLegacyName());

  // Intentionally collide the frame index to make sure frame position markers
  // are preserved.
  TestFrameAdapter frame_0_0_0b(&frame_0_0, 0, "");
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "56D999B69FA6734C5ED4B7C34CC994909AA7C5BF46D6A2B9E6357CB0911CD48C-->--><!"
      "--framePosition-0-0-0/0-->",
      frame_0_0_0b.GetUniqueName());
  // Though the legacy name should incorporate the full frame path.
  EXPECT_EQ("<!--framePath //<!--frame0-->/" + just_fits_name +
                "/<!--frame0-->--><!--framePosition-0-0-0/0-->",
            frame_0_0_0b.GetLegacyName());

  // Ensure that a legacy name with no unique suffix of its own (but with a
  // unique suffix embedded in the frame path) is handled correctly.
  // in the frame path.
  TestFrameAdapter frame_0_0_0b_0(&frame_0_0_0b, 0, "");
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "56D999B69FA6734C5ED4B7C34CC994909AA7C5BF46D6A2B9E6357CB0911CD48C-->--><!"
      "--framePosition-0-0-0/0/<!--frame0-->-->",
      frame_0_0_0b_0.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame0-->/" + just_fits_name +
                "/<!--frame0-->--><!--framePosition-0-0-0/0/<!--frame0-->-->",
            frame_0_0_0b_0.GetLegacyName());

  // Ensure that a legacy name with a unique suffix of its own (and with a
  // unique suffix embedded in the frame path) is handled correctly.
  TestFrameAdapter frame_0_0_0b_0b(&frame_0_0_0b, 0, "");
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "56D999B69FA6734C5ED4B7C34CC994909AA7C5BF46D6A2B9E6357CB0911CD48C-->--><!"
      "--framePosition-0-0-0/0/<!--frame0-->--><!--framePosition-0-0-0-0/0-->",
      frame_0_0_0b_0b.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame0-->/" + just_fits_name +
                "/<!--frame0-->--><!--framePosition-0-0-0/0/"
                "<!--frame0-->--><!--framePosition-0-0-0-0/0-->",
            frame_0_0_0b_0b.GetLegacyName());

  // Nest one more time, to make sure that unique name updates handle nested
  // hashing correctly. Note that this name is intentionally chosen to be a
  // prefix of |just_fits_name|.
  const std::string just_fits_name_2(UniqueNameHelper::kMaxSize - 2, 'a');
  TestFrameAdapter frame_0_0_0_0(&frame_0_0_0, 0, just_fits_name_2);
  EXPECT_EQ(just_fits_name_2, frame_0_0_0_0.GetUniqueName());
  EXPECT_EQ(just_fits_name_2, frame_0_0_0_0.GetLegacyName());

  TestFrameAdapter frame_0_0_0_0_0(&frame_0_0_0_0, 0, "");
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "9674397B6C53B4C9DA2D5E150FB090A75E6F89E18F2BD372851E6CC38A337060-->-->",
      frame_0_0_0_0_0.GetUniqueName());
  EXPECT_EQ("<!--framePath //<!--frame0-->/" + just_fits_name +
                "/<!--frame0-->/" + just_fits_name_2 + "/<!--frame0-->-->",
            frame_0_0_0_0_0.GetLegacyName());

  VerifyPageStateForTargetUpdate(main_frame);
}

TEST(UniqueNameHelper, UniqueSuffix) {
  LinearTree tree = CreateLinearTree();
  auto& frames = tree.frames;

  // Create an index collision with frames[12].
  TestFrameAdapter unhashed_conflicting_frame(frames[11].get(), 12, "");
  // Unique suffix trimming shouldn't be used when the name is not hashed: the
  // generated unique suffix should include the full path of frame indices and
  // not be limited to only 10.
  EXPECT_EQ(
      "<!--framePath "
      "//<!--frame1-->/<!--frame2-->/<!--frame3-->/<!--frame4-->/<!--frame5-->/"
      "<!--frame6-->/<!--frame7-->/<!--frame8-->/<!--frame9-->/<!--frame10-->/"
      "<!--frame11-->/"
      "<!--frame12-->--><!--framePosition-12-11-10-9-8-7-6-5-4-3-2-1/0-->",
      unhashed_conflicting_frame.GetUniqueName());
  EXPECT_EQ(unhashed_conflicting_frame.GetUniqueName(),
            unhashed_conflicting_frame.GetLegacyName());

  // Test behavior at the boundary where the unique name transitions to a hashed
  // name.

  // Two tests right before the transition to a hashed unique name. The unique
  // suffix should not be trimmed.
  // Create an index collision with frames[26].
  TestFrameAdapter unhashed_conflicting_frame_2(frames[25].get(), 26, "");
  EXPECT_EQ(
      "<!--framePath "
      "//<!--frame1-->/<!--frame2-->/<!--frame3-->/<!--frame4-->/<!--frame5-->/"
      "<!--frame6-->/<!--frame7-->/<!--frame8-->/<!--frame9-->/<!--frame10-->/"
      "<!--frame11-->/<!--frame12-->/<!--frame13-->/<!--frame14-->/"
      "<!--frame15-->/<!--frame16-->/<!--frame17-->/<!--frame18-->/"
      "<!--frame19-->/<!--frame20-->/<!--frame21-->/<!--frame22-->/"
      "<!--frame23-->/<!--frame24-->/<!--frame25-->/"
      "<!--frame26-->--><!--framePosition-26-25-24-23-22-21-20-19-18-17-16-15-"
      "14-13-12-11-10-9-8-7-6-5-4-3-2-1/0-->",
      unhashed_conflicting_frame_2.GetUniqueName());
  EXPECT_EQ(unhashed_conflicting_frame_2.GetUniqueName(),
            unhashed_conflicting_frame_2.GetLegacyName());
  // Create an index collision with frames[27].
  TestFrameAdapter unhashed_conflicting_frame_3(frames[26].get(), 27, "");
  EXPECT_EQ(
      "<!--framePath "
      "//<!--frame1-->/<!--frame2-->/<!--frame3-->/<!--frame4-->/<!--frame5-->/"
      "<!--frame6-->/<!--frame7-->/<!--frame8-->/<!--frame9-->/<!--frame10-->/"
      "<!--frame11-->/<!--frame12-->/<!--frame13-->/<!--frame14-->/"
      "<!--frame15-->/<!--frame16-->/<!--frame17-->/<!--frame18-->/"
      "<!--frame19-->/<!--frame20-->/<!--frame21-->/<!--frame22-->/"
      "<!--frame23-->/<!--frame24-->/<!--frame25-->/<!--frame26-->/"
      "<!--frame27-->"
      "--><!--framePosition-27-26-25-24-23-22-21-20-19-"
      "18-17-16-15-14-13-12-11-10-9-8-7-6-5-4-3-2-1/0-->",
      unhashed_conflicting_frame_3.GetUniqueName());
  EXPECT_EQ(unhashed_conflicting_frame_3.GetUniqueName(),
            unhashed_conflicting_frame_3.GetLegacyName());

  // Two tests after the transition to a hashed unique name. The legacy unique
  // name should contain the full list of frame indices, while the hashed unique
  // name should be limited to 10.
  // Create an index collision with frames[28].
  TestFrameAdapter hashed_conflicting_frame(frames[27].get(), 28, "");
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "7B8BD7AD1163EF2FA14D299769A779DE7F11CB275FDAF81B54FBB89D3D88B640-->--><!"
      "--framePosition-28-27-26-25-24-23-22-21-20-19/0-->",
      hashed_conflicting_frame.GetUniqueName());
  EXPECT_EQ(
      "<!--framePath "
      "//<!--frame1-->/<!--frame2-->/<!--frame3-->/<!--frame4-->/<!--frame5-->/"
      "<!--frame6-->/<!--frame7-->/<!--frame8-->/<!--frame9-->/<!--frame10-->/"
      "<!--frame11-->/<!--frame12-->/<!--frame13-->/<!--frame14-->/"
      "<!--frame15-->/<!--frame16-->/<!--frame17-->/<!--frame18-->/"
      "<!--frame19-->/<!--frame20-->/<!--frame21-->/<!--frame22-->/"
      "<!--frame23-->/<!--frame24-->/<!--frame25-->/<!--frame26-->/"
      "<!--frame27-->/"
      "<!--frame28-->--><!--framePosition-28-27-26-25-24-23-22-21-20-19-18-17-"
      "16-15-14-13-12-11-10-9-8-7-6-5-4-3-2-1/0-->",
      hashed_conflicting_frame.GetLegacyName());

  // Create an index collision with frames[29].
  TestFrameAdapter hashed_conflicting_frame_2(frames[28].get(), 29, "");
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "05D4EFE7545F4FD3201894C0076AC3D97F2F1A42F8D0FAA25E5C91F2E0364BFB-->--><!"
      "--framePosition-29-28-27-26-25-24-23-22-21-20/0-->",
      hashed_conflicting_frame_2.GetUniqueName());
  EXPECT_EQ(
      "<!--framePath "
      "//<!--frame1-->/<!--frame2-->/<!--frame3-->/<!--frame4-->/<!--frame5-->/"
      "<!--frame6-->/<!--frame7-->/<!--frame8-->/<!--frame9-->/<!--frame10-->/"
      "<!--frame11-->/<!--frame12-->/<!--frame13-->/<!--frame14-->/"
      "<!--frame15-->/<!--frame16-->/<!--frame17-->/<!--frame18-->/"
      "<!--frame19-->/<!--frame20-->/<!--frame21-->/<!--frame22-->/"
      "<!--frame23-->/<!--frame24-->/<!--frame25-->/<!--frame26-->/"
      "<!--frame27-->/<!--frame28-->/"
      "<!--frame29-->--><!--framePosition-29-28-27-26-25-24-23-22-21-20-19-18-"
      "17-16-15-14-13-12-11-10-9-8-7-6-5-4-3-2-1/0-->",
      hashed_conflicting_frame_2.GetLegacyName());

  VerifyPageStateForTargetUpdate(*frames.front());
}

TEST(UniqueNameHelper, UniqueSuffixCollisionAfterHash) {
  LinearTree tree = CreateLinearTree();
  auto& frames = tree.frames;

  // Malicious frame that has a name that intentionally collides with the output
  // from generating a hashed unique name.
  TestFrameAdapter malicious_frame(
      frames[0].get(), 137,
      "<!--framePath //<!--frameHash "
      "05D4EFE7545F4FD3201894C0076AC3D97F2F1A42F8D0FAA25E5C91F2E0364BFB-->--><!"
      "--framePosition-29-28-27-26-25-24-23-22-21-20/0-->");

  // Index collision with frames[29] and hashed unique name collision with
  // |malicious_frame|. The resulting name should have no unique suffix
  // appended, since the generation algorithm should notice that just the hash
  // frame path is sufficient for uniqueness.
  TestFrameAdapter frame(frames[28].get(), 29, "");
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "05D4EFE7545F4FD3201894C0076AC3D97F2F1A42F8D0FAA25E5C91F2E0364BFB-->-->",
      frame.GetUniqueName());

  // Another index collision with frames[29] and hashed unique name collision
  // with |malicious_frame|. This time, the resulting name should have a unique
  // suffix that disambiguates it from the unique names of both |frame| and
  // |malicious_frame|.
  TestFrameAdapter frame_2(frames[28].get(), 29, "");
  EXPECT_EQ(
      "<!--framePath //<!--frameHash "
      "05D4EFE7545F4FD3201894C0076AC3D97F2F1A42F8D0FAA25E5C91F2E0364BFB-->--><!"
      "--framePosition-29-28-27-26-25-24-23-22-21-20/1-->",
      frame_2.GetUniqueName());

  // Note: this test intentionally does not verify page state or check the
  // legacy name. As the names would not collide in the same way, this is
  // necessarily an instance where the legacy and new unique name generation
  // would differ.
}

}  // namespace
}  // namespace content
