// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_WINDOW_SERVICE_CLIENT_H_
#define SERVICES_UI_WS2_WINDOW_SERVICE_CLIENT_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "base/component_export.h"
#include "base/macros.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "services/ui/ws2/ids.h"
#include "ui/aura/window_observer.h"

namespace aura {
class Window;
}

namespace ui {
namespace ws2 {

class WindowService;
class WindowServiceClientBinding;

// WindowServiceClient represents a client connected to the Window Service.
// This is created in two distinct ways:
// . when the client is embedded in a specific aura::Window, by way of
//   WindowTree::Embed().
// . by way of a WindowTreeFactory. In this mode the client has no initial
//   roots, instead the client requests a top-level window, which gives the
//   client a view into a specific portion of the tree.
//
// Each root is handled by a ClientRoot.
class COMPONENT_EXPORT(WINDOW_SERVICE) WindowServiceClient
    : public mojom::WindowTree,
      aura::WindowObserver {
 public:
  // This is going to need type, so that it can handle the case of top level.
  WindowServiceClient(WindowService* window_service,
                      ClientSpecificId client_id,
                      mojom::WindowTreeClient* client,
                      bool intercepts_events);
  ~WindowServiceClient() override;

  void InitForEmbedding(aura::Window* root,
                        mojom::WindowTreePtr window_tree_ptr);
  void InitFromFactory();

 private:
  class WindowEmbedding;

  using WindowEmbeddings = std::vector<std::unique_ptr<WindowEmbedding>>;

  enum class ConnectionType {
    // This tree is the result of an embedding. That is, another client calling
    // Embed().
    kEmbedding,

    // This tree was not the result of an embedding (connected directly to
    // this service).
    kOther,
  };

  enum class DeleteWindowEmbeddingReason {
    // The window is being destroyed.
    kDeleted,

    // Another client is being embedded in the window.
    kEmbed,

    // The embedded client explicitly asked to be unembedded.
    kUnembed,

    // Called when the WindowEmbedding is deleted from the destructor.
    kDestructor,
  };

  // Creates a new WindowEmbedding. The returned WindowEmbedding is owned by
  // this.
  WindowEmbedding* CreateWindowEmbedding(aura::Window* window,
                                         mojom::WindowTreePtr window_tree);
  void DeleteWindowEmbedding(WindowEmbedding* window_embedding,
                             DeleteWindowEmbeddingReason reason);
  void DeleteWindowEmbeddingWithRoot(aura::Window* window);

  aura::Window* GetWindowByClientId(const ClientWindowId& id);

  // Called for windows created by the client (including top-levels).
  aura::Window* AddClientCreatedWindow(const ClientWindowId& id,
                                       std::unique_ptr<aura::Window> window_ptr,
                                       bool is_top_level);

  bool DidCreateWindow(aura::Window* window);
  bool IsRoot(aura::Window* window);
  WindowEmbeddings::iterator FindWindowEmbeddingWithRoot(aura::Window* window);
  bool IsWindowKnown(aura::Window* window) const;
  bool IsWindowRootOfAnotherClient(aura::Window* window) const;
  void RegisterWindow(aura::Window* window, const ClientWindowId& id);
  void UnregisterWindow(aura::Window* window);

  // Unregisters window and all it's descendants. This stops at windows created
  // by this client. Any windows created by this client are added to
  // |created_windows|.
  void UnregisterRecursive(aura::Window* window,
                           std::vector<aura::Window*>* created_windows);

  WindowId GenerateNewWindowId();

  bool IsValidIdForNewWindow(const ClientWindowId& id) const;

  // Before the ClientWindowId gets sent back to the client, making sure we
  // reset the high 16 bits back to 0 if it's being sent back to the client
  // that created the window.
  Id ClientWindowIdToTransportId(const ClientWindowId& client_window_id) const;

  Id TransportIdForWindow(aura::Window* window) const;

  // Returns the ClientWindowId from a transport id. Uses |client_id_| as the
  // ClientWindowId::client_id part if invalid. This function does a straight
  // mapping, there may not be a window with the returned id.
  ClientWindowId MakeClientWindowId(Id transport_window_id) const;

  std::vector<mojom::WindowDataPtr> WindowsToWindowDatas(
      const std::vector<aura::Window*>& windows);
  mojom::WindowDataPtr WindowToWindowData(aura::Window* window);

  // Returns true if the request to change a property of |window| should be
  // routed to the delegate.
  bool ShouldRouteToDelegate(aura::Window* window);

  void PrepareForEmbedAndDetachExistingEmbedding(aura::Window* window);

  // Methods with the name Impl() mirror those of the mojom. The return value
  // indicates whether they succeeded or not. Generally failure means the
  // operation was not allowed.
  bool NewWindowImpl(
      const ClientWindowId& client_window_id,
      const std::map<std::string, std::vector<uint8_t>>& properties);
  bool DeleteWindowImpl(const ClientWindowId& window_id);
  bool AddWindowImpl(const ClientWindowId& parent_id,
                     const ClientWindowId& child_id);
  bool RemoveWindowFromParentImpl(const ClientWindowId& client_window_id);
  bool SetWindowVisibilityImpl(const ClientWindowId& window_id, bool visible);
  bool EmbedImpl(const ClientWindowId& window_id,
                 mojom::WindowTreeClientPtr window_tree_client,
                 uint32_t flags);
  bool SetWindowOpacityImpl(const ClientWindowId& window_id, float opacity);
  bool SetWindowBoundsImpl(
      const ClientWindowId& window_id,
      const gfx::Rect& bounds,
      const base::Optional<viz::LocalSurfaceId>& local_surface_id);
  std::vector<aura::Window*> GetWindowTreeImpl(const ClientWindowId& window_id);
  void GetWindowTreeRecursive(aura::Window* window,
                              std::vector<aura::Window*>* windows);

  // aura::WindowObserver:
  void OnWindowDestroyed(aura::Window* window) override;

  // mojom::WindowTree::
  void NewWindow(uint32_t change_id,
                 Id transport_window_id,
                 const base::Optional<
                     std::unordered_map<std::string, std::vector<uint8_t>>>&
                     transport_properties) override;
  void NewTopLevelWindow(
      uint32_t change_id,
      uint32_t window_id,
      const std::unordered_map<std::string, std::vector<uint8_t>>& properties)
      override;
  void DeleteWindow(uint32_t change_id, Id transport_window_id) override;
  void SetCapture(uint32_t change_id, uint32_t window_id) override;
  void ReleaseCapture(uint32_t change_id, uint32_t window_id) override;
  void StartPointerWatcher(bool want_moves) override;
  void StopPointerWatcher() override;
  void SetWindowBounds(
      uint32_t change_id,
      Id window_id,
      const gfx::Rect& bounds,
      const base::Optional<viz::LocalSurfaceId>& local_surface_id) override;
  void SetWindowTransform(uint32_t change_id,
                          uint32_t window_id,
                          const gfx::Transform& transform) override;
  void SetClientArea(uint32_t window_id,
                     const gfx::Insets& insets,
                     const base::Optional<std::vector<gfx::Rect>>&
                         additional_client_areas) override;
  void SetHitTestMask(uint32_t window_id,
                      const base::Optional<gfx::Rect>& mask) override;
  void SetCanAcceptDrops(uint32_t window_id, bool accepts_drops) override;
  void SetWindowVisibility(uint32_t change_id,
                           Id transport_window_id,
                           bool visible) override;
  void SetWindowProperty(
      uint32_t change_id,
      uint32_t window_id,
      const std::string& name,
      const base::Optional<std::vector<uint8_t>>& value) override;
  void SetWindowOpacity(uint32_t change_id,
                        Id transport_window_id,
                        float opacity) override;
  void AttachCompositorFrameSink(
      Id transport_window_id,
      ::viz::mojom::CompositorFrameSinkRequest compositor_frame_sink,
      ::viz::mojom::CompositorFrameSinkClientPtr client) override;
  void AddWindow(uint32_t change_id, Id parent_id, Id child_id) override;
  void RemoveWindowFromParent(uint32_t change_id, Id window_id) override;
  void AddTransientWindow(uint32_t change_id,
                          uint32_t window_id,
                          uint32_t transient_window_id) override;
  void RemoveTransientWindowFromParent(uint32_t change_id,
                                       uint32_t transient_window_id) override;
  void SetModalType(uint32_t change_id,
                    uint32_t window_id,
                    ui::ModalType type) override;
  void SetChildModalParent(uint32_t change_id,
                           uint32_t window_id,
                           uint32_t parent_window_id) override;
  void ReorderWindow(uint32_t change_id,
                     uint32_t window_id,
                     uint32_t relative_window_id,
                     ::ui::mojom::OrderDirection direction) override;
  void GetWindowTree(uint32_t window_id,
                     const GetWindowTreeCallback& callback) override;
  void Embed(Id transport_window_id,
             mojom::WindowTreeClientPtr client,
             uint32_t embed_flags,
             const EmbedCallback& callback) override;
  void ScheduleEmbed(mojom::WindowTreeClientPtr client,
                     const ScheduleEmbedCallback& callback) override;
  void EmbedUsingToken(uint32_t window_id,
                       const base::UnguessableToken& token,
                       uint32_t embed_flags,
                       const EmbedUsingTokenCallback& callback) override;
  void SetFocus(uint32_t change_id, uint32_t window_id) override;
  void SetCanFocus(uint32_t window_id, bool can_focus) override;
  void SetCursor(uint32_t change_id,
                 uint32_t window_id,
                 ui::CursorData cursor) override;
  void SetWindowTextInputState(uint32_t window_id,
                               ::ui::mojom::TextInputStatePtr state) override;
  void SetImeVisibility(uint32_t window_id,
                        bool visible,
                        ::ui::mojom::TextInputStatePtr state) override;
  void SetEventTargetingPolicy(
      uint32_t window_id,
      ::ui::mojom::EventTargetingPolicy policy) override;
  void OnWindowInputEventAck(uint32_t event_id,
                             ::ui::mojom::EventResult result) override;
  void DeactivateWindow(uint32_t window_id) override;
  void StackAbove(uint32_t change_id,
                  uint32_t above_id,
                  uint32_t below_id) override;
  void StackAtTop(uint32_t change_id, uint32_t window_id) override;
  void PerformWmAction(uint32_t window_id, const std::string& action) override;
  void GetWindowManagerClient(
      ::ui::mojom::WindowManagerClientAssociatedRequest internal) override;
  void GetCursorLocationMemory(
      const GetCursorLocationMemoryCallback& callback) override;
  void PerformWindowMove(uint32_t change_id,
                         uint32_t window_id,
                         ::ui::mojom::MoveLoopSource source,
                         const gfx::Point& cursor) override;
  void CancelWindowMove(uint32_t window_id) override;
  void PerformDragDrop(
      uint32_t change_id,
      uint32_t source_window_id,
      const gfx::Point& screen_location,
      const std::unordered_map<std::string, std::vector<uint8_t>>& drag_data,
      const SkBitmap& drag_image,
      const gfx::Vector2d& drag_image_offset,
      uint32_t drag_operation,
      ::ui::mojom::PointerKind source) override;
  void CancelDragDrop(uint32_t window_id) override;

  WindowService* window_service_;

  const ClientSpecificId client_id_;

  ConnectionType connection_type_ = ConnectionType::kEmbedding;

  mojom::WindowTreeClient* window_tree_client_;

  ClientSpecificId next_window_id_ = 1;

  // If true the client sees all the decendants of windows with embeddings
  // in them that were created by this client.
  const bool intercepts_events_;

  // Controls whether the client can change the visibility of the roots.
  bool can_change_root_window_visibility_ = true;

  WindowEmbeddings window_embeddings_;

  std::set<aura::Window*> client_created_windows_;

  std::map<aura::Window*, ClientWindowId> window_to_client_window_id_;

  std::unordered_map<ClientWindowId, aura::Window*, ClientWindowIdHash>
      client_window_id_to_window_map_;

  // If true a window is being deleted at the request of the client.
  aura::Window* window_deleting_ = nullptr;

  // WindowServiceClientBindings created by way of Embed().
  std::vector<std::unique_ptr<WindowServiceClientBinding>>
      embedded_client_bindings_;

  // XXX
  std::vector<GetCursorLocationMemoryCallback> HACK_;

  DISALLOW_COPY_AND_ASSIGN(WindowServiceClient);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_WINDOW_SERVICE_CLIENT_H_
