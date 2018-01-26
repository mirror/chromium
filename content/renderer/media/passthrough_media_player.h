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

class PassthroughMediaPlayerDelegate;

// Allows control of an underlying WebMediaPlayer, bypassing the passthrough
// wrapper.  This lets a PassthroughMediaPlayerDelegate call through to the
// player impl to do work.
//
// As a convenience, we maintain ownership of the delegate, and inform it about
// us.  This makes it easier to coordinate object destruction.
class PassthroughMediaPlayerController {
 public:
  PassthroughMediaPlayerController(
      std::unique_ptr<PassthroughMediaPlayerDelegate> delegate);
  virtual ~PassthroughMediaPlayerController();

  // Call through to the underlying player's implementation.
  virtual void PlayImpl() = 0;

 protected:
  PassthroughMediaPlayerDelegate* delegate() const { return delegate_.get(); }

 private:
  std::unique_ptr<PassthroughMediaPlayerDelegate> delegate_;
};

// Delegate class that intercepts important WebMediaPlayer calls, and may
// forward them (or not) to the underlying impl via a Controller.
class PassthroughMediaPlayerDelegate {
 public:
  virtual ~PassthroughMediaPlayerDelegate();

  // Notify us about the controller that we should use to control the underlying
  // media player impl.
  void SetController(PassthroughMediaPlayerController* controller);

  // The delegate can also specify the client that the underlying player should
  // use to communicate back.
  virtual blink::WebMediaPlayerClient* GetClient() = 0;

  // Called when the element calls Play() on the WebMediaPlayer.  A do-nothing
  // implementation would call controller()->PlayImpl() to forward the request
  // to the underlying WebMediaPlayer.
  virtual void OnPlay() = 0;

 protected:
  PassthroughMediaPlayerController* controller() const { return controller_; }

 private:
  PassthroughMediaPlayerController* controller_;
};

// Wrapper around an existing media player type.  This is supposed to be a
// lightweight wrapper around a concrete impl, rather than a full WMP impl
// itself.  While we could be a non-templated WMP that simply calls through to
// an arbitrary (run-time) instance of WMP, that involves proxying a lot of
// calls for no reason at all.  From a code maintenence perspective, it means
// that adding methods to WebMediaPlayer with a default impl will not proxy
// them to underlying players, unless one also remembers to edit this class.
// That seems like it's quite error-prone.
//
// In contrast, deriving from |Impl| will not have this problem and probably be
// less total (generated) code, since we have only ~2 implementations anyway.
//
// Unfortunately, the same trick won't work for the WebMediaPlayerClient, so
// presumably, whoever uses this also wraps up the real client in a
// PassthroughWebMediaPlayerClient and gives that to us.
//
// Important but subtle note: the order of inheritance matters.  We want to
// init the controller before Impl so that it can be destructed afterwards.
// This lets it own the WebMediaPlayerClient, which might be accessed from
// Impl's destructor.  As a side bonus, it lets us fully init the controller
// with the Passthrough...Controller.  While it's impossible for that to really
// matter, it still seems nicer.  Note that it's impossble because the delegate
// must be called from the blink side, and while it's possible (likely) that the
// WebMediaPlayerClient will be called during construction of Impl, the blink
// side can't yet have a pointer to us yet, since we're still being constructed.
// The only way to call into the delegate is if blink (as WebMediaPlayerClient)
// calls into the WMP (us), and we, in turn, call into the delegate.
template <typename Impl>
class PassthroughMediaPlayer : public PassthroughMediaPlayerController,
                               public Impl {
 public:
  template <typename... Args>
  PassthroughMediaPlayer(
      std::unique_ptr<PassthroughMediaPlayerDelegate> delegate,
      Args... args)
      : PassthroughMediaPlayerController(std::move(delegate)),
        Impl(std::move(args)...) {}

  // WebMediaPlayer

  // Override calls to WMP::Play, and redirect to |resource_impl_|.
  void Play() override { delegate()->OnPlay(); }

  // PassthroughMediaPlayerController

  // Called by |resource_impl_| to notify the player impl.
  void PlayImpl() { Impl::Play(); }

 private:
  // Class which we delegate selected WMP calls to.
  // std::unique_ptr<PassthroughMediaPlayerDelegate> delegate_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_PASSTHROUGH_MEDIA_PLAYER_H_
