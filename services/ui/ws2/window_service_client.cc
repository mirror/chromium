// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/window_service_client.h"

#include "base/auto_reset.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#include "components/viz/common/surfaces/surface_info.h"
#include "mojo/public/cpp/bindings/map.h"
#include "services/ui/ws2/window_data.h"
#include "services/ui/ws2/window_service.h"
#include "services/ui/ws2/window_service_client_binding.h"
#include "services/ui/ws2/window_service_delegate.h"
#include "ui/aura/client/focus_client.h"
#include "ui/aura/client/transient_window_client.h"
#include "ui/aura/mus/client_surface_embedder.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"
#include "ui/compositor/dip_util.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_type.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace ui {
namespace ws2 {
namespace {

std::string DebugWindowId(const aura::Window* window) {
  const WindowData* window_data = window ? WindowData::Get(window) : nullptr;
  return window_data ? window_data->window_id().ToString() : "null";
}

}  // namespace

// TODO: move into own header and rename ClientRoot.
class WindowServiceClient::WindowEmbedding : public aura::WindowObserver {
 public:
  WindowEmbedding(WindowServiceClient* window_service_client,
                  aura::Window* window)
      : window_service_client_(window_service_client), window_(window) {
    window_->AddObserver(this);
    // TODO: WindowService should maintain insets (insets are the normal client
    // insets).
    // TODO: this may only be needed for first level embedding. Verify that.
    client_surface_embedder_ = std::make_unique<aura::ClientSurfaceEmbedder>(
        window_,
        window_service_client->connection_type_ ==
            WindowServiceClient::ConnectionType::kOther,
        gfx::Insets());
  }

  ~WindowEmbedding() override {
    WindowData::Get(window_)->set_embedded_window_service_client(nullptr);
    window_->RemoveObserver(this);
  }

  void FrameSinkIdChanged() {
    window_->set_embed_frame_sink_id(WindowData::Get(window_)->frame_sink_id());
    UpdatePrimarySurfaceId();
  }

  aura::Window* window() { return window_; }

  const viz::LocalSurfaceId& GetLocalSurfaceId() {
    gfx::Size size_in_pixels =
        ui::ConvertSizeToPixel(window_->layer(), window_->bounds().size());
    if (last_surface_size_in_pixels_ != size_in_pixels ||
        !local_surface_id_.is_valid()) {
      local_surface_id_ = parent_local_surface_id_allocator_.GenerateId();
      last_surface_size_in_pixels_ = size_in_pixels;
    }
    return local_surface_id_;
  }

 private:
  void UpdatePrimarySurfaceId() {
    client_surface_embedder_->SetPrimarySurfaceId(
        viz::SurfaceId(window_->embed_frame_sink_id(), GetLocalSurfaceId()));
  }

  // aura::WindowObserver:
  void OnWindowDestroyed(aura::Window* window) override {
    // OnWindowEmbeddingWindowDestroyed() delete this, so no need to remove
    // observers or null out |window_|.
    window_service_client_->DeleteWindowEmbedding(
        this, WindowServiceClient::DeleteWindowEmbeddingReason::kDeleted);
  }
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override {
    UpdatePrimarySurfaceId();
    LOG(ERROR) << "On top level bounds changed " << new_bounds.ToString()
               << " surface_id=" << local_surface_id_.ToString()
               << " size=" << last_surface_size_in_pixels_.ToString()
               << " frame_sink=" << window_->embed_frame_sink_id().ToString();
    client_surface_embedder_->UpdateSizeAndGutters();
    base::Optional<viz::LocalSurfaceId> surface_id = local_surface_id_;
    window_service_client_->window_tree_client_->OnWindowBoundsChanged(
        window_service_client_->TransportIdForWindow(window), old_bounds,
        new_bounds, std::move(surface_id));
  }
  // TODO: add MORE OVERRIDES.

 private:
  WindowServiceClient* window_service_client_;
  aura::Window* window_;
  gfx::Size last_surface_size_in_pixels_;
  viz::LocalSurfaceId local_surface_id_;
  viz::ParentLocalSurfaceIdAllocator parent_local_surface_id_allocator_;
  std::unique_ptr<aura::ClientSurfaceEmbedder> client_surface_embedder_;

  DISALLOW_COPY_AND_ASSIGN(WindowEmbedding);
};

WindowServiceClient::WindowServiceClient(WindowService* window_service,
                                         ClientSpecificId client_id,
                                         mojom::WindowTreeClient* client,
                                         bool intercepts_events)
    : window_service_(window_service),
      client_id_(client_id),
      window_tree_client_(client),
      intercepts_events_(intercepts_events) {}

void WindowServiceClient::InitForEmbedding(
    aura::Window* root,
    mojom::WindowTreePtr window_tree_ptr) {
  CreateWindowEmbedding(root, std::move(window_tree_ptr));
}

void WindowServiceClient::InitFromFactory() {
  connection_type_ = ConnectionType::kOther;
  LOG(ERROR) << "InitFromFactory";
}

WindowServiceClient::~WindowServiceClient() {
  // Delete any WindowServiceClients created as a result of Embed(). There's
  // no point in having them outlive us given all the windows they were embedded
  // in have been destroyed.
  embedded_client_bindings_.clear();

  // Delete the embeddings first, that way we don't attempt to notify the client
  // when the windows the client created are deleted.
  while (!window_embeddings_.empty()) {
    DeleteWindowEmbedding(window_embeddings_.begin()->get(),
                          DeleteWindowEmbeddingReason::kDestructor);
  }

  // Delete any remaining windows created by this client, but do it in such a
  // way that the client isn't notified.
  while (!client_created_windows_.empty()) {
    aura::Window* window = *client_created_windows_.begin();
    UnregisterWindow(window);
    // UnregisterWindow() should make it such that the Window is no longer
    // recognized as being created (owned) by this client.
    DCHECK(!DidCreateWindow(window));
    delete window;
  }
}

WindowServiceClient::WindowEmbedding*
WindowServiceClient::CreateWindowEmbedding(aura::Window* window,
                                           mojom::WindowTreePtr window_tree) {
  PrepareForEmbedAndDetachExistingEmbedding(window);

  WindowData* window_data = window_service_->GetWindowDataForWindow(window);
  window_data->set_embedded_window_service_client(this);
  const ClientWindowId client_window_id = window_data->frame_sink_id();
  RegisterWindow(window, client_window_id);

  std::unique_ptr<WindowEmbedding> embedding_ptr =
      std::make_unique<WindowEmbedding>(this, window);
  WindowEmbedding* embedding = embedding_ptr.get();
  window_embeddings_.push_back(std::move(embedding_ptr));

  const bool for_embed = window_tree.is_bound();

  if (for_embed) {
    int64_t display_id = display::kInvalidDisplayId;
    ClientWindowId focused_window_id;
    aura::WindowTreeHost* window_tree_host = window->GetHost();
    if (window_tree_host) {
      display_id =
          display::Screen::GetScreen()->GetDisplayNearestWindow(window).id();
      aura::client::FocusClient* focus_client =
          aura::client::GetFocusClient(window);
      aura::Window* focused_window =
          focus_client ? focus_client->GetFocusedWindow() : nullptr;
      if (focused_window && IsWindowKnown(focused_window))
        focused_window_id = window_to_client_window_id_[focused_window];
    }

    const bool drawn = window_tree_host && window->IsVisible();
    window_tree_client_->OnEmbed(WindowToWindowData(window),
                                 std::move(window_tree), display_id,
                                 ClientWindowIdToTransportId(focused_window_id),
                                 drawn, embedding->GetLocalSurfaceId());

    // Reset the frame sink id (after calling OnEmbed()). This is needed for
    // hit-testing, so that we can lookup window ids later.
    window_data->SetFrameSinkId(ClientWindowId(client_id_, 0));
  }

  embedding->FrameSinkIdChanged();

  // Requests for top-levels don't get OnFrameSinkIdAllocated().
  if (for_embed) {
    window_tree_client_->OnFrameSinkIdAllocated(
        ClientWindowIdToTransportId(client_window_id),
        window_data->frame_sink_id());
  }

  return embedding;
}

void WindowServiceClient::DeleteWindowEmbedding(
    WindowEmbedding* window_embedding,
    DeleteWindowEmbeddingReason reason) {
  aura::Window* window = window_embedding->window();
  // Delete the WindowEmbedding first, so that we don't attempt to spam the
  // client with a bunch of notifications.
  auto iter = FindWindowEmbeddingWithRoot(window_embedding->window());
  DCHECK(iter != window_embeddings_.end());
  window_embeddings_.erase(iter);

  const Id client_window_id = TransportIdForWindow(window);

  if (reason == DeleteWindowEmbeddingReason::kEmbed) {
    window_tree_client_->OnUnembed(client_window_id);
    window_tree_client_->OnWindowDeleted(client_window_id);
  }

  // This client no longer knows about |window|. Unparent any windows that
  // were created by this client and parented to windows in |window|.
  std::vector<aura::Window*> created_windows;
  UnregisterRecursive(window, &created_windows);
  for (aura::Window* created_window : created_windows)
    created_window->parent()->RemoveChild(created_window);

  if (reason == DeleteWindowEmbeddingReason::kUnembed) {
    // Notify the owner of the window it no longer has a client embedded in it.
    // Owner is null in the case of the windowmanager unembedding itself from
    // a root.
    WindowData* window_data = WindowData::Get(window);
    if (window_data->owning_window_service_client()) {
      window_data->owning_window_service_client()
          ->window_tree_client_->OnEmbeddedAppDisconnected(
              window_data->owning_window_service_client()->TransportIdForWindow(
                  window));
    }
  }
}

void WindowServiceClient::DeleteWindowEmbeddingWithRoot(aura::Window* window) {
  auto iter = FindWindowEmbeddingWithRoot(window);
  if (iter == window_embeddings_.end())
    return;

  DeleteWindowEmbedding(iter->get(), DeleteWindowEmbeddingReason::kEmbed);
}

aura::Window* WindowServiceClient::GetWindowByClientId(
    const ClientWindowId& id) {
  auto iter = client_window_id_to_window_map_.find(id);
  return iter == client_window_id_to_window_map_.end() ? nullptr : iter->second;
}

aura::Window* WindowServiceClient::AddClientCreatedWindow(
    const ClientWindowId& id,
    std::unique_ptr<aura::Window> window_ptr,
    bool is_top_level) {
  aura::Window* window =
      *(client_created_windows_.insert(window_ptr.release()).first);
  WindowData::Create(window, is_top_level ? nullptr : this,
                     GenerateNewWindowId(), id);
  return window;
}

bool WindowServiceClient::DidCreateWindow(aura::Window* window) {
  return window && client_created_windows_.count(window) > 0u;
}

bool WindowServiceClient::IsRoot(aura::Window* window) {
  return window &&
         FindWindowEmbeddingWithRoot(window) != window_embeddings_.end();
}

WindowServiceClient::WindowEmbeddings::iterator
WindowServiceClient::FindWindowEmbeddingWithRoot(aura::Window* window) {
  for (auto iter = window_embeddings_.begin(); iter != window_embeddings_.end();
       ++iter) {
    if (iter->get()->window() == window)
      return iter;
  }
  return window_embeddings_.end();
}

bool WindowServiceClient::IsWindowKnown(aura::Window* window) const {
  return window && window_to_client_window_id_.count(window) > 0u;
}

bool WindowServiceClient::IsWindowRootOfAnotherClient(
    aura::Window* window) const {
  WindowData* window_data = WindowData::Get(window);
  return window_data &&
         window_data->embedded_window_service_client() != nullptr &&
         window_data->embedded_window_service_client() != this;
}

void WindowServiceClient::RegisterWindow(aura::Window* window,
                                         const ClientWindowId& id) {
  LOG(ERROR) << "Registering window for client, id=" << id.ToString()
             << " id=" << client_id_;
  window_to_client_window_id_[window] = id;

  DCHECK(IsWindowKnown(window));
  client_window_id_to_window_map_[id] = window;
  if (DidCreateWindow(window))
    window->AddObserver(this);
}

void WindowServiceClient::UnregisterWindow(aura::Window* window) {
  DCHECK(IsWindowKnown(window));
  LOG(ERROR) << "Unregister window for client, id="
             << window_to_client_window_id_[window].ToString()
             << " client=" << client_id_;
  WindowData* window_data = WindowData::Get(window);
  if (window_data && window_data->embedded_window_service_client() == this)
    window_data->set_embedded_window_service_client(nullptr);
  auto iter = window_to_client_window_id_.find(window);
  client_window_id_to_window_map_.erase(iter->second);
  window_to_client_window_id_.erase(iter);
  if (client_created_windows_.erase(window))
    window->RemoveObserver(this);
}

WindowId WindowServiceClient::GenerateNewWindowId() {
  const WindowId result = WindowId(client_id_, next_window_id_++);
  CHECK_NE(0, next_window_id_);
  return result;
}

bool WindowServiceClient::IsValidIdForNewWindow(
    const ClientWindowId& id) const {
  // Reserve 0 to indicate a null window.
  return client_window_id_to_window_map_.count(id) == 0u &&
         base::checked_cast<ClientSpecificId>(id.client_id()) == client_id_ &&
         id != ClientWindowId();
}

Id WindowServiceClient::TransportIdForWindow(aura::Window* window) const {
  DCHECK(IsWindowKnown(window));
  auto iter = window_to_client_window_id_.find(window);
  DCHECK(iter != window_to_client_window_id_.end());
  return ClientWindowIdToTransportId(iter->second);
}

Id WindowServiceClient::ClientWindowIdToTransportId(
    const ClientWindowId& client_window_id) const {
  if (client_window_id.client_id() == client_id_)
    return client_window_id.sink_id();
  return (base::checked_cast<ClientSpecificId>(client_window_id.client_id())
          << 16) |
         base::checked_cast<ClientSpecificId>(client_window_id.sink_id());
}

ClientWindowId WindowServiceClient::MakeClientWindowId(
    Id transport_window_id) const {
  if (!HiWord(transport_window_id))
    return ClientWindowId(client_id_, transport_window_id);
  return ClientWindowId(HiWord(transport_window_id),
                        LoWord(transport_window_id));
}

void WindowServiceClient::UnregisterRecursive(
    aura::Window* window,
    std::vector<aura::Window*>* created_windows) {
  if (DidCreateWindow(window)) {
    // Stop iterating at windows created by this client. We assume the client
    // will keep seeing any descendants.
    if (created_windows)
      created_windows->push_back(window);
    return;
  }

  if (IsWindowKnown(window))
    UnregisterWindow(window);

  for (aura::Window* child : window->children())
    UnregisterRecursive(child, created_windows);
}

std::vector<mojom::WindowDataPtr> WindowServiceClient::WindowsToWindowDatas(
    const std::vector<aura::Window*>& windows) {
  std::vector<mojom::WindowDataPtr> array(windows.size());
  for (size_t i = 0; i < windows.size(); ++i)
    array[i] = WindowToWindowData(windows[i]);
  return array;
}

mojom::WindowDataPtr WindowServiceClient::WindowToWindowData(
    aura::Window* window) {
  aura::Window* parent = window->parent();
  aura::Window* transient_parent =
      aura::client::GetTransientWindowClient()->GetTransientParent(window);

  // If the parent or transient parent isn't known, it means it is not visible
  // to the client and should not be sent over.
  if (!IsWindowKnown(parent))
    parent = nullptr;
  if (!IsWindowKnown(transient_parent))
    transient_parent = nullptr;
  mojom::WindowDataPtr window_data(mojom::WindowData::New());
  window_data->parent_id =
      parent ? TransportIdForWindow(parent) : kInvalidTransportId;
  window_data->window_id =
      window ? TransportIdForWindow(window) : kInvalidTransportId;
  window_data->transient_parent_id =
      transient_parent ? TransportIdForWindow(transient_parent)
                       : kInvalidTransportId;
  window_data->bounds = window->bounds();
  // XXX needs mapping!
  // window_data->properties = mojo::MapToUnorderedMap(window->properties());
  window_data->visible = window->TargetVisibility();
  return window_data;
}

bool WindowServiceClient::ShouldRouteToDelegate(aura::Window* window) {
  // Only route requests to the delegate for windows other than kEmbedding.
  if (connection_type_ == ConnectionType::kEmbedding)
    return false;

  WindowData* window_data = window ? WindowData::Get(window) : nullptr;
  return window_data && window_data->owning_window_service_client() == nullptr;
}

void WindowServiceClient::PrepareForEmbedAndDetachExistingEmbedding(
    aura::Window* window) {
  DCHECK(window);

  // When embedding there should be no children.
  while (!window->children().empty())
    window->RemoveChild(window->children().front());

  // Only one client maybe embedded in a window at a time.
  WindowData* window_data = window_service_->GetWindowDataForWindow(window);
  if (!window_data->embedded_window_service_client())
    return;

  window_data->embedded_window_service_client()->DeleteWindowEmbeddingWithRoot(
      window);
}

bool WindowServiceClient::NewWindowImpl(
    const ClientWindowId& client_window_id,
    const std::map<std::string, std::vector<uint8_t>>& properties) {
  DVLOG(3) << "new window client=" << client_id_
           << " window_id=" << client_window_id.ToString();
  if (!IsValidIdForNewWindow(client_window_id)) {
    DVLOG(1) << "NewWindow failed (id is not valid for client)";
    return false;
  }
  DCHECK(!GetWindowByClientId(client_window_id));
  const bool is_top_level = false;
  aura::Window* window = AddClientCreatedWindow(
      client_window_id, std::make_unique<aura::Window>(nullptr), is_top_level);
  window->Init(LAYER_NOT_DRAWN);
  // Windows created by the client should only be destroyed by the client.
  window->set_owned_by_parent(false);
  RegisterWindow(window, client_window_id);
  return true;
}

bool WindowServiceClient::DeleteWindowImpl(const ClientWindowId& window_id) {
  aura::Window* window = GetWindowByClientId(window_id);
  DVLOG(3) << "removing window from parent client=" << client_id_
           << " client window_id= " << window_id.ToString()
           << " global window_id=" << DebugWindowId(window);
  if (!window)
    return false;

  auto iter = FindWindowEmbeddingWithRoot(window);
  if (iter != window_embeddings_.end()) {
    DeleteWindowEmbedding(iter->get(), DeleteWindowEmbeddingReason::kUnembed);
    return true;
  }

  if (!DidCreateWindow(window))
    return false;

  {
    base::AutoReset<aura::Window*> auto_reset(&window_deleting_, window);
    // This triggers the necessary cleanup.
    delete window;
  }
  return true;
}

bool WindowServiceClient::AddWindowImpl(const ClientWindowId& parent_id,
                                        const ClientWindowId& child_id) {
  aura::Window* parent = GetWindowByClientId(parent_id);
  aura::Window* child = GetWindowByClientId(child_id);
  DVLOG(3) << "add window client=" << client_id_
           << " client parent window_id=" << parent_id.ToString()
           << " global window_id=" << DebugWindowId(parent)
           << " client child window_id= " << child_id.ToString()
           << " global window_id=" << DebugWindowId(child);
  if (!parent) {
    DVLOG(1) << "AddWindow failed (no parent)";
    return false;
  }
  if (!child) {
    DVLOG(1) << "AddWindow failed (no child)";
    return false;
  }
  if (child->parent() == parent) {
    DVLOG(1) << "AddWindow failed (already has parent)";
    return false;
  }
  if (child->Contains(parent)) {
    DVLOG(1) << "AddWindow failed (child contains parent)";
    return false;
  }
  if (DidCreateWindow(child) &&
      (IsRoot(parent) ||
       (DidCreateWindow(parent) && !IsWindowRootOfAnotherClient(parent)))) {
    parent->AddChild(child);
    return true;
  }
  DVLOG(1) << "AddWindow failed (access denied)";
  return false;
}

bool WindowServiceClient::RemoveWindowFromParentImpl(
    const ClientWindowId& client_window_id) {
  aura::Window* window = GetWindowByClientId(client_window_id);
  DVLOG(3) << "removing window from parent client=" << client_id_
           << " client window_id= " << client_window_id
           << " global window_id=" << DebugWindowId(window);
  if (!window) {
    DVLOG(1) << "RemoveWindowFromParent failed (invalid window id="
             << client_window_id.ToString() << ")";
    return false;
  }
  if (!window->parent()) {
    DVLOG(1) << "RemoveWindowFromParent failed (no parent id="
             << client_window_id.ToString() << ")";
    return false;
  }
  if (DidCreateWindow(window) || IsRoot(window)) {
    window->parent()->RemoveChild(window);
    return true;
  }
  DVLOG(1) << "RemoveWindowFromParent failed (access policy disallowed id="
           << client_window_id.ToString() << ")";
  return false;
}

bool WindowServiceClient::SetWindowVisibilityImpl(
    const ClientWindowId& window_id,
    bool visible) {
  aura::Window* window = GetWindowByClientId(window_id);
  DVLOG(3) << "SetWindowVisibility client=" << client_id_
           << " client window_id= " << window_id.ToString()
           << " global window_id=" << DebugWindowId(window);
  if (!window) {
    DVLOG(1) << "SetWindowVisibility failed (no window)";
    return false;
  }
  if ((DidCreateWindow(window) || IsRoot(window)) &&
      (!IsRoot(window) || can_change_root_window_visibility_)) {
    if (window->TargetVisibility() == visible)
      return true;
    if (visible)
      window->Show();
    else
      window->Hide();
    return true;
  }
  DVLOG(1) << "SetWindowVisibility failed (access policy denied change)";
  return false;
}

bool WindowServiceClient::SetWindowOpacityImpl(const ClientWindowId& window_id,
                                               float opacity) {
  aura::Window* window = GetWindowByClientId(window_id);
  if (!DidCreateWindow(window) || !IsRoot(window))
    return false;
  if (window->layer()->opacity() == opacity)
    return true;
  window->layer()->SetOpacity(opacity);
  return true;
}

bool WindowServiceClient::SetWindowBoundsImpl(
    const ClientWindowId& window_id,
    const gfx::Rect& bounds,
    const base::Optional<viz::LocalSurfaceId>& local_surface_id) {
  aura::Window* window = GetWindowByClientId(window_id);

  DVLOG(3) << "SetWindowBounds window_id=" << window_id
           << " global window_id=" << DebugWindowId(window)
           << " bounds=" << bounds.ToString() << " local_surface_id="
           << (local_surface_id ? local_surface_id->ToString() : "null");

  if (!window) {
    DVLOG(1) << "SetWindowBounds failed (invalid window id)";
    return false;
  }

  WindowData* window_data = WindowData::Get(window);
  LOG(ERROR) << " SetWindowBounds wd=" << window_data << " owning_wsc="
             << (window_data ? window_data->owning_window_service_client()
                             : nullptr)
             << " this=" << this << " route=" << ShouldRouteToDelegate(window);

  if (ShouldRouteToDelegate(window)) {
    DVLOG(3) << "Redirecting request to change bounds for "
             << DebugWindowId(window) << " to window manager...";
    window_service_->delegate()->ClientRequestedWindowBoundsChange(window,
                                                                   bounds);
    // Return false for success, which triggers the client reverting to
    // whatever value the delegate changed the bounds to.
    return false;
  }

  // Only the owner of the window can change the bounds.
  if (!DidCreateWindow(window))
    return false;

  window->SetBounds(bounds);
  // TODO: figure out if need to pass through local_surface_id. Doesn't matter
  // in all cases.
  return true;
}

bool WindowServiceClient::EmbedImpl(
    const ClientWindowId& window_id,
    mojom::WindowTreeClientPtr window_tree_client,
    uint32_t flags) {
  // TODO: support mojom::kEmbedFlagEmbedderControlsVisibility.
  NOTIMPLEMENTED();

  aura::Window* window = GetWindowByClientId(window_id);
  if (!window || !DidCreateWindow(window))
    return false;

  // mojom::kEmbedFlagEmbedderInterceptsEvents is inherited, otherwise an
  // embedder could effectively circumvent it by embedding itself.
  const bool intercepts_events = intercepts_events_;

  // When embedding we don't know the user id of where the TreeClient came
  // from. Use an invalid id, which limits what the client is able to do.
  auto new_client_binding = std::make_unique<WindowServiceClientBinding>();
  new_client_binding->InitForEmbed(window_service_,
                                   std::move(window_tree_client),
                                   intercepts_events, window);

  if (flags & mojom::kEmbedFlagEmbedderControlsVisibility) {
    new_client_binding->window_service_client_
        ->can_change_root_window_visibility_ = false;
  }

  embedded_client_bindings_.push_back(std::move(new_client_binding));
  return true;
}

std::vector<aura::Window*> WindowServiceClient::GetWindowTreeImpl(
    const ClientWindowId& window_id) {
  aura::Window* window = GetWindowByClientId(window_id);
  std::vector<aura::Window*> results;
  GetWindowTreeRecursive(window, &results);
  return results;
}

void WindowServiceClient::GetWindowTreeRecursive(
    aura::Window* window,
    std::vector<aura::Window*>* windows) {
  if (!IsWindowKnown(window))
    return;

  windows->push_back(window);
  for (aura::Window* child : window->children())
    GetWindowTreeRecursive(child, windows);
}

void WindowServiceClient::OnWindowDestroyed(aura::Window* window) {
  DCHECK(IsWindowKnown(window));

  if (window != window_deleting_) {
    DCHECK_NE(0u, window_to_client_window_id_.count(window));
    const ClientWindowId client_window_id = window_to_client_window_id_[window];
    window_tree_client_->OnWindowDeleted(
        ClientWindowIdToTransportId(client_window_id));
  }

  UnregisterWindow(window);
}

void WindowServiceClient::NewWindow(
    uint32_t change_id,
    Id transport_window_id,
    const base::Optional<std::unordered_map<std::string, std::vector<uint8_t>>>&
        transport_properties) {
  // TODO: needs to map and validate |transport_properties|.
  std::map<std::string, std::vector<uint8_t>> properties;

  window_tree_client_->OnChangeCompleted(
      change_id,
      NewWindowImpl(MakeClientWindowId(transport_window_id), properties));
}

void WindowServiceClient::NewTopLevelWindow(
    uint32_t change_id,
    Id transport_window_id,
    const std::unordered_map<std::string, std::vector<uint8_t>>& properties) {
  const ClientWindowId client_window_id =
      MakeClientWindowId(transport_window_id);
  DVLOG(3) << "NewTopLevelWindow client_window_id="
           << client_window_id.ToString();
  if (!IsValidIdForNewWindow(client_window_id)) {
    DVLOG(1) << "NewTopLevelWindow failed (invalid window id)";
    window_tree_client_->OnChangeCompleted(change_id, false);
    return;
  }
  std::unique_ptr<aura::Window> top_level_ptr =
      window_service_->delegate()->NewTopLevel(properties);
  if (!top_level_ptr) {
    DVLOG(1) << "NewTopLevelWindow failed (delegate window creation failed)";
    window_tree_client_->OnChangeCompleted(change_id, false);
    return;
  }
  const bool is_top_level = true;
  aura::Window* top_level = AddClientCreatedWindow(
      client_window_id, std::move(top_level_ptr), is_top_level);
  const int64_t display_id =
      display::Screen::GetScreen()->GetDisplayNearestWindow(top_level).id();
  WindowEmbedding* embedding = CreateWindowEmbedding(top_level, nullptr);
  window_tree_client_->OnTopLevelCreated(
      change_id, WindowToWindowData(top_level), display_id,
      top_level->IsVisible(), embedding->GetLocalSurfaceId());
}

void WindowServiceClient::DeleteWindow(uint32_t change_id,
                                       Id transport_window_id) {
  window_tree_client_->OnChangeCompleted(
      change_id, DeleteWindowImpl(MakeClientWindowId(transport_window_id)));
}

void WindowServiceClient::SetCapture(uint32_t change_id, uint32_t window_id) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::ReleaseCapture(uint32_t change_id,
                                         uint32_t window_id) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::StartPointerWatcher(bool want_moves) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::StopPointerWatcher() {
  NOTIMPLEMENTED();
}

void WindowServiceClient::SetWindowBounds(
    uint32_t change_id,
    Id window_id,
    const gfx::Rect& bounds,
    const base::Optional<viz::LocalSurfaceId>& local_surface_id) {
  window_tree_client_->OnChangeCompleted(
      change_id, SetWindowBoundsImpl(MakeClientWindowId(window_id), bounds,
                                     local_surface_id));
}

void WindowServiceClient::SetWindowTransform(uint32_t change_id,
                                             uint32_t window_id,
                                             const gfx::Transform& transform) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::SetClientArea(
    uint32_t window_id,
    const gfx::Insets& insets,
    const base::Optional<std::vector<gfx::Rect>>& additional_client_areas) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::SetHitTestMask(
    uint32_t window_id,
    const base::Optional<gfx::Rect>& mask) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::SetCanAcceptDrops(uint32_t window_id,
                                            bool accepts_drops) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::SetWindowVisibility(uint32_t change_id,
                                              Id transport_window_id,
                                              bool visible) {
  window_tree_client_->OnChangeCompleted(
      change_id, SetWindowVisibilityImpl(
                     MakeClientWindowId(transport_window_id), visible));
}

void WindowServiceClient::SetWindowProperty(
    uint32_t change_id,
    uint32_t window_id,
    const std::string& name,
    const base::Optional<std::vector<uint8_t>>& value) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::SetWindowOpacity(uint32_t change_id,
                                           Id transport_window_id,
                                           float opacity) {
  window_tree_client_->OnChangeCompleted(
      change_id,
      SetWindowOpacityImpl(MakeClientWindowId(transport_window_id), opacity));
}

void WindowServiceClient::AttachCompositorFrameSink(
    Id transport_window_id,
    viz::mojom::CompositorFrameSinkRequest compositor_frame_sink,
    viz::mojom::CompositorFrameSinkClientPtr client) {
  DVLOG(3) << "AttachCompositorFrameSink id="
           << MakeClientWindowId(transport_window_id).ToString();
  aura::Window* window =
      GetWindowByClientId(MakeClientWindowId(transport_window_id));
  if (!window) {
    DVLOG(1) << "AttachCompositorFrameSink failed (invalid window id)";
    return;
  }
  WindowData* window_data = WindowData::Get(window);
  // If this isn't called on the root, then only allow it if there is not
  // another client embedded in the window.
  const bool allow = IsRoot(window) ||
                     (DidCreateWindow(window) &&
                      window_data->embedded_window_service_client() == nullptr);
  if (!allow) {
    DVLOG(1) << "AttachCompositorFrameSink failed (policy disallowed)";
    return;
  }

  window_data->AttachCompositorFrameSink(std::move(compositor_frame_sink),
                                         std::move(client));
}

void WindowServiceClient::AddWindow(uint32_t change_id,
                                    Id parent_id,
                                    Id child_id) {
  window_tree_client_->OnChangeCompleted(
      change_id, AddWindowImpl(MakeClientWindowId(parent_id),
                               MakeClientWindowId(child_id)));
}

void WindowServiceClient::RemoveWindowFromParent(uint32_t change_id,
                                                 Id window_id) {
  window_tree_client_->OnChangeCompleted(
      change_id, RemoveWindowFromParentImpl(MakeClientWindowId(window_id)));
}

void WindowServiceClient::AddTransientWindow(uint32_t change_id,
                                             uint32_t window_id,
                                             uint32_t transient_window_id) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::RemoveTransientWindowFromParent(
    uint32_t change_id,
    uint32_t transient_window_id) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::SetModalType(uint32_t change_id,
                                       uint32_t window_id,
                                       ui::ModalType type) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::SetChildModalParent(uint32_t change_id,
                                              uint32_t window_id,
                                              uint32_t parent_window_id) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::ReorderWindow(uint32_t change_id,
                                        uint32_t window_id,
                                        uint32_t relative_window_id,
                                        ::ui::mojom::OrderDirection direction) {
}

void WindowServiceClient::GetWindowTree(Id window_id,
                                        const GetWindowTreeCallback& callback) {
  std::vector<aura::Window*> windows =
      GetWindowTreeImpl(MakeClientWindowId(window_id));
  callback.Run(WindowsToWindowDatas(windows));
}

void WindowServiceClient::Embed(Id transport_window_id,
                                mojom::WindowTreeClientPtr client,
                                uint32_t embed_flags,
                                const EmbedCallback& callback) {
  callback.Run(EmbedImpl(MakeClientWindowId(transport_window_id),
                         std::move(client), embed_flags));
}

void WindowServiceClient::ScheduleEmbed(mojom::WindowTreeClientPtr client,
                                        const ScheduleEmbedCallback& callback) {
}

void WindowServiceClient::EmbedUsingToken(
    uint32_t window_id,
    const base::UnguessableToken& token,
    uint32_t embed_flags,
    const EmbedUsingTokenCallback& callback) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::SetFocus(uint32_t change_id, uint32_t window_id) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::SetCanFocus(uint32_t window_id, bool can_focus) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::SetCursor(uint32_t change_id,
                                    uint32_t window_id,
                                    ui::CursorData cursor) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::SetWindowTextInputState(
    uint32_t window_id,
    ::ui::mojom::TextInputStatePtr state) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::SetImeVisibility(
    uint32_t window_id,
    bool visible,
    ::ui::mojom::TextInputStatePtr state) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::SetEventTargetingPolicy(
    uint32_t window_id,
    ::ui::mojom::EventTargetingPolicy policy) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::OnWindowInputEventAck(
    uint32_t event_id,
    ::ui::mojom::EventResult result) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::DeactivateWindow(uint32_t window_id) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::StackAbove(uint32_t change_id,
                                     uint32_t above_id,
                                     uint32_t below_id) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::StackAtTop(uint32_t change_id, uint32_t window_id) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::PerformWmAction(uint32_t window_id,
                                          const std::string& action) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::GetWindowManagerClient(
    ::ui::mojom::WindowManagerClientAssociatedRequest internal) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::GetCursorLocationMemory(
    const GetCursorLocationMemoryCallback& callback) {
  HACK_.push_back(std::move(callback));
}

void WindowServiceClient::PerformWindowMove(uint32_t change_id,
                                            uint32_t window_id,
                                            ::ui::mojom::MoveLoopSource source,
                                            const gfx::Point& cursor) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::CancelWindowMove(uint32_t window_id) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::PerformDragDrop(
    uint32_t change_id,
    uint32_t source_window_id,
    const gfx::Point& screen_location,
    const std::unordered_map<std::string, std::vector<uint8_t>>& drag_data,
    const SkBitmap& drag_image,
    const gfx::Vector2d& drag_image_offset,
    uint32_t drag_operation,
    ::ui::mojom::PointerKind source) {
  NOTIMPLEMENTED();
}

void WindowServiceClient::CancelDragDrop(uint32_t window_id) {
  NOTIMPLEMENTED();
}

}  // namespace ws2
}  // namespace ui
