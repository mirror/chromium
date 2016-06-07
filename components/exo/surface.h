// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_SURFACE_H_
#define COMPONENTS_EXO_SURFACE_H_

#include <list>
#include <memory>
#include <set>
#include <utility>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "cc/resources/transferable_resource.h"
#include "cc/surfaces/surface_factory_client.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "third_party/skia/include/core/SkXfermode.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"
#include "ui/compositor/compositor_observer.h"
#include "ui/compositor/layer_owner_delegate.h"
#include "ui/gfx/geometry/rect.h"

namespace base {
namespace trace_event {
class TracedValue;
}
}

namespace cc {
class SurfaceFactory;
enum class SurfaceDrawStatus;
}

namespace gfx {
class Path;
}

namespace exo {
class Buffer;
class Pointer;
class SurfaceDelegate;
class SurfaceObserver;
class Surface;

// The pointer class is currently the only cursor provider class but this can
// change in the future when better hardware cursor support is added.
using CursorProvider = Pointer;

// This class owns the SurfaceFactory and keeps track of references to the
// contents of Buffers. It's keeped alive by references from
// release_callbacks_. It's destroyed when its owning Surface is destroyed and
// the last outstanding release callback is called.
class SurfaceFactoryOwner : public base::RefCounted<SurfaceFactoryOwner>,
                            public cc::SurfaceFactoryClient {
 public:
  SurfaceFactoryOwner();

  // Overridden from cc::SurfaceFactoryClient:
  void ReturnResources(const cc::ReturnedResourceArray& resources) override;
  void WillDrawSurface(cc::SurfaceId id, const gfx::Rect& damage_rect) override;
  void SetBeginFrameSource(cc::BeginFrameSource* begin_frame_source) override;

 private:
  friend class base::RefCounted<SurfaceFactoryOwner>;
  friend class Surface;
  ~SurfaceFactoryOwner() override;

  std::map<int,
           std::pair<scoped_refptr<SurfaceFactoryOwner>,
                     std::unique_ptr<cc::SingleReleaseCallback>>>
      release_callbacks_;
  std::unique_ptr<cc::SurfaceIdAllocator> id_allocator_;
  std::unique_ptr<cc::SurfaceFactory> surface_factory_;
  Surface* surface_;
};

// This class represents a rectangular area that is displayed on the screen.
// It has a location, size and pixel contents.
class Surface : public aura::Window,
                public aura::WindowObserver,
                public ui::LayerOwnerDelegate,
                public ui::CompositorObserver {
 public:
  Surface();
  ~Surface() override;

  // Type-checking downcast routine.
  static Surface* AsSurface(const aura::Window* window);

  // Sets whether to put the contents in a SurfaceLayer or a TextureLayer.
  static void SetUseSurfaceLayer(bool use_surface_layer);

  // Set a buffer as the content of this surface. A buffer can only be attached
  // to one surface at a time.
  void Attach(Buffer* buffer);

  // Describe the regions where the pending buffer is different from the
  // current surface contents, and where the surface therefore needs to be
  // repainted.
  void Damage(const gfx::Rect& rect);

  // Request notification when the next frame is displayed. Useful for
  // throttling redrawing operations, and driving animations.
  using FrameCallback = base::Callback<void(base::TimeTicks frame_time)>;
  void RequestFrameCallback(const FrameCallback& callback);

  // This sets the region of the surface that contains opaque content.
  void SetOpaqueRegion(const SkRegion& region);

  // This sets the region of the surface that can receive pointer and touch
  // events.
  void SetInputRegion(const SkRegion& region);

  // This sets the scaling factor used to interpret the contents of the buffer
  // attached to the surface. Note that if the scale is larger than 1, then you
  // have to attach a buffer that is larger (by a factor of scale in each
  // dimension) than the desired surface size.
  void SetBufferScale(float scale);

  // Functions that control sub-surface state. All sub-surface state is
  // double-buffered and will be applied when Commit() is called.
  void AddSubSurface(Surface* sub_surface);
  void RemoveSubSurface(Surface* sub_surface);
  void SetSubSurfacePosition(Surface* sub_surface, const gfx::Point& position);
  void PlaceSubSurfaceAbove(Surface* sub_surface, Surface* reference);
  void PlaceSubSurfaceBelow(Surface* sub_surface, Surface* sibling);

  // This sets the surface viewport for scaling.
  void SetViewport(const gfx::Size& viewport);

  // This sets the surface crop rectangle.
  void SetCrop(const gfx::RectF& crop);

  // This sets the only visible on secure output flag, preventing it from
  // appearing in screenshots or from being viewed on non-secure displays.
  void SetOnlyVisibleOnSecureOutput(bool only_visible_on_secure_output);

  // This sets the blend mode that will be used when drawing the surface.
  void SetBlendMode(SkXfermode::Mode blend_mode);

  // This sets the alpha value that will be applied to the whole surface.
  void SetAlpha(float alpha);

  // Surface state (damage regions, attached buffers, etc.) is double-buffered.
  // A Commit() call atomically applies all pending state, replacing the
  // current state. Commit() is not guaranteed to be synchronous. See
  // CommitSurfaceHierarchy() below.
  void Commit();

  // This will synchronously commit all pending state of the surface and its
  // descendants by recursively calling CommitSurfaceHierarchy() for each
  // sub-surface with pending state.
  void CommitSurfaceHierarchy();

  // Returns true if surface is in synchronized mode.
  bool IsSynchronized() const;

  // Returns the bounds of the current input region of surface.
  gfx::Rect GetHitTestBounds() const;

  // Returns true if |rect| intersects this surface's bounds.
  bool HitTestRect(const gfx::Rect& rect) const;

  // Returns true if the current input region is different than the surface
  // bounds.
  bool HasHitTestMask() const;

  // Returns the current input region of surface in the form of a hit-test mask.
  void GetHitTestMask(gfx::Path* mask) const;

  // Surface does not own cursor providers. It is the responsibility of the
  // caller to remove the cursor provider before it is destroyed.
  void RegisterCursorProvider(CursorProvider* provider);
  void UnregisterCursorProvider(CursorProvider* provider);

  // Returns true if surface has at least one cursor provider registered.
  bool HasCursorProvider() const;

  // Set the surface delegate.
  void SetSurfaceDelegate(SurfaceDelegate* delegate);

  // Returns true if surface has been assigned a surface delegate.
  bool HasSurfaceDelegate() const;

  // Surface does not own observers. It is the responsibility of the observer
  // to remove itself when it is done observing.
  void AddSurfaceObserver(SurfaceObserver* observer);
  void RemoveSurfaceObserver(SurfaceObserver* observer);
  bool HasSurfaceObserver(const SurfaceObserver* observer) const;

  // Returns a trace value representing the state of the surface.
  std::unique_ptr<base::trace_event::TracedValue> AsTracedValue() const;

  bool HasPendingDamageForTesting(const gfx::Rect& damage) const {
    return pending_damage_.contains(gfx::RectToSkIRect(damage));
  }

  // Overridden from aura::WindowObserver:
  void OnWindowAddedToRootWindow(aura::Window* window) override;
  void OnWindowRemovingFromRootWindow(aura::Window* window,
                                      aura::Window* new_root) override;

  // Overridden from ui::LayerOwnerDelegate:
  void OnLayerRecreated(ui::Layer* old_layer, ui::Layer* new_layer) override;

  // Overridden from ui::CompositorObserver:
  void OnCompositingDidCommit(ui::Compositor* compositor) override;
  void OnCompositingStarted(ui::Compositor* compositor,
                            base::TimeTicks start_time) override;
  void OnCompositingEnded(ui::Compositor* compositor) override;
  void OnCompositingAborted(ui::Compositor* compositor) override;
  void OnCompositingLockStateChanged(ui::Compositor* compositor) override {}
  void OnCompositingShuttingDown(ui::Compositor* compositor) override;

  void WillDraw(cc::SurfaceId surface_id);

 private:
  bool needs_commit_surface_hierarchy() const {
    return needs_commit_surface_hierarchy_;
  }

  // Commit the current attached buffer to a TextureLayer.
  void CommitTextureContents();

  // Commit the current attached buffer to a SurfaceLayer.
  void CommitSurfaceContents();

  // Set TextureLayer contents to the current buffer.
  void SetTextureLayerContents(ui::Layer* layer);

  // Set SurfaceLayer contents to the current buffer.
  void SetSurfaceLayerContents(ui::Layer* layer);

  // This returns true when the surface has some contents assigned to it.
  bool has_contents() const { return !!current_buffer_; }

  // This is true if the buffer contents should be put in a SurfaceLayer
  // rather than a TextureLayer.
  static bool use_surface_layer_;

  // This is true when Attach() has been called and new contents should take
  // effect next time Commit() is called.
  bool has_pending_contents_;

  // The buffer that will become the content of surface when Commit() is called.
  base::WeakPtr<Buffer> pending_buffer_;

  cc::SurfaceManager* surface_manager_;

  scoped_refptr<SurfaceFactoryOwner> factory_owner_;

  // The Surface Id currently attached to the window.
  cc::SurfaceId surface_id_;

  // The next resource id the buffer will be attached to.
  int next_resource_id_ = 1;

  // The damage region to schedule paint for when Commit() is called.
  SkRegion pending_damage_;

  // These lists contains the callbacks to notify the client when it is a good
  // time to start producing a new frame. These callbacks move to
  // |frame_callbacks_| when Commit() is called. Later they are moved to
  // |active_frame_callbacks_| when the effect of the Commit() is reflected in
  // the compositor's active layer tree. The callbacks fire once we're notified
  // that the compositor started drawing that active layer tree.
  std::list<FrameCallback> pending_frame_callbacks_;
  std::list<FrameCallback> frame_callbacks_;
  std::list<FrameCallback> active_frame_callbacks_;

  // The opaque region to take effect when Commit() is called.
  SkRegion pending_opaque_region_;

  // The input region to take effect when Commit() is called.
  SkRegion pending_input_region_;

  // The buffer scaling factor to take effect when Commit() is called.
  float pending_buffer_scale_;

  // The stack of sub-surfaces to take effect when Commit() is called.
  // Bottom-most sub-surface at the front of the list and top-most sub-surface
  // at the back.
  using SubSurfaceEntry = std::pair<Surface*, gfx::Point>;
  using SubSurfaceEntryList = std::list<SubSurfaceEntry>;
  SubSurfaceEntryList pending_sub_surfaces_;

  // The viewport to take effect when Commit() is called.
  gfx::Size pending_viewport_;

  // The crop rectangle to take effect when Commit() is called.
  gfx::RectF pending_crop_;

  // The active crop rectangle.
  gfx::RectF crop_;

  // The secure output visibility state to take effect when Commit() is called.
  bool pending_only_visible_on_secure_output_;

  // The active secure output visibility state.
  bool only_visible_on_secure_output_;

  // The blend mode state to take effect when Commit() is called.
  SkXfermode::Mode pending_blend_mode_;

  // The alpha state to take effect when Commit() is called.
  float pending_alpha_;

  // The active alpha state.
  float alpha_;

  // The buffer that is currently set as content of surface.
  base::WeakPtr<Buffer> current_buffer_;

  // The last resource that was sent to a surface.
  cc::TransferableResource current_resource_;

  // The active input region used for hit testing.
  SkRegion input_region_;

  // This is true if a call to Commit() as been made but
  // CommitSurfaceHierarchy() has not yet been called.
  bool needs_commit_surface_hierarchy_;

  // This is set when the compositing starts and passed to active frame
  // callbacks when compositing successfully ends.
  base::TimeTicks last_compositing_start_time_;

  // This is true when the contents of the surface should be updated next time
  // the compositor successfully ends compositing.
  bool update_contents_after_successful_compositing_;

  // The compsitor being observer or null if not observing a compositor.
  ui::Compositor* compositor_;

  // Cursor providers. Surface does not own the cursor providers.
  std::set<CursorProvider*> cursor_providers_;

  // Texture size.
  gfx::Size texture_size_in_dip_;

  // This can be set to have some functions delegated. E.g. ShellSurface class
  // can set this to handle Commit() and apply any double buffered state it
  // maintains.
  SurfaceDelegate* delegate_;

  // Surface observer list. Surface does not own the observers.
  base::ObserverList<SurfaceObserver, true> observers_;

  DISALLOW_COPY_AND_ASSIGN(Surface);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_SURFACE_H_
