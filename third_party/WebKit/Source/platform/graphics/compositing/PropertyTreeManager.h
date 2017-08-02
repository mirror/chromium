// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PropertyTreeManager_h
#define PropertyTreeManager_h

#include "cc/layers/content_layer_client.h"
#include "platform/wtf/HashMap.h"
#include "platform/wtf/HashSet.h"
#include "platform/wtf/Noncopyable.h"
#include "platform/wtf/Vector.h"
#include "third_party/skia/include/core/SkBlendMode.h"

namespace cc {
class ClipTree;
class EffectTree;
class Layer;
class PictureLayer;
class PropertyTrees;
class ScrollTree;
class TransformTree;
}

namespace blink {

class ClipPaintPropertyNode;
class EffectPaintPropertyNode;
class FloatRoundedRect;
class ScrollPaintPropertyNode;
class TransformPaintPropertyNode;

class RRectContentLayerClient : public cc::ContentLayerClient {
 public:
  RRectContentLayerClient(const FloatRoundedRect& rrect);
  ~RRectContentLayerClient();

  cc::Layer* GetLayer() const;

  gfx::Rect PaintableRegion() override;
  scoped_refptr<cc::DisplayItemList> PaintContentsToDisplayList(
      PaintingControlSetting painting_status) override;
  bool FillsBoundsCompletely() const override { return false; }
  size_t GetApproximateUnsharedMemoryUsage() const { return 0; }

 private:
  scoped_refptr<cc::PictureLayer> layer_;
  SkRRect rrect_;
};

// Mutates a cc property tree to reflect Blink paint property tree
// state. Intended for use by PaintArtifactCompositor.
class PropertyTreeManager {
  WTF_MAKE_NONCOPYABLE(PropertyTreeManager);

 public:
  PropertyTreeManager(cc::PropertyTrees&,
                      Vector<std::unique_ptr<RRectContentLayerClient>>&,
                      cc::Layer* root_layer,
                      int sequence_number);
  ~PropertyTreeManager();

  void SetupRootTransformNode();
  void SetupRootClipNode();
  void SetupRootEffectNode();
  void SetupRootScrollNode();

  // A brief discourse on cc property tree nodes, identifiers, and current and
  // future design evolution envisioned:
  //
  // cc property trees identify nodes by their |id|, which implementation-wise
  // is actually its index in the property tree's vector of its node type. More
  // recent cc code now refers to these as 'node indices', or 'property tree
  // indices'. |parent_id| is the same sort of 'node index' of that node's
  // parent.
  //
  // Note there are two other primary types of 'ids' referenced in cc property
  // tree related logic: (1) ElementId, also known Blink-side as
  // CompositorElementId, used by the animation system to allow tying an element
  // to its respective layer, and (2) layer ids. There are other ancillary ids
  // not relevant to any of the above, such as
  // cc::TransformNode::sorting_context_id
  // (a.k.a. blink::TransformPaintPropertyNode::renderingContextId()).
  //
  // There is a vision to move toward a world where cc property nodes have no
  // association with layers and instead have a |stable_id|. The id could come
  // from an ElementId in turn derived from the layout object responsible for
  // creating the property node.
  //
  // We would also like to explore moving to use a single shared property tree
  // representation across both cc and Blink. See
  // platform/graphics/paint/README.md for more.
  //
  // With the above as background, we can now state more clearly a description
  // of the below set of compositor node methods: they take Blink paint property
  // tree nodes as input, create a corresponding compositor property tree node
  // if none yet exists, and return the compositor node's 'node id', a.k.a.,
  // 'node index'.

  int EnsureCompositorTransformNode(const TransformPaintPropertyNode*);
  int EnsureCompositorClipNode(const ClipPaintPropertyNode*);
  // Update the layer->scroll and scroll->layer mapping. The latter is temporary
  // until |owning_layer_id| is removed from the scroll node.
  void UpdateLayerScrollMapping(cc::Layer*, const TransformPaintPropertyNode*);

  int SwitchToEffectNodeWithRoundedClip(
      const EffectPaintPropertyNode& next_effect,
      const ClipPaintPropertyNode& next_clip);
  void Finalize();

 private:
  bool BuildEffectNodesRecursively(const EffectPaintPropertyNode* next_effect);
  SkBlendMode BuildSyntheticEffectNodeForRoundedClipIfNeeded(
      const ClipPaintPropertyNode* target_clip,
      SkBlendMode delegated_blend,
      bool effect_is_newly_built);
  void EmitRoundedClipMaskLayer();
  bool IsCurrentEffectSynthetic() const;

  cc::TransformTree& GetTransformTree();
  cc::ClipTree& GetClipTree();
  cc::EffectTree& GetEffectTree();
  cc::ScrollTree& GetScrollTree();

  int EnsureCompositorScrollNode(const ScrollPaintPropertyNode*);

  // Scroll translation has special treatment in the transform and scroll trees.
  void UpdateScrollAndScrollTranslationNodes(const TransformPaintPropertyNode*);

  void ExitEffect();

  // Property trees which should be updated by the manager.
  cc::PropertyTrees& property_trees_;

  Vector<std::unique_ptr<RRectContentLayerClient>>&
      rrect_content_layer_clients_;

  // Layer to which transform "owner" layers should be added. These will not
  // have any actual children, but at present must exist in the tree.
  cc::Layer* root_layer_;

  // Maps from Blink-side property tree nodes to cc property node indices.
  HashMap<const TransformPaintPropertyNode*, int> transform_node_map_;
  HashMap<const ClipPaintPropertyNode*, int> clip_node_map_;
  HashMap<const ScrollPaintPropertyNode*, int> scroll_node_map_;

  struct EffectStackEntry {
    enum class EffectType : char { kRoundedClip, kEffect } type;
    const EffectPaintPropertyNode* effect;
    const ClipPaintPropertyNode* clip;
    int effect_id;
  };
  Vector<EffectStackEntry> effect_stack_;
  const EffectPaintPropertyNode* current_effect_;
  const ClipPaintPropertyNode* current_clip_;
  int current_effect_id_;

  int sequence_number_;

#if DCHECK_IS_ON()
  HashSet<const EffectPaintPropertyNode*> effect_nodes_converted_;
#endif
};

}  // namespace blink

#endif  // PropertyTreeManager_h
