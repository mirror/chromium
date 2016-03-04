// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/mus/ws/connection_manager.h"

#include "base/logging.h"
#include "base/stl_util.h"
#include "components/mus/ws/client_connection.h"
#include "components/mus/ws/connection_manager_delegate.h"
#include "components/mus/ws/operation.h"
#include "components/mus/ws/server_window.h"
#include "components/mus/ws/window_coordinate_conversions.h"
#include "components/mus/ws/window_manager_factory_service.h"
#include "components/mus/ws/window_manager_state.h"
#include "components/mus/ws/window_tree_host_connection.h"
#include "components/mus/ws/window_tree_impl.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/input_events/input_events_type_converters.h"
#include "mojo/converters/surfaces/surfaces_type_converters.h"
#include "mojo/shell/public/cpp/connection.h"
#include "ui/gfx/geometry/size_conversions.h"

namespace mus {
namespace ws {

ConnectionManager::ConnectionManager(
    ConnectionManagerDelegate* delegate,
    const scoped_refptr<mus::SurfacesState>& surfaces_state)
    : delegate_(delegate),
      surfaces_state_(surfaces_state),
      next_connection_id_(1),
      next_host_id_(0),
      current_operation_(nullptr),
      in_destructor_(false),
      next_wm_change_id_(0),
      got_valid_frame_decorations_(false),
      window_manager_factory_registry_(this, &user_id_tracker_) {}

ConnectionManager::~ConnectionManager() {
  in_destructor_ = true;

  while (!pending_hosts_.empty())
    DestroyHost(*pending_hosts_.begin());
  DCHECK(pending_hosts_.empty());

  // DestroyHost() removes from |hosts_| and deletes the WindowTreeHostImpl.
  while (!hosts_.empty())
    DestroyHost(*hosts_.begin());
  DCHECK(hosts_.empty());

  tree_map_.clear();
}

void ConnectionManager::AddHost(WindowTreeHostImpl* host) {
  DCHECK_EQ(0u, pending_hosts_.count(host));
  pending_hosts_.insert(host);
}

void ConnectionManager::DestroyHost(WindowTreeHostImpl* host) {
  for (auto& pair : tree_map_)
    pair.second->OnWillDestroyWindowTreeHost(host);

  if (pending_hosts_.count(host)) {
    pending_hosts_.erase(host);
  } else {
    DCHECK(hosts_.count(host));
    hosts_.erase(host);
  }
  delete host;

  // If we have no more roots left, let the app know so it can terminate.
  if (!hosts_.size() && !pending_hosts_.size())
    delegate_->OnNoMoreRootConnections();
}

ServerWindow* ConnectionManager::CreateServerWindow(
    const WindowId& id,
    const std::map<std::string, std::vector<uint8_t>>& properties) {
  ServerWindow* window = new ServerWindow(this, id, properties);
  window->AddObserver(this);
  return window;
}

ConnectionSpecificId ConnectionManager::GetAndAdvanceNextConnectionId() {
  const ConnectionSpecificId id = next_connection_id_++;
  DCHECK_LT(id, next_connection_id_);
  return id;
}

uint16_t ConnectionManager::GetAndAdvanceNextHostId() {
  const uint16_t id = next_host_id_++;
  DCHECK_LT(id, next_host_id_);
  return id;
}

WindowTreeImpl* ConnectionManager::EmbedAtWindow(
    ServerWindow* root,
    uint32_t policy_bitmask,
    mojom::WindowTreeClientPtr client) {
  scoped_ptr<WindowTreeImpl> tree_ptr(
      new ws::WindowTreeImpl(this, root, policy_bitmask));
  WindowTreeImpl* tree = tree_ptr.get();

  mojom::WindowTreePtr window_tree_ptr;
  scoped_ptr<ClientConnection> client_connection =
      delegate_->CreateClientConnectionForEmbedAtWindow(
          this, tree, GetProxy(&window_tree_ptr), std::move(client));

  AddTree(std::move(tree_ptr), std::move(client_connection),
          std::move(window_tree_ptr));
  OnConnectionMessagedClient(tree->id());
  return tree;
}

WindowTreeImpl* ConnectionManager::AddTree(
    scoped_ptr<WindowTreeImpl> tree_impl_ptr,
    scoped_ptr<ClientConnection> connection,
    mojom::WindowTreePtr tree_ptr) {
  CHECK_EQ(0u, tree_map_.count(tree_impl_ptr->id()));
  WindowTreeImpl* tree = tree_impl_ptr.get();
  tree_map_[tree->id()] = std::move(tree_impl_ptr);
  tree->Init(std::move(connection), std::move(tree_ptr));
  return tree;
}

WindowTreeImpl* ConnectionManager::CreateTreeForWindowManager(
    WindowTreeHostImpl* host,
    mojom::WindowManagerFactory* factory,
    ServerWindow* root) {
  mojom::DisplayPtr display = DisplayForHost(host);
  mojom::WindowTreeClientPtr tree_client;
  factory->CreateWindowManager(std::move(display), GetProxy(&tree_client));
  scoped_ptr<ws::WindowTreeImpl> tree_ptr(new ws::WindowTreeImpl(
      this, root, mojom::WindowTree::kAccessPolicyEmbedRoot));
  ws::WindowTreeImpl* tree = tree_ptr.get();
  scoped_ptr<ws::DefaultClientConnection> connection(
      new ws::DefaultClientConnection(tree_ptr.get(), this,
                                      std::move(tree_client)));
  mojom::WindowTreePtr window_tree_ptr =
      connection->CreateInterfacePtrAndBind();
  AddTree(std::move(tree_ptr), std::move(connection),
          std::move(window_tree_ptr));
  tree->ConfigureWindowManager();
  return tree;
}

void ConnectionManager::DestroyTree(WindowTreeImpl* tree) {
  scoped_ptr<WindowTreeImpl> tree_ptr;
  {
    auto iter = tree_map_.find(tree->id());
    DCHECK(iter != tree_map_.end());
    tree_ptr = std::move(iter->second);
    tree_map_.erase(iter);
  }

  // Notify remaining connections so that they can cleanup.
  for (auto& pair : tree_map_)
    pair.second->OnWindowDestroyingTreeImpl(tree);

  // Notify the hosts, taking care to only notify each host once.
  std::set<WindowTreeHostImpl*> hosts_notified;
  for (auto* root : tree->roots()) {
    WindowTreeHostImpl* host = GetWindowTreeHostByWindow(root);
    if (host && hosts_notified.count(host) == 0) {
      host->OnWindowTreeConnectionError(tree);
      hosts_notified.insert(host);
    }
  }

  // Remove any requests from the client that resulted in a call to the window
  // manager and we haven't gotten a response back yet.
  std::set<uint32_t> to_remove;
  for (auto& pair : in_flight_wm_change_map_) {
    if (pair.second.connection_id == tree->id())
      to_remove.insert(pair.first);
  }
  for (uint32_t id : to_remove)
    in_flight_wm_change_map_.erase(id);
}

WindowTreeImpl* ConnectionManager::GetConnection(
    ConnectionSpecificId connection_id) {
  auto iter = tree_map_.find(connection_id);
  return iter == tree_map_.end() ? nullptr : iter->second.get();
}

ServerWindow* ConnectionManager::GetWindow(const WindowId& id) {
  // kInvalidConnectionId is used for WindowTreeHost and WindowManager root
  // nodes.
  if (id.connection_id == kInvalidConnectionId) {
    for (WindowTreeHostImpl* host : hosts_) {
      ServerWindow* window = host->GetRootWithId(id);
      if (window)
        return window;
    }
  }
  WindowTreeImpl* service = GetConnection(id.connection_id);
  return service ? service->GetWindow(id) : nullptr;
}

void ConnectionManager::SchedulePaint(const ServerWindow* window,
                                      const gfx::Rect& bounds) {
  for (WindowTreeHostImpl* host : hosts_) {
    if (host->SchedulePaintIfInViewport(window, bounds))
      return;
  }
}

void ConnectionManager::OnWindowTreeHostDisplayAvailable(
    WindowTreeHostImpl* host) {
  DCHECK_NE(0u, pending_hosts_.count(host));
  DCHECK_EQ(0u, hosts_.count(host));
  const bool is_first_connection = hosts_.empty();
  hosts_.insert(host);
  pending_hosts_.erase(host);
  if (is_first_connection)
    delegate_->OnFirstRootConnectionCreated();
}

void ConnectionManager::OnConnectionMessagedClient(ConnectionSpecificId id) {
  if (current_operation_)
    current_operation_->MarkConnectionAsMessaged(id);
}

bool ConnectionManager::DidConnectionMessageClient(
    ConnectionSpecificId id) const {
  return current_operation_ && current_operation_->DidMessageConnection(id);
}

mojom::ViewportMetricsPtr ConnectionManager::GetViewportMetricsForWindow(
    const ServerWindow* window) {
  WindowTreeHostImpl* host = GetWindowTreeHostByWindow(window);
  if (host)
    return host->GetViewportMetrics().Clone();

  if (!hosts_.empty())
    return (*hosts_.begin())->GetViewportMetrics().Clone();

  mojom::ViewportMetricsPtr metrics = mojom::ViewportMetrics::New();
  metrics->size_in_pixels = mojo::Size::New();
  return metrics;
}

const WindowTreeImpl* ConnectionManager::GetConnectionWithRoot(
    const ServerWindow* window) const {
  if (!window)
    return nullptr;
  for (auto& pair : tree_map_) {
    if (pair.second->HasRoot(window))
      return pair.second.get();
  }
  return nullptr;
}

WindowManagerAndHostConst ConnectionManager::GetWindowManagerAndHost(
    const ServerWindow* window) const {
  const ServerWindow* last = window;
  while (window && window->parent()) {
    last = window;
    window = window->parent();
  }
  for (WindowTreeHostImpl* host : hosts_) {
    if (window == host->root_window()) {
      WindowManagerAndHostConst result;
      result.window_tree_host = host;
      result.window_manager_state = host->GetWindowManagerStateWithRoot(last);
      return result;
    }
  }
  return WindowManagerAndHostConst();
}

WindowManagerAndHost ConnectionManager::GetWindowManagerAndHost(
    const ServerWindow* window) {
  WindowManagerAndHostConst result_const =
      const_cast<const ConnectionManager*>(this)->GetWindowManagerAndHost(
          window);
  WindowManagerAndHost result;
  result.window_tree_host =
      const_cast<WindowTreeHostImpl*>(result_const.window_tree_host);
  result.window_manager_state =
      const_cast<WindowManagerState*>(result_const.window_manager_state);
  return result;
}

WindowTreeHostImpl* ConnectionManager::GetWindowTreeHostByWindow(
    const ServerWindow* window) {
  return const_cast<WindowTreeHostImpl*>(
      static_cast<const ConnectionManager*>(this)
          ->GetWindowTreeHostByWindow(window));
}

const WindowTreeHostImpl* ConnectionManager::GetWindowTreeHostByWindow(
    const ServerWindow* window) const {
  while (window && window->parent())
    window = window->parent();
  for (WindowTreeHostImpl* host : hosts_) {
    if (window == host->root_window())
      return host;
  }
  return nullptr;
}

WindowTreeHostImpl* ConnectionManager::GetActiveWindowTreeHost() {
  // TODO(sky): this isn't active, but first. Make it active.
  return hosts_.size() ? *hosts_.begin() : nullptr;
}

void ConnectionManager::AddDisplayManagerBinding(
    mojo::InterfaceRequest<mojom::DisplayManager> request) {
  display_manager_bindings_.AddBinding(this, std::move(request));
}

void ConnectionManager::OnFirstWindowManagerFactorySet() {
  if (!hosts_.empty() || !pending_hosts_.empty())
    return;

  // We've been supplied a WindowManagerFactory and no windowtreehosts have been
  // created yet. Treat this as a signal to create a WindowTreeHost.
  // TODO(sky): we need a better way to determine this, most likely a switch.
  delegate_->CreateDefaultWindowTreeHosts();
}

uint32_t ConnectionManager::GenerateWindowManagerChangeId(
    WindowTreeImpl* source,
    uint32_t client_change_id) {
  const uint32_t wm_change_id = next_wm_change_id_++;
  in_flight_wm_change_map_[wm_change_id] = {source->id(), client_change_id};
  return wm_change_id;
}

void ConnectionManager::WindowManagerChangeCompleted(
    uint32_t window_manager_change_id,
    bool success) {
  InFlightWindowManagerChange change;
  if (!GetAndClearInFlightWindowManagerChange(window_manager_change_id,
                                              &change)) {
    return;
  }

  WindowTreeImpl* connection = GetConnection(change.connection_id);
  connection->OnChangeCompleted(change.client_change_id, success);
}

void ConnectionManager::WindowManagerCreatedTopLevelWindow(
    WindowTreeImpl* wm_connection,
    uint32_t window_manager_change_id,
    const ServerWindow* window) {
  InFlightWindowManagerChange change;
  if (!GetAndClearInFlightWindowManagerChange(window_manager_change_id,
                                              &change)) {
    return;
  }
  if (!window) {
    WindowManagerSentBogusMessage();
    return;
  }

  WindowTreeImpl* connection = GetConnection(change.connection_id);
  // The window manager should have created the window already, and it should
  // be ready for embedding.
  if (!connection->IsWaitingForNewTopLevelWindow(window_manager_change_id) ||
      !window || window->id().connection_id != wm_connection->id() ||
      !window->children().empty() || GetConnectionWithRoot(window)) {
    WindowManagerSentBogusMessage();
    return;
  }

  connection->OnWindowManagerCreatedTopLevelWindow(
      window_manager_change_id, change.client_change_id, window);
}

mojom::DisplayPtr ConnectionManager::DisplayForHost(WindowTreeHostImpl* host) {
  size_t i = 0;
  int next_x = 0;
  for (WindowTreeHostImpl* host2 : hosts_) {
    const ServerWindow* root = host->root_window();
    if (host == host2) {
      mojom::DisplayPtr display = mojom::Display::New();
      display = mojom::Display::New();
      display->id = host->id();
      display->bounds = mojo::Rect::New();
      display->bounds->x = next_x;
      display->bounds->y = 0;
      display->bounds->width = root->bounds().size().width();
      display->bounds->height = root->bounds().size().height();
      // TODO(sky): window manager needs an API to set the work area.
      display->work_area = display->bounds.Clone();
      display->device_pixel_ratio =
          host->GetViewportMetrics().device_pixel_ratio;
      display->rotation = host->GetRotation();
      // TODO(sky): make this real.
      display->is_primary = i == 0;
      // TODO(sky): make this real.
      display->touch_support = mojom::TouchSupport::UNKNOWN;
      display->frame_decoration_values =
          host->frame_decoration_values().Clone();
      return display;
    }
    next_x += root->bounds().size().width();
    ++i;
  }
  NOTREACHED();
  return mojom::Display::New();
}

void ConnectionManager::ProcessWindowBoundsChanged(
    const ServerWindow* window,
    const gfx::Rect& old_bounds,
    const gfx::Rect& new_bounds) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessWindowBoundsChanged(window, old_bounds, new_bounds,
                                            IsOperationSource(pair.first));
  }
}

void ConnectionManager::ProcessClientAreaChanged(
    const ServerWindow* window,
    const gfx::Insets& new_client_area,
    const std::vector<gfx::Rect>& new_additional_client_areas) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessClientAreaChanged(window, new_client_area,
                                          new_additional_client_areas,
                                          IsOperationSource(pair.first));
  }
}

void ConnectionManager::ProcessLostCapture(const ServerWindow* window) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessLostCapture(window, IsOperationSource(pair.first));
  }
}

void ConnectionManager::ProcessWillChangeWindowHierarchy(
    const ServerWindow* window,
    const ServerWindow* new_parent,
    const ServerWindow* old_parent) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessWillChangeWindowHierarchy(
        window, new_parent, old_parent, IsOperationSource(pair.first));
  }
}

void ConnectionManager::ProcessWindowHierarchyChanged(
    const ServerWindow* window,
    const ServerWindow* new_parent,
    const ServerWindow* old_parent) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessWindowHierarchyChanged(window, new_parent, old_parent,
                                               IsOperationSource(pair.first));
  }
}

void ConnectionManager::ProcessWindowReorder(
    const ServerWindow* window,
    const ServerWindow* relative_window,
    const mojom::OrderDirection direction) {
  // We'll probably do a bit of reshuffling when we add a transient window.
  if ((current_operation_type() == OperationType::ADD_TRANSIENT_WINDOW) ||
      (current_operation_type() ==
       OperationType::REMOVE_TRANSIENT_WINDOW_FROM_PARENT)) {
    return;
  }
  for (auto& pair : tree_map_) {
    pair.second->ProcessWindowReorder(window, relative_window, direction,
                                      IsOperationSource(pair.first));
  }
}

void ConnectionManager::ProcessWindowDeleted(const ServerWindow* window) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessWindowDeleted(window, IsOperationSource(pair.first));
  }
}

void ConnectionManager::ProcessWillChangeWindowPredefinedCursor(
    ServerWindow* window,
    int32_t cursor_id) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessCursorChanged(window, cursor_id,
                                      IsOperationSource(pair.first));
  }

  // Pass the cursor change to the native window.
  WindowTreeHostImpl* host = GetWindowTreeHostByWindow(window);
  if (host)
    host->OnCursorUpdated(window);
}

void ConnectionManager::ProcessViewportMetricsChanged(
    WindowTreeHostImpl* host,
    const mojom::ViewportMetrics& old_metrics,
    const mojom::ViewportMetrics& new_metrics) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessViewportMetricsChanged(host, old_metrics, new_metrics,
                                               IsOperationSource(pair.first));
  }

  if (!got_valid_frame_decorations_)
    return;
}

void ConnectionManager::ProcessFrameDecorationValuesChanged(
    WindowTreeHostImpl* host) {
  if (!got_valid_frame_decorations_) {
    got_valid_frame_decorations_ = true;
    display_manager_observers_.ForAllPtrs([this](
        mojom::DisplayManagerObserver* observer) { CallOnDisplays(observer); });
    return;
  }

  display_manager_observers_.ForAllPtrs(
      [this, &host](mojom::DisplayManagerObserver* observer) {
        CallOnDisplayChanged(observer, host);
      });
}

bool ConnectionManager::GetAndClearInFlightWindowManagerChange(
    uint32_t window_manager_change_id,
    InFlightWindowManagerChange* change) {
  // There are valid reasons as to why we wouldn't know about the id. The
  // most likely is the client disconnected before the response from the window
  // manager came back.
  auto iter = in_flight_wm_change_map_.find(window_manager_change_id);
  if (iter == in_flight_wm_change_map_.end())
    return false;

  *change = iter->second;
  in_flight_wm_change_map_.erase(iter);
  return true;
}

void ConnectionManager::PrepareForOperation(Operation* op) {
  // Should only ever have one change in flight.
  CHECK(!current_operation_);
  current_operation_ = op;
}

void ConnectionManager::FinishOperation() {
  // PrepareForOperation/FinishOperation should be balanced.
  CHECK(current_operation_);
  current_operation_ = nullptr;
}

void ConnectionManager::MaybeUpdateNativeCursor(ServerWindow* window) {
  // This can be null in unit tests.
  WindowTreeHostImpl* impl = GetWindowTreeHostByWindow(window);
  if (impl)
    impl->MaybeChangeCursorOnWindowTreeChange();
}

void ConnectionManager::CallOnDisplays(
    mojom::DisplayManagerObserver* observer) {
  mojo::Array<mojom::DisplayPtr> displays(hosts_.size());
  {
    size_t i = 0;
    // TODO(sky): need ordering!
    for (WindowTreeHostImpl* host : hosts_) {
      displays[i] = DisplayForHost(host);
      ++i;
    }
  }
  observer->OnDisplays(std::move(displays));
}

void ConnectionManager::CallOnDisplayChanged(
    mojom::DisplayManagerObserver* observer,
    WindowTreeHostImpl* host) {
  mojo::Array<mojom::DisplayPtr> displays(1);
  displays[0] = DisplayForHost(host);
  display_manager_observers_.ForAllPtrs(
      [&displays](mojom::DisplayManagerObserver* observer) {
        observer->OnDisplaysChanged(displays.Clone());
      });
}

mus::SurfacesState* ConnectionManager::GetSurfacesState() {
  return surfaces_state_.get();
}

void ConnectionManager::OnScheduleWindowPaint(const ServerWindow* window) {
  if (!in_destructor_)
    SchedulePaint(window, gfx::Rect(window->bounds().size()));
}

const ServerWindow* ConnectionManager::GetRootWindow(
    const ServerWindow* window) const {
  const WindowTreeHostImpl* host = GetWindowTreeHostByWindow(window);
  return host ? host->root_window() : nullptr;
}

void ConnectionManager::ScheduleSurfaceDestruction(ServerWindow* window) {
  for (WindowTreeHostImpl* host : hosts_) {
    if (host->root_window()->Contains(window)) {
      host->ScheduleSurfaceDestruction(window);
      break;
    }
  }
}

ServerWindow* ConnectionManager::FindWindowForSurface(
    const ServerWindow* ancestor,
    mojom::SurfaceType surface_type,
    const ClientWindowId& client_window_id) {
  WindowTreeImpl* window_tree;
  if (ancestor->id().connection_id == kInvalidConnectionId) {
    WindowManagerAndHost wm_and_host = GetWindowManagerAndHost(ancestor);
    window_tree = wm_and_host.window_manager_state
                      ? wm_and_host.window_manager_state->tree()
                      : nullptr;
  } else {
    window_tree = GetConnection(ancestor->id().connection_id);
  }
  if (!window_tree)
    return nullptr;
  if (surface_type == mojom::SurfaceType::DEFAULT) {
    // At embed points the default surface comes from the embedded app.
    WindowTreeImpl* connection_with_root = GetConnectionWithRoot(ancestor);
    if (connection_with_root)
      window_tree = connection_with_root;
  }
  return window_tree->GetWindowByClientId(client_window_id);
}

void ConnectionManager::OnWindowDestroyed(ServerWindow* window) {
  if (!in_destructor_)
    ProcessWindowDeleted(window);
}

void ConnectionManager::OnWillChangeWindowHierarchy(ServerWindow* window,
                                                    ServerWindow* new_parent,
                                                    ServerWindow* old_parent) {
  if (in_destructor_)
    return;

  ProcessWillChangeWindowHierarchy(window, new_parent, old_parent);
}

void ConnectionManager::OnWindowHierarchyChanged(ServerWindow* window,
                                                 ServerWindow* new_parent,
                                                 ServerWindow* old_parent) {
  if (in_destructor_)
    return;

  ProcessWindowHierarchyChanged(window, new_parent, old_parent);

  // TODO(beng): optimize.
  if (old_parent)
    SchedulePaint(old_parent, gfx::Rect(old_parent->bounds().size()));
  if (new_parent)
    SchedulePaint(new_parent, gfx::Rect(new_parent->bounds().size()));

  MaybeUpdateNativeCursor(window);
}

void ConnectionManager::OnWindowBoundsChanged(ServerWindow* window,
                                              const gfx::Rect& old_bounds,
                                              const gfx::Rect& new_bounds) {
  if (in_destructor_)
    return;

  ProcessWindowBoundsChanged(window, old_bounds, new_bounds);
  if (!window->parent())
    return;

  // TODO(sky): optimize this.
  SchedulePaint(window->parent(), old_bounds);
  SchedulePaint(window->parent(), new_bounds);

  MaybeUpdateNativeCursor(window);
}

void ConnectionManager::OnWindowClientAreaChanged(
    ServerWindow* window,
    const gfx::Insets& new_client_area,
    const std::vector<gfx::Rect>& new_additional_client_areas) {
  if (in_destructor_)
    return;

  ProcessClientAreaChanged(window, new_client_area,
                           new_additional_client_areas);
}

void ConnectionManager::OnWindowReordered(ServerWindow* window,
                                          ServerWindow* relative,
                                          mojom::OrderDirection direction) {
  ProcessWindowReorder(window, relative, direction);
  if (!in_destructor_)
    SchedulePaint(window, gfx::Rect(window->bounds().size()));
  MaybeUpdateNativeCursor(window);
}

void ConnectionManager::OnWillChangeWindowVisibility(ServerWindow* window) {
  if (in_destructor_)
    return;

  // Need to repaint if the window was drawn (which means it's in the process of
  // hiding) or the window is transitioning to drawn.
  if (window->parent() &&
      (window->IsDrawn() ||
       (!window->visible() && window->parent()->IsDrawn()))) {
    SchedulePaint(window->parent(), window->bounds());
  }

  for (auto& pair : tree_map_) {
    pair.second->ProcessWillChangeWindowVisibility(
        window, IsOperationSource(pair.first));
  }
}

void ConnectionManager::OnWindowPredefinedCursorChanged(ServerWindow* window,
                                                        int32_t cursor_id) {
  if (in_destructor_)
    return;

  ProcessWillChangeWindowPredefinedCursor(window, cursor_id);
}

void ConnectionManager::OnWindowSharedPropertyChanged(
    ServerWindow* window,
    const std::string& name,
    const std::vector<uint8_t>* new_data) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessWindowPropertyChanged(window, name, new_data,
                                              IsOperationSource(pair.first));
  }
}

void ConnectionManager::OnWindowTextInputStateChanged(
    ServerWindow* window,
    const ui::TextInputState& state) {
  WindowTreeHostImpl* host = GetWindowTreeHostByWindow(window);
  host->UpdateTextInputState(window, state);
}

void ConnectionManager::OnTransientWindowAdded(ServerWindow* window,
                                               ServerWindow* transient_child) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessTransientWindowAdded(window, transient_child,
                                             IsOperationSource(pair.first));
  }
}

void ConnectionManager::OnTransientWindowRemoved(
    ServerWindow* window,
    ServerWindow* transient_child) {
  // If we're deleting a window, then this is a superfluous message.
  if (current_operation_type() == OperationType::DELETE_WINDOW)
    return;
  for (auto& pair : tree_map_) {
    pair.second->ProcessTransientWindowRemoved(window, transient_child,
                                               IsOperationSource(pair.first));
  }
}

void ConnectionManager::AddObserver(mojom::DisplayManagerObserverPtr observer) {
  // TODO(sky): this needs to be per user.

  // Many clients key off the frame decorations to size widgets. Wait for frame
  // decorations before notifying so that we don't have to worry about clients
  // resizing appropriately.
  if (!got_valid_frame_decorations_) {
    display_manager_observers_.AddInterfacePtr(std::move(observer));
    return;
  }
  CallOnDisplays(observer.get());
  display_manager_observers_.AddInterfacePtr(std::move(observer));
}

}  // namespace ws
}  // namespace mus
