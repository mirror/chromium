// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_TOOLS_TOOLS_MENU_ITEM_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_TOOLS_TOOLS_MENU_ITEM_H_

#import <Foundation/Foundation.h>

// Object that encapsulates the title and function of a menu item.
@interface ToolsMenuItem : NSObject
// The title of the item.
@property(nonatomic, copy) NSString* title;
// The selector called by the item. This will be called on the global dispatcher
// if |local| is NO, or on the menu dispatcher if it is YES.
@property(nonatomic) SEL action;
// YES if the menu item is enabled.
@property(nonatomic) BOOL enabled;
// YES if the menu dispatcher should be used; see above.
@property(nonatomic) BOOL local;
@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_TOOLS_TOOLS_MENU_ITEM_H_
