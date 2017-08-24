// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/prompting_mailto_url_rewriter.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "base/logging.h"
#import "ios/chrome/browser/web/mailto_handler.h"
#import "ios/chrome/browser/web/mailto_handler_gmail.h"
#import "ios/chrome/browser/web/mailto_handler_system_mail.h"

#import <UIKit/UIKit.h>

namespace {
NSString* const kMailtoDefaultHandlerKey = @"MailtoHandlerDefault";
}  // namespace

@interface PromptingMailtoURLRewriter ()

// Dictionary keyed by the unique ID of the Mail client. The value is
// the MailtoHandler object that can rewrite a mailto: URL.
@property(nonatomic, strong)
    NSMutableDictionary<NSString*, MailtoHandler*>* handlers;

- (void)addMailtoApps:(NSArray<MailtoHandler*>*)handlerApps;

@end

@implementation PromptingMailtoURLRewriter
@synthesize handlers = _handlers;

- (instancetype)init {
  self = [super init];
  if (self) {
    _handlers = [NSMutableDictionary dictionary];
  }
  return self;
}

- (instancetype)initWithStandardHandlers {
  self = [self init];
  if (self) {
    [self addMailtoApps:@[
      [[MailtoHandlerSystemMail alloc] init], [[MailtoHandlerGmail alloc] init]
    ]];
  }
  return self;
}

- (NSArray<MailtoHandler*>*)defaultHandlers {
  return [[_handlers allValues]
      sortedArrayUsingComparator:^NSComparisonResult(
          MailtoHandler* _Nonnull obj1, MailtoHandler* _Nonnull obj2) {
        return [[obj1 appName] compare:[obj2 appName]];
      }];
}

- (NSString*)defaultHandlerID {
  NSString* value = [[NSUserDefaults standardUserDefaults]
      stringForKey:kMailtoDefaultHandlerKey];
  if (value) {
    if ([_handlers[value] isAvailable])
      return value;
    return [[self class] systemMailApp];
  }
  // User has not made a choice.
  NSMutableArray* availableHandlers = [NSMutableArray array];
  for (MailtoHandler* handler in [_handlers allValues]) {
    if ([handler isAvailable])
      [availableHandlers addObject:handler];
  }
  if ([availableHandlers count] == 1)
    return [[availableHandlers firstObject] appStoreID];
  return nil;
}

- (void)setDefaultHandlerID:(NSString*)appStoreID {
  DCHECK([appStoreID length]);
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  if ([appStoreID
          isEqualToString:[defaults objectForKey:kMailtoDefaultHandlerKey]])
    return;
  [defaults setObject:appStoreID forKey:kMailtoDefaultHandlerKey];
  [self.observer rewriterDidChange:self];
}

- (NSString*)defaultHandlerName {
  NSString* handlerID = [self defaultHandlerID];
  if (!handlerID)
    return nil;
  MailtoHandler* handler = _handlers[handlerID];
  return [handler appName];
}

- (NSString*)rewriteMailtoURL:(const GURL&)gURL {
  NSString* value = [self defaultHandlerID];
  if ([value length]) {
    MailtoHandler* handler = _handlers[value];
    if ([handler isAvailable]) {
      return [handler rewriteMailtoURL:gURL];
    }
  }
  return nil;
}

#pragma mark - Private

- (void)addMailtoApps:(NSArray<MailtoHandler*>*)handlerApps {
  for (MailtoHandler* app in handlerApps) {
    [_handlers setObject:app forKey:[app appStoreID]];
  }
}

@end
