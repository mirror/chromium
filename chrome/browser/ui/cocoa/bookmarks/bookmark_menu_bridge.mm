// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <AppKit/AppKit.h>

#include "base/strings/sys_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#import "chrome/browser/app_controller_mac.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/bookmarks/managed_bookmark_service_factory.h"
#include "chrome/browser/prefs/incognito_mode_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/bookmarks/bookmark_utils_desktop.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/cocoa/bookmarks/bookmark_menu_bridge.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_menu_cocoa_controller.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/managed/managed_bookmark_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"
#include "ui/resources/grit/ui_resources.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

BookmarkMenuBridge::BookmarkMenuBridge(Profile* profile)
    : profile_(profile),
      controller_([[BookmarkMenuCocoaController alloc] initWithBridge:this]) {
  if (GetBookmarkModel())
    ObserveBookmarkModel();
}

BookmarkMenuBridge::~BookmarkMenuBridge() {
  AttachToMenu(nil);
  BookmarkModel* model = GetBookmarkModel();
  if (model)
    model->RemoveObserver(this);
}

void BookmarkMenuBridge::AttachToMenu(NSMenu* menu) {
  [menu_root_ setDelegate:nil];
  menu_root_.reset([menu retain]);
  if (!menu_root_)
    return;

  [menu_root_ setDelegate:controller_];
  BuildRootMenu();
}

void BookmarkMenuBridge::BookmarkModelLoaded(BookmarkModel* model,
                                             bool ids_reassigned) {
  InvalidateMenu();
}

void BookmarkMenuBridge::UpdateMenu(NSMenu* bookmark_menu,
                                    const BookmarkNode* node) {
  DCHECK(bookmark_menu);
  DCHECK(controller_);
  DCHECK_EQ([bookmark_menu delegate], controller_.get());

  if (bookmark_menu == menu_root_) {
    if (!IsMenuValid())
      BuildRootMenu();
    return;
  }

  AddNodeToMenu(node, bookmark_menu);
  // Clear the delegate to prevent further refreshes.
  [bookmark_menu setDelegate:nil];
}

void BookmarkMenuBridge::BuildRootMenu() {
  BookmarkModel* model = GetBookmarkModel();
  if (!model || !model->loaded())
    return;

  if (!folder_image_) {
    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    folder_image_.reset(
        rb.GetNativeImageNamed(IDR_BOOKMARK_BAR_FOLDER).CopyNSImage());
    [folder_image_ setTemplate:YES];
  }

  ClearBookmarkMenu();

  // Add at most one separator for the bookmark bar and the managed bookmarks
  // folder.
  bookmarks::ManagedBookmarkService* managed =
      ManagedBookmarkServiceFactory::GetForProfile(profile_);
  const BookmarkNode* barNode = model->bookmark_bar_node();
  const BookmarkNode* managedNode = managed->managed_node();
  if (!barNode->empty() || !managedNode->empty())
    [menu_root_ addItem:[NSMenuItem separatorItem]];
  if (!managedNode->empty()) {
    // Most users never see this node, so the image is only loaded if needed.
    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    NSImage* image =
        rb.GetNativeImageNamed(IDR_BOOKMARK_BAR_FOLDER_MANAGED).ToNSImage();
    AddNodeAsSubmenu(menu_root_, managedNode, image);
  }
  if (!barNode->empty())
    AddNodeToMenu(barNode, menu_root_);

  // If the "Other Bookmarks" folder has any content, make a submenu for it and
  // fill it in.
  if (!model->other_node()->empty()) {
    [menu_root_ addItem:[NSMenuItem separatorItem]];
    AddNodeAsSubmenu(menu_root_, model->other_node(), folder_image_);
  }

  // If the "Mobile Bookmarks" folder has any content, make a submenu for it and
  // fill it in.
  if (!model->mobile_node()->empty()) {
    // Add a separator if we did not already add one due to a non-empty
    // "Other Bookmarks" folder.
    if (model->other_node()->empty())
      [menu_root_ addItem:[NSMenuItem separatorItem]];

    AddNodeAsSubmenu(menu_root_, model->mobile_node(), folder_image_);
  }
}

void BookmarkMenuBridge::BookmarkModelBeingDeleted(BookmarkModel* model) {
  InvalidateMenu();
  ClearBookmarkMenu();
}

void BookmarkMenuBridge::BookmarkNodeMoved(BookmarkModel* model,
                                           const BookmarkNode* old_parent,
                                           int old_index,
                                           const BookmarkNode* new_parent,
                                           int new_index) {
  InvalidateMenu();
}

void BookmarkMenuBridge::BookmarkNodeAdded(BookmarkModel* model,
                                           const BookmarkNode* parent,
                                           int index) {
  InvalidateMenu();
}

void BookmarkMenuBridge::BookmarkNodeRemoved(
    BookmarkModel* model,
    const BookmarkNode* parent,
    int old_index,
    const BookmarkNode* node,
    const std::set<GURL>& removed_urls) {
  InvalidateMenu();
}

void BookmarkMenuBridge::BookmarkAllUserNodesRemoved(
    BookmarkModel* model,
    const std::set<GURL>& removed_urls) {
  InvalidateMenu();
}

void BookmarkMenuBridge::BookmarkNodeChanged(BookmarkModel* model,
                                             const BookmarkNode* node) {
  NSMenuItem* item = MenuItemForNode(node);
  if (item)
    ConfigureMenuItem(node, item, true);
}

void BookmarkMenuBridge::BookmarkNodeFaviconChanged(BookmarkModel* model,
                                                    const BookmarkNode* node) {
  NSMenuItem* item = MenuItemForNode(node);
  if (item)
    ConfigureMenuItem(node, item, false);
}

void BookmarkMenuBridge::BookmarkNodeChildrenReordered(
    BookmarkModel* model, const BookmarkNode* node) {
  InvalidateMenu();
}

void BookmarkMenuBridge::ResetMenu() {
  ClearBookmarkMenu();
}

void BookmarkMenuBridge::BuildMenu() {
  UpdateMenu(menu_root_, nullptr);
}

// Watch for changes.
void BookmarkMenuBridge::ObserveBookmarkModel() {
  BookmarkModel* model = GetBookmarkModel();
  model->AddObserver(this);
  if (model->loaded())
    BookmarkModelLoaded(model, false);
}

BookmarkModel* BookmarkMenuBridge::GetBookmarkModel() {
  if (!profile_)
    return nullptr;
  return BookmarkModelFactory::GetForBrowserContext(profile_);
}

Profile* BookmarkMenuBridge::GetProfile() {
  return profile_;
}

NSMenu* BookmarkMenuBridge::BookmarkMenu() {
  return menu_root_;
}

void BookmarkMenuBridge::ClearBookmarkMenu() {
  if (!menu_root_)
    return;

  // Recursively delete all menus that look like a bookmark. Also delete all
  // separator items since we explicitly add them back in. This deletes
  // everything except the first item ("Add Bookmark...").
  NSArray* items = [menu_root_ itemArray];
  for (NSMenuItem* item in items) {
    // Convention: items in the bookmark list which are bookmarks have
    // an action of openBookmarkMenuItem:.  Also, assume all items
    // with submenus are submenus of bookmarks.
    if (([item action] == @selector(openBookmarkMenuItem:)) ||
        [item hasSubmenu] ||
        [item isSeparatorItem]) {
      // This will eventually [obj release] all its kids, if it has
      // any.
      [menu_root_ removeItem:item];
    } else {
      // Leave it alone.
    }
  }
}

void BookmarkMenuBridge::InvalidateMenu() {
  bookmark_nodes_.clear();
}

bool BookmarkMenuBridge::IsMenuValid() const {
  // There can be no bookmark nodes if the user has never added a bookmark. But
  // there's not much to cache then, so just say it's always invalid.
  return !bookmark_nodes_.empty();
}

void BookmarkMenuBridge::AddNodeAsSubmenu(NSMenu* menu,
                                          const BookmarkNode* node,
                                          NSImage* image) {
  NSString* title = base::SysUTF16ToNSString(node->GetTitle());
  NSMenuItem* items = [[[NSMenuItem alloc]
                            initWithTitle:title
                                   action:nil
                            keyEquivalent:@""] autorelease];
  [items setImage:image];
  [items setTag:node->id()];
  [menu addItem:items];
  NSMenu* submenu = [[[NSMenu alloc] initWithTitle:title] autorelease];
  [menu setSubmenu:submenu forItem:items];
  [submenu setDelegate:controller_];
}

// TODO(jrg): limit the number of bookmarks in the menubar?
void BookmarkMenuBridge::AddNodeToMenu(const BookmarkNode* node, NSMenu* menu) {
  int child_count = node->child_count();
  if (child_count == 0) {
    NSString* empty_string = l10n_util::GetNSString(IDS_MENU_EMPTY_SUBMENU);
    NSMenuItem* item =
        [[[NSMenuItem alloc] initWithTitle:empty_string
                                    action:nil
                             keyEquivalent:@""] autorelease];
    [menu addItem:item];
    return;
  }

  for (int i = 0; i < child_count; i++) {
    const BookmarkNode* child = node->GetChild(i);
    if (child->is_folder()) {
      AddNodeAsSubmenu(menu, child, folder_image_);
    } else {
      NSString* title = [BookmarkMenuCocoaController menuTitleForNode:child];
      NSMenuItem* item =
          [[[NSMenuItem alloc] initWithTitle:title action:nil keyEquivalent:@""]
              autorelease];
      bookmark_nodes_[child] = item;
      [menu addItem:item];
      ConfigureMenuItem(child, item, false);
    }
  }
}

void BookmarkMenuBridge::ConfigureMenuItem(const BookmarkNode* node,
                                           NSMenuItem* item,
                                           bool set_title) {
  if (set_title)
    [item setTitle:[BookmarkMenuCocoaController menuTitleForNode:node]];
  [item setTarget:controller_];
  [item setAction:@selector(openBookmarkMenuItem:)];
  [item setTag:node->id()];
  if (node->is_url())
    [item setToolTip:[BookmarkMenuCocoaController tooltipForNode:node]];
  // Check to see if we have a favicon.
  NSImage* favicon = nil;
  BookmarkModel* model = GetBookmarkModel();
  if (model) {
    const gfx::Image& image = model->GetFavicon(node);
    if (!image.IsEmpty())
      favicon = image.ToNSImage();
  }
  // If we do not have a loaded favicon, use the default site image instead.
  if (!favicon) {
    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    favicon = rb.GetNativeImageNamed(IDR_DEFAULT_FAVICON).ToNSImage();
    [favicon setTemplate:YES];
  }
  [item setImage:favicon];
}

NSMenuItem* BookmarkMenuBridge::MenuItemForNode(const BookmarkNode* node) {
  if (!node)
    return nil;
  auto it = bookmark_nodes_.find(node);
  if (it == bookmark_nodes_.end())
    return nil;
  return it->second;
}
