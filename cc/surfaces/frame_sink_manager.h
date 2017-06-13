// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_SURFACES_FRAME_SINK_MANAGER_H_
#define CC_SURFACES_FRAME_SINK_MANAGER_H_

#include <stdint.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "cc/surfaces/frame_sink_id.h"
#include "cc/surfaces/primary_begin_frame_source.h"
#include "cc/surfaces/surface_manager.h"
#include "cc/surfaces/surfaces_export.h"

namespace cc {
class BeginFrameSource;
class Display;
class DisplayClient;
class FrameSinkManagerClient;

namespace test {
class CompositorFrameSinkSupportTest;
class SurfaceSynchronizationTest;
}

class CC_SURFACES_EXPORT FrameSinkManager {
 public:
  FrameSinkManager(SurfaceManager::LifetimeType lifetime_type =
                       SurfaceManager::LifetimeType::SEQUENCES);
  ~FrameSinkManager();

  void RegisterFrameSinkId(const FrameSinkId& frame_sink_id);

  // Invalidate a frame_sink_id that might still have associated sequences,
  // possibly because a renderer process has crashed.
  void InvalidateFrameSinkId(const FrameSinkId& frame_sink_id);

  // CompositorFrameSinkSupport, hierarchy, and BeginFrameSource can be
  // registered and unregistered in any order with respect to each other.
  //
  // This happens in practice, e.g. the relationship to between ui::Compositor /
  // DelegatedFrameHost is known before ui::Compositor has a surface/client).
  // However, DelegatedFrameHost can register itself as a client before its
  // relationship with the ui::Compositor is known.

  // Associates a FrameSinkManagerClient with the frame_sink_id it uses.
  // FrameSinkManagerClient and framesink allocators have a 1:1 mapping.
  // Caller guarantees the client is alive between register/unregister.
  void RegisterFrameSinkManagerClient(const FrameSinkId& frame_sink_id,
                                      FrameSinkManagerClient* client);
  void UnregisterFrameSinkManagerClient(const FrameSinkId& frame_sink_id);

  // Associates a |source| with a particular framesink.  That framesink and
  // any children of that framesink with valid clients can potentially use
  // that |source|.
  void RegisterBeginFrameSource(BeginFrameSource* source,
                                const FrameSinkId& frame_sink_id);
  void UnregisterBeginFrameSource(BeginFrameSource* source);

  // Returns a stable BeginFrameSource that forwards BeginFrames from the first
  // available BeginFrameSource.
  BeginFrameSource* GetPrimaryBeginFrameSource();

  // Register a relationship between two framesinks.  This relationship means
  // that surfaces from the child framesik will be displayed in the parent.
  // Children are allowed to use any begin frame source that their parent can
  // use.
  void RegisterFrameSinkHierarchy(const FrameSinkId& parent_frame_sink_id,
                                  const FrameSinkId& child_frame_sink_id);
  void UnregisterFrameSinkHierarchy(const FrameSinkId& parent_frame_sink_id,
                                    const FrameSinkId& child_frame_sink_id);

  void AddSurfaceObserver(SurfaceObserver* obs);
  void RemoveSurfaceObserver(SurfaceObserver* obs);
  SurfaceDependencyTracker* GetDependencyTracker();
  void DropTemporaryReference(const SurfaceId& surface_id);
  bool IsUsingSurfaceReferences() const;
  void InitializeDisplay(Display* display, DisplayClient* display_client);
  void AddSurfaceReferences(const std::vector<SurfaceReference>& references);
  void RemoveSurfaceReferences(const std::vector<SurfaceReference>& references);
  const SurfaceId& GetRootSurfaceId() const;
  void AssignTemporaryReference(const SurfaceId& surface_id,
                                const FrameSinkId& owner);
  void SurfaceCreated(const SurfaceInfo& surface_info);
  void SurfaceActivated(Surface* surface);
  std::unique_ptr<Surface> CreateSurface(
      base::WeakPtr<CompositorFrameSinkSupport> compositor_frame_sink_support,
      const SurfaceInfo& surface_info);
  void DestroySurface(std::unique_ptr<Surface> surface);
  scoped_refptr<SurfaceReferenceFactory> GetReferenceFactory();
  void RequireSequence(const SurfaceId& surface_id,
                       const SurfaceSequence& sequence);
  void SatisfySequence(const SurfaceSequence& sequence);
  bool SurfaceModified(const SurfaceId& surface_id, const BeginFrameAck& ack);
  const base::flat_set<SurfaceId>& GetSurfacesReferencedByParent(
      const SurfaceId& surface_id) const;
  void SurfaceDamageExpected(const SurfaceId& surface_id,
                             const BeginFrameArgs& args);
  void SetDependencyTracker(SurfaceDependencyTracker* dependency_tracker);
  Surface* GetSurfaceForId(const SurfaceId& surface_id);

  SurfaceManager* surface_manager() { return &surface_manager_; }

 private:
  friend class test::CompositorFrameSinkSupportTest;
  friend class test::SurfaceSynchronizationTest;

  void RecursivelyAttachBeginFrameSource(const FrameSinkId& frame_sink_id,
                                         BeginFrameSource* source);
  void RecursivelyDetachBeginFrameSource(const FrameSinkId& frame_sink_id,
                                         BeginFrameSource* source);

  // Returns true if |child framesink| is or has |search_frame_sink_id| as a
  // child.
  bool ChildContains(const FrameSinkId& child_frame_sink_id,
                     const FrameSinkId& search_frame_sink_id) const;

  // Begin frame source routing. Both BeginFrameSource and
  // CompositorFrameSinkSupport pointers guaranteed alive by callers until
  // unregistered.
  struct FrameSinkSourceMapping {
    FrameSinkSourceMapping();
    FrameSinkSourceMapping(const FrameSinkSourceMapping& other);
    ~FrameSinkSourceMapping();
    bool has_children() const { return !children.empty(); }
    // The currently assigned begin frame source for this client.
    BeginFrameSource* source = nullptr;
    // This represents a dag of parent -> children mapping.
    std::vector<FrameSinkId> children;
  };

  std::unordered_map<FrameSinkId, FrameSinkManagerClient*, FrameSinkIdHash>
      clients_;
  std::unordered_map<FrameSinkId, FrameSinkSourceMapping, FrameSinkIdHash>
      frame_sink_source_map_;
  SurfaceManager surface_manager_;

  // Set of BeginFrameSource along with associated FrameSinkIds. Any child
  // that is implicitly using this framesink must be reachable by the
  // parent in the dag.
  std::unordered_map<BeginFrameSource*, FrameSinkId> registered_sources_;

  PrimaryBeginFrameSource primary_source_;

  DISALLOW_COPY_AND_ASSIGN(FrameSinkManager);
};

}  // namespace cc

#endif  // CC_SURFACES_FRAME_SINK_MANAGER_H_
