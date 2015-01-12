// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ENHANCED_BOOKMARKS_ENHANCED_BOOKMARK_UTILS_H_
#define COMPONENTS_ENHANCED_BOOKMARKS_ENHANCED_BOOKMARK_UTILS_H_

#include <set>
#include <string>
#include <vector>

class BookmarkNode;

namespace bookmarks {
class BookmarkModel;
}

namespace enhanced_bookmarks {

// The vector is sorted in place.
// All of the bookmarks in |nodes| must be urls.
void SortBookmarksByName(std::vector<const BookmarkNode*>& nodes);

// Returns the permanent nodes whose url children are considered uncategorized
// and whose folder children should be shown in the bookmark menu.
// |model| must be loaded.
std::vector<const BookmarkNode*> PrimaryPermanentNodes(
    bookmarks::BookmarkModel* model);

// Returns an unsorted vector of folders that are considered to be at the "root"
// level of the bookmark hierarchy. Functionally, this means all direct
// descendants of PrimaryPermanentNodes, and possibly a node for the bookmarks
// bar.
std::vector<const BookmarkNode*> RootLevelFolders(
    bookmarks::BookmarkModel* model);

// Returns whether |node| is a primary permanent node in the sense of
// |PrimaryPermanentNodes|.
bool IsPrimaryPermanentNode(const BookmarkNode* node,
                            bookmarks::BookmarkModel* model);

// Returns the root level folder in which this node is directly, or indirectly
// via subfolders, located.
const BookmarkNode* RootLevelFolderForNode(const BookmarkNode* node,
                                           bookmarks::BookmarkModel* model);

}  // namespace enhanced_bookmarks

#endif  // COMPONENTS_ENHANCED_BOOKMARKS_ENHANCED_BOOKMARK_UTILS_H_
