// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/web_state/web_state_observer.h"
#import "ios/web/public/web_state/web_state_user_data.h"

#include "base/macros.h"
#include "base/timer/elapsed_timer.h"

@protocol SadTabTabHelperDelegate;

// SadTabTabHelper listens to RenderProcessGone events and presents a
// SadTabView view appropriately.
class SadTabTabHelper : public web::WebStateUserData<SadTabTabHelper>,
                        public web::WebStateObserver {
 public:
  // Creates a SadTabTabHelper and attaches it to a specific web_state object.
  // Uses the default |repeat_failure_interval|. |delegate| is not retained by
  // TabHelper and must not be null.
  static void CreateForWebState(web::WebState* web_state,
                                id<SadTabTabHelperDelegate> delegate);

  // Creates a SadTabTabHelper and attaches it to a specific web)state object,
  // |repeat_failure_interval| sets the corresponding instance variable used for
  // determining repeat failures. |delegate| is not retained by TabHelper and
  // must not be null.
  static void CreateForWebState(web::WebState* web_state,
                                double repeat_failure_interval,
                                id<SadTabTabHelperDelegate> delegate);

  ~SadTabTabHelper() override;

  bool requires_reload_on_becoming_visible() const {
    return requires_reload_on_becoming_visible_;
  }
  void set_requires_reload_on_becoming_visible(bool flag) {
    requires_reload_on_becoming_visible_ = flag;
  }

  bool requires_reload_on_becoming_active() const {
    return requires_reload_on_becoming_active_;
  }
  void set_requires_reload_on_becoming_active(bool flag) {
    requires_reload_on_becoming_active_ = flag;
  }

 private:
  // Constructs a SadTabTabHelper, assigning the helper to a web_state. A
  // default repeat_failure_interval will be used. |delegate| will be in charge
  // of presenting the SadTabView.
  SadTabTabHelper(web::WebState* web_state,
                  id<SadTabTabHelperDelegate> delegate);

  // Constructs a SadTabTabHelper allowing an optional |repeat_failure_interval|
  // value to be passed in, representing a timeout period in seconds during
  // which a second failure will be considered a 'repeated' crash rather than an
  // initial event. |delegate| will be in charge of presenting the SadTabView.
  SadTabTabHelper(web::WebState* web_state,
                  double repeat_failure_interval,
                  id<SadTabTabHelperDelegate> delegate);

  // Presents a new SadTabView via the web_state object.
  void PresentSadTab(const GURL& url_causing_failure);

  // WebStateObserver:
  void WasShown() override;
  void WasHidden() override;
  void RenderProcessGone() override;
  void DidFinishNavigation(web::NavigationContext* navigation_context) override;

  // Stores the last URL that caused a renderer crash,
  // used to detect repeated crashes.
  GURL last_failed_url_;

  // Stores the last date that the renderer crashed,
  // used to determine time window for repeated crashes.
  std::unique_ptr<base::ElapsedTimer> last_failed_timer_;

  // Stores the interval window during which a second RenderProcessGone failure
  // will be considered a repeat failure.
  double repeat_failure_interval_;

  // Whether or not WebState is currently being displayed.
  bool is_visible_;

  // true if the WebState needs to be reloaded after web state becomes visible.
  bool requires_reload_on_becoming_visible_;

  // true if the WebState needs to be reloaded after the app becomes active.
  bool requires_reload_on_becoming_active_;

  // Delegate which displays the SadTabView.
  __weak id<SadTabTabHelperDelegate> delegate_ = nil;

  DISALLOW_COPY_AND_ASSIGN(SadTabTabHelper);
};
