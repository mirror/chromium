// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MUS_WS_SERVER_WINDOW_H_
#define COMPONENTS_MUS_WS_SERVER_WINDOW_H_

#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "components/mus/public/interfaces/compositor_frame.mojom.h"
#include "components/mus/public/interfaces/window_tree.mojom.h"
#include "components/mus/ws/ids.h"
#include "components/mus/ws/server_window_surface.h"
#include "third_party/mojo/src/mojo/public/cpp/bindings/binding.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/transform.h"
#include "ui/platform_window/text_input_state.h"

namespace mus {

namespace ws {

class ServerWindowDelegate;
class ServerWindowObserver;
class ServerWindowSurfaceManager;

// Server side representation of a window. Delegate is informed of interesting
// events.
//
// It is assumed that all functions that mutate the tree have validated the
// mutation is possible before hand. For example, Reorder() assumes the supplied
// window is a child and not already in position.
//
// ServerWindows do not own their children. If you delete a window that has
// children the children are implicitly removed. Similarly if a window has a
// parent and the window is deleted the deleted window is implicitly removed
// from the parent.
class ServerWindow {
 public:
  using Windows = std::vector<ServerWindow*>;

  ServerWindow(ServerWindowDelegate* delegate, const WindowId& id);
  ~ServerWindow();

  void AddObserver(ServerWindowObserver* observer);
  void RemoveObserver(ServerWindowObserver* observer);

  // Creates a new surface of the specified type, replacing the existing.
  void CreateSurface(mojom::SurfaceType surface_type,
                     mojo::InterfaceRequest<mojom::Surface> request,
                     mojom::SurfaceClientPtr client);

  const WindowId& id() const { return id_; }

  void Add(ServerWindow* child);
  void Remove(ServerWindow* child);
  void Reorder(ServerWindow* child,
               ServerWindow* relative,
               mojom::OrderDirection direction);

  const gfx::Rect& bounds() const { return bounds_; }
  // Sets the bounds. If the size changes this implicitly resets the client
  // area to fill the whole bounds.
  void SetBounds(const gfx::Rect& bounds);

  const gfx::Rect& client_area() const { return client_area_; }
  void SetClientArea(const gfx::Rect& bounds);

  const ServerWindow* parent() const { return parent_; }
  ServerWindow* parent() { return parent_; }

  const ServerWindow* GetRoot() const;
  ServerWindow* GetRoot() {
    return const_cast<ServerWindow*>(
        const_cast<const ServerWindow*>(this)->GetRoot());
  }

  std::vector<const ServerWindow*> GetChildren() const;
  std::vector<ServerWindow*> GetChildren();
  const Windows& children() const { return children_; }

  // Returns the ServerWindow object with the provided |id| if it lies in a
  // subtree of |this|.
  ServerWindow* GetChildWindow(const WindowId& id);

  // Returns true if this contains |window| or is |window|.
  bool Contains(const ServerWindow* window) const;

  // Returns the visibility requested by this window. IsDrawn() returns whether
  // the window is actually visible on screen.
  bool visible() const { return visible_; }
  void SetVisible(bool value);

  float opacity() const { return opacity_; }
  void SetOpacity(float value);

  const gfx::Transform& transform() const { return transform_; }
  void SetTransform(const gfx::Transform& transform);

  const std::map<std::string, std::vector<uint8_t>>& properties() const {
    return properties_;
  }
  void SetProperty(const std::string& name, const std::vector<uint8_t>* value);

  void SetTextInputState(const ui::TextInputState& state);
  const ui::TextInputState& text_input_state() const {
    return text_input_state_;
  }

  // Returns true if this window is attached to a root and all ancestors are
  // visible.
  bool IsDrawn() const;

  ServerWindowSurfaceManager* GetOrCreateSurfaceManager();
  ServerWindowSurfaceManager* surface_manager() {
    return surface_manager_.get();
  }

  ServerWindowDelegate* delegate() { return delegate_; }

#if !defined(NDEBUG)
  std::string GetDebugWindowHierarchy() const;
  void BuildDebugInfo(const std::string& depth, std::string* result) const;
#endif

 private:

  // Implementation of removing a window. Doesn't send any notification.
  void RemoveImpl(ServerWindow* window);

  ServerWindowDelegate* delegate_;
  const WindowId id_;
  ServerWindow* parent_;
  Windows children_;
  bool visible_;
  gfx::Rect bounds_;
  gfx::Rect client_area_;
  scoped_ptr<ServerWindowSurfaceManager> surface_manager_;
  float opacity_;
  gfx::Transform transform_;
  ui::TextInputState text_input_state_;

  std::map<std::string, std::vector<uint8_t>> properties_;

  base::ObserverList<ServerWindowObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(ServerWindow);
};

}  // namespace ws

}  // namespace mus

#endif  // COMPONENTS_MUS_WS_SERVER_WINDOW_H_
