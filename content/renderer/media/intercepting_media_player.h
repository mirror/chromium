// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_PASSTHROUGH_MEDIA_PLAYER_H_
#define CONTENT_RENDERER_MEDIA_PASSTHROUGH_MEDIA_PLAYER_H_

#include <memory>

#include "third_party/WebKit/public/platform/WebMediaPlayer.h"

namespace blink {
class WebMediaPlayerClient;
}  // namespace blink

namespace content {

class InterceptingMediaPlayerDelegate;

// Allows control of an underlying WebMediaPlayer, bypassing the intercepting
// wrapper.  This lets a InterceptingMediaPlayerDelegate call through to the
// player impl to do work.
//
// As a convenience, we maintain ownership of the delegate, and inform it about
// us.  This makes it easier to coordinate object destruction.
class InterceptingMediaPlayerController {
 public:
  InterceptingMediaPlayerController(
      std::unique_ptr<InterceptingMediaPlayerDelegate> delegate);
  virtual ~InterceptingMediaPlayerController();

  // Call through to the underlying player's implementation.
  virtual void PlayImpl() = 0;

 protected:
  InterceptingMediaPlayerDelegate* delegate() const { return delegate_.get(); }

 private:
  std::unique_ptr<InterceptingMediaPlayerDelegate> delegate_;
};

// Delegate class that is notified about important WebMediaPlayer calls, and may
// forward them (or not) to the underlying impl via a Controller.
class InterceptingMediaPlayerDelegate {
 public:
  virtual ~InterceptingMediaPlayerDelegate();

  // Notify us about the controller that we should use to control the underlying
  // media player impl.
  void SetController(InterceptingMediaPlayerController* controller);

  // The delegate can also specify the client that the underlying player should
  // use to communicate back.
  virtual blink::WebMediaPlayerClient* GetClient() = 0;

  // Called when the element calls Play() on the WebMediaPlayer.  A do-nothing
  // implementation would call controller()->PlayImpl() to forward the request
  // to the underlying WebMediaPlayer.
  virtual void OnPlay() = 0;

 protected:
  InterceptingMediaPlayerController* controller() const { return controller_; }

 private:
  InterceptingMediaPlayerController* controller_;
};

// Wrapper around an existing media player type.  This is supposed to be a
// lightweight wrapper around a concrete impl, rather than a full WMP impl
// itself.  While we could be a non-templated WMP that simply calls through to
// an arbitrary (run-time) instance of WMP, that involves proxying a lot of
// calls for no reason at all.  From a code maintenence perspective, it means
// that adding methods to WebMediaPlayer with a default impl will not proxy
// them to underlying players, unless one also remembers to edit this class.
// That seems like it's quite error-prone.  Ownership in that model is quite
// simple, however, compared to the work we do here.
//
// In contrast, deriving from |Impl| will not have this problem and probably be
// less total (generated) code, since we have only ~2 implementations anyway.
//
// Unfortunately, the same trick won't work for the WebMediaPlayerClient, so
// presumably, whoever uses this also wraps up the real client in a
// InterceptingWebMediaPlayerClient and gives that to us.
//
// Important but subtle note: the order of inheritance matters.  We want to
// init the controller before Impl so that it can be destructed afterwards.
// This lets it own the WebMediaPlayerClient, which might be accessed from
// Impl's destructor.  As a side bonus, it lets us fully init the controller
// with the Intercepting...Controller.  While it's impossible for that to really
// matter, it still seems nicer.  Note that it's impossble because the delegate
// must be called from the blink side, and while it's possible (likely) that the
// WebMediaPlayerClient will be called during construction of Impl, the blink
// side can't yet have a pointer to us yet, since we're still being constructed.
// The only way to call into the delegate is if blink (as WebMediaPlayerClient)
// calls into the WMP (us), and we, in turn, call into the delegate.
template <typename Impl>
class InterceptingMediaPlayer : public InterceptingMediaPlayerController,
                                public Impl {
 public:
  template <typename... Args>
  InterceptingMediaPlayer(
      std::unique_ptr<InterceptingMediaPlayerDelegate> delegate,
      Args... args)
      : InterceptingMediaPlayerController(std::move(delegate)),
        Impl(std::move(args)...) {}

  // WebMediaPlayer
  // Override whatever methods you'd like to intercept.

  // Override calls to WMP::Play, and redirect to |resource_impl_|.
  void Play() override { delegate()->OnPlay(); }

  // InterceptingMediaPlayerController

  // Called by the delegate to notify the player impl.
  void PlayImpl() { Impl::Play(); }
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_PASSTHROUGH_MEDIA_PLAYER_H_
