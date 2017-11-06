// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_EXPERIMENTAL_OBSERVER_H_
#define CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_EXPERIMENTAL_OBSERVER_H_

class TabStripModelExperimentalObserver {
 public:
  // If the group is null the index is into the toplevel tab strip.
  void TabInsertedAt(Group* group, int index);
  void TabClosedAt(Group* group, int index);

  /* TODO(brettw) add group notifications. Some initial thinking:
  struct GroupCreateParams {
    Group* group;

    // This is the range of existing tabs in the toplevel tab strip that are
    // incorporated into the group. These are effectively removed from the
    // toplevel and inserted into the group without changing them.
    //
    // The range is, like STL, [begin, end) and it may be empty.
    int incorporate_begin;
    int incorporate_end;

    // These are indicies of new tabs that are added inside the group.
    std::vector<int> added_indices;
  };
  void GroupCreatedAt(const GroupCreateParams& params);

  void GroupDestroyedAt(int index);
  */

 private:
};

#endif  // CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_EXPERIMENTAL_OBSERVER_H_
