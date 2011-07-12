// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#import "remoting/host/disconnect_window_mac.h"

#include "base/compiler_specific.h"
#include "base/sys_string_conversions.h"
#include "remoting/host/chromoting_host.h"
#include "remoting/host/disconnect_window.h"

namespace remoting {
class DisconnectWindowMac : public remoting::DisconnectWindow {
 public:
  DisconnectWindowMac();
  virtual ~DisconnectWindowMac();

  virtual void Show(remoting::ChromotingHost* host,
                    const std::string& username) OVERRIDE;
  virtual void Hide() OVERRIDE;

  void ResetWindowController(DisconnectWindowController* window_controller);

 private:
  DisconnectWindowController* window_controller_;

  DISALLOW_COPY_AND_ASSIGN(DisconnectWindowMac);
};

DisconnectWindowMac::DisconnectWindowMac()
    : window_controller_(nil) {
}

DisconnectWindowMac::~DisconnectWindowMac() {
  [window_controller_ close];
}

void DisconnectWindowMac::Show(remoting::ChromotingHost* host,
                               const std::string& username) {
  CHECK(window_controller_ == nil);
  NSString* nsUsername = base::SysUTF8ToNSString(username);
  window_controller_ =
      [[DisconnectWindowController alloc] initWithHost:host
                                                window:this
                                              username:nsUsername];
  [window_controller_ showWindow:nil];
}

void DisconnectWindowMac::Hide() {
  [window_controller_ close];
  window_controller_ = nil;
}

void DisconnectWindowMac::ResetWindowController(
    DisconnectWindowController* window_controller) {
  if (window_controller_ == window_controller) {
    window_controller_ = NULL;
  }
}

remoting::DisconnectWindow* remoting::DisconnectWindow::Create() {
  return new DisconnectWindowMac;
}
}  // namespace remoting

@interface DisconnectWindowController()
@property (nonatomic, assign) remoting::ChromotingHost* host;
@property (nonatomic, assign) remoting::DisconnectWindowMac* disconnectWindow;
@property (nonatomic, copy) NSString* username;
@end

@implementation DisconnectWindowController

@synthesize host = host_;
@synthesize disconnectWindow = disconnectWindow_;
@synthesize username = username_;

- (id)initWithHost:(remoting::ChromotingHost*)host
            window:(remoting::DisconnectWindowMac*)disconnectWindow
          username:(NSString*)username {
  self = [super initWithWindowNibName:@"disconnect_window"];
  if (self) {
    self.host = host;
    self.disconnectWindow = disconnectWindow;
    self.username = username;
  }
  return self;
}

- (void)dealloc {
  self.username = nil;
  [super dealloc];
}

- (IBAction)stopSharing:(id)sender {
  if (self.host) {
    self.host->Shutdown(NULL);
    self.host = NULL;
  }
  if (self.disconnectWindow) {
    self.disconnectWindow->ResetWindowController(self);
    self.disconnectWindow = NULL;
  }
}

- (void)windowDidLoad {
  // TODO(jamiewalch): Resize the window to fit the username without leaving too
  // much space.
  [usernameField_ setStringValue:self.username];
}

- (void)windowWillClose:(NSNotification*)notification {
  [self stopSharing:self];
  [self autorelease];
}

@end
