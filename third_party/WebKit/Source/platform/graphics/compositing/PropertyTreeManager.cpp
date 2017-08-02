// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/compositing/PropertyTreeManager.h"

#include "cc/layers/picture_layer.h"
#include "cc/trees/clip_node.h"
#include "cc/trees/effect_node.h"
#include "cc/trees/layer_tree_host.h"
#include "cc/trees/property_tree.h"
#include "cc/trees/scroll_node.h"
#include "cc/trees/transform_node.h"
#include "platform/graphics/paint/ClipPaintPropertyNode.h"
#include "platform/graphics/paint/EffectPaintPropertyNode.h"
#include "platform/graphics/paint/GeometryMapper.h"
#include "platform/graphics/paint/ScrollPaintPropertyNode.h"
#include "platform/graphics/paint/TransformPaintPropertyNode.h"
#include "public/platform/WebLayerScrollClient.h"
#include "third_party/skia/include/effects/SkColorFilterImageFilter.h"
#include "third_party/skia/include/effects/SkLumaColorFilter.h"

namespace blink {

namespace {

static constexpr int kInvalidNodeId = -1;
// cc's property trees use 0 for the root node (always non-null).
static constexpr int kRealRootNodeId = 0;
// cc allocates special nodes for root effects such as the device scale.
static constexpr int kSecondaryRootNodeId = 1;

}  // namespace

PropertyTreeManager::PropertyTreeManager(
    cc::PropertyTrees& property_trees,
    Vector<std::unique_ptr<RRectContentLayerClient>>&
        rrect_content_layer_clients,
    cc::Layer* root_layer,
    int sequence_number)
    : property_trees_(property_trees),
      rrect_content_layer_clients_(rrect_content_layer_clients),
      root_layer_(root_layer),
      sequence_number_(sequence_number) {
  SetupRootTransformNode();
  SetupRootClipNode();
  SetupRootEffectNode();
  SetupRootScrollNode();
}

PropertyTreeManager::~PropertyTreeManager() = default;

void PropertyTreeManager::Finalize() {
  while (effect_stack_.size())
    ExitEffect();
}

cc::TransformTree& PropertyTreeManager::GetTransformTree() {
  return property_trees_.transform_tree;
}

cc::ClipTree& PropertyTreeManager::GetClipTree() {
  return property_trees_.clip_tree;
}

cc::EffectTree& PropertyTreeManager::GetEffectTree() {
  return property_trees_.effect_tree;
}

cc::ScrollTree& PropertyTreeManager::GetScrollTree() {
  return property_trees_.scroll_tree;
}

void PropertyTreeManager::SetupRootTransformNode() {
  // cc is hardcoded to use transform node index 1 for device scale and
  // transform.
  cc::TransformTree& transform_tree = property_trees_.transform_tree;
  transform_tree.clear();
  property_trees_.element_id_to_transform_node_index.clear();
  cc::TransformNode& transform_node = *transform_tree.Node(
      transform_tree.Insert(cc::TransformNode(), kRealRootNodeId));
  DCHECK_EQ(transform_node.id, kSecondaryRootNodeId);
  transform_node.source_node_id = transform_node.parent_id;

  // TODO(jaydasika): We shouldn't set ToScreen and FromScreen of root
  // transform node here. They should be set while updating transform tree in
  // cc.
  float device_scale_factor =
      root_layer_->layer_tree_host()->device_scale_factor();
  gfx::Transform to_screen;
  to_screen.Scale(device_scale_factor, device_scale_factor);
  transform_tree.SetToScreen(kRealRootNodeId, to_screen);
  gfx::Transform from_screen;
  bool invertible = to_screen.GetInverse(&from_screen);
  DCHECK(invertible);
  transform_tree.SetFromScreen(kRealRootNodeId, from_screen);
  transform_tree.set_needs_update(true);

  transform_node_map_.Set(TransformPaintPropertyNode::Root(),
                          transform_node.id);
  root_layer_->SetTransformTreeIndex(transform_node.id);
}

void PropertyTreeManager::SetupRootClipNode() {
  // cc is hardcoded to use clip node index 1 for viewport clip.
  cc::ClipTree& clip_tree = property_trees_.clip_tree;
  clip_tree.clear();
  cc::ClipNode& clip_node =
      *clip_tree.Node(clip_tree.Insert(cc::ClipNode(), kRealRootNodeId));
  DCHECK_EQ(clip_node.id, kSecondaryRootNodeId);

  clip_node.clip_type = cc::ClipNode::ClipType::APPLIES_LOCAL_CLIP;
  clip_node.clip = gfx::RectF(
      gfx::SizeF(root_layer_->layer_tree_host()->device_viewport_size()));
  clip_node.transform_id = kRealRootNodeId;

  clip_node_map_.Set(ClipPaintPropertyNode::Root(), clip_node.id);
  root_layer_->SetClipTreeIndex(clip_node.id);
}

void PropertyTreeManager::SetupRootEffectNode() {
  // cc is hardcoded to use effect node index 1 for root render surface.
  cc::EffectTree& effect_tree = property_trees_.effect_tree;
  effect_tree.clear();
  property_trees_.element_id_to_effect_node_index.clear();
  cc::EffectNode& effect_node =
      *effect_tree.Node(effect_tree.Insert(cc::EffectNode(), kInvalidNodeId));
  DCHECK_EQ(effect_node.id, kSecondaryRootNodeId);
  effect_node.stable_id =
      CompositorElementIdFromRootEffectId(kSecondaryRootNodeId).id_;
  effect_node.transform_id = kRealRootNodeId;
  effect_node.clip_id = kSecondaryRootNodeId;
  effect_node.has_render_surface = true;
  root_layer_->SetEffectTreeIndex(effect_node.id);

  current_effect_ = EffectPaintPropertyNode::Root();
  current_clip_ = current_effect_->OutputClip();
  current_effect_id_ = effect_node.id;
}

void PropertyTreeManager::SetupRootScrollNode() {
  cc::ScrollTree& scroll_tree = property_trees_.scroll_tree;
  scroll_tree.clear();
  property_trees_.element_id_to_scroll_node_index.clear();
  cc::ScrollNode& scroll_node =
      *scroll_tree.Node(scroll_tree.Insert(cc::ScrollNode(), kRealRootNodeId));
  DCHECK_EQ(scroll_node.id, kSecondaryRootNodeId);
  scroll_node.transform_id = kSecondaryRootNodeId;

  scroll_node_map_.Set(ScrollPaintPropertyNode::Root(), scroll_node.id);
  root_layer_->SetScrollTreeIndex(scroll_node.id);
}

int PropertyTreeManager::EnsureCompositorTransformNode(
    const TransformPaintPropertyNode* transform_node) {
  DCHECK(transform_node);
  // TODO(crbug.com/645615): Remove the failsafe here.
  if (!transform_node)
    return kSecondaryRootNodeId;

  auto it = transform_node_map_.find(transform_node);
  if (it != transform_node_map_.end())
    return it->value;

  scoped_refptr<cc::Layer> dummy_layer = cc::Layer::Create();
  int parent_id = EnsureCompositorTransformNode(transform_node->Parent());
  int id = GetTransformTree().Insert(cc::TransformNode(), parent_id);

  cc::TransformNode& compositor_node = *GetTransformTree().Node(id);
  compositor_node.source_node_id = parent_id;

  FloatPoint3D origin = transform_node->Origin();
  compositor_node.pre_local.matrix().setTranslate(-origin.X(), -origin.Y(),
                                                  -origin.Z());
  compositor_node.local.matrix() =
      TransformationMatrix::ToSkMatrix44(transform_node->Matrix());
  compositor_node.post_local.matrix().setTranslate(origin.X(), origin.Y(),
                                                   origin.Z());
  compositor_node.needs_local_transform_update = true;
  compositor_node.flattens_inherited_transform =
      transform_node->FlattensInheritedTransform();
  compositor_node.sorting_context_id = transform_node->RenderingContextId();

  root_layer_->AddChild(dummy_layer);
  dummy_layer->SetTransformTreeIndex(id);
  dummy_layer->SetClipTreeIndex(kSecondaryRootNodeId);
  dummy_layer->SetEffectTreeIndex(kSecondaryRootNodeId);
  dummy_layer->SetScrollTreeIndex(kRealRootNodeId);
  dummy_layer->set_property_tree_sequence_number(sequence_number_);
  CompositorElementId compositor_element_id =
      transform_node->GetCompositorElementId();
  if (compositor_element_id) {
    property_trees_.element_id_to_transform_node_index[compositor_element_id] =
        id;
  }

  auto result = transform_node_map_.Set(transform_node, id);
  DCHECK(result.is_new_entry);
  GetTransformTree().set_needs_update(true);

  if (transform_node->ScrollNode())
    UpdateScrollAndScrollTranslationNodes(transform_node);

  return id;
}

int PropertyTreeManager::EnsureCompositorClipNode(
    const ClipPaintPropertyNode* clip_node) {
  DCHECK(clip_node);
  // TODO(crbug.com/645615): Remove the failsafe here.
  if (!clip_node)
    return kSecondaryRootNodeId;

  auto it = clip_node_map_.find(clip_node);
  if (it != clip_node_map_.end())
    return it->value;

  scoped_refptr<cc::Layer> dummy_layer = cc::Layer::Create();
  int parent_id = EnsureCompositorClipNode(clip_node->Parent());
  int id = GetClipTree().Insert(cc::ClipNode(), parent_id);

  cc::ClipNode& compositor_node = *GetClipTree().Node(id);

  compositor_node.clip = clip_node->ClipRect().Rect();
  compositor_node.transform_id =
      EnsureCompositorTransformNode(clip_node->LocalTransformSpace());
  compositor_node.clip_type = cc::ClipNode::ClipType::APPLIES_LOCAL_CLIP;

  root_layer_->AddChild(dummy_layer);
  dummy_layer->SetTransformTreeIndex(compositor_node.transform_id);
  dummy_layer->SetClipTreeIndex(id);
  dummy_layer->SetEffectTreeIndex(kSecondaryRootNodeId);
  dummy_layer->SetScrollTreeIndex(kRealRootNodeId);
  dummy_layer->set_property_tree_sequence_number(sequence_number_);

  auto result = clip_node_map_.Set(clip_node, id);
  DCHECK(result.is_new_entry);
  GetClipTree().set_needs_update(true);
  return id;
}

int PropertyTreeManager::EnsureCompositorScrollNode(
    const ScrollPaintPropertyNode* scroll_node) {
  DCHECK(scroll_node);
  // TODO(crbug.com/645615): Remove the failsafe here.
  if (!scroll_node)
    return kSecondaryRootNodeId;

  auto it = scroll_node_map_.find(scroll_node);
  if (it != scroll_node_map_.end())
    return it->value;

  int parent_id = EnsureCompositorScrollNode(scroll_node->Parent());
  int id = GetScrollTree().Insert(cc::ScrollNode(), parent_id);

  cc::ScrollNode& compositor_node = *GetScrollTree().Node(id);
  compositor_node.scrollable = true;

  compositor_node.container_bounds = scroll_node->ContainerBounds();
  compositor_node.bounds = scroll_node->Bounds();
  compositor_node.user_scrollable_horizontal =
      scroll_node->UserScrollableHorizontal();
  compositor_node.user_scrollable_vertical =
      scroll_node->UserScrollableVertical();
  compositor_node.main_thread_scrolling_reasons =
      scroll_node->GetMainThreadScrollingReasons();

  auto result = scroll_node_map_.Set(scroll_node, id);
  DCHECK(result.is_new_entry);
  GetScrollTree().set_needs_update(true);

  return id;
}

void PropertyTreeManager::UpdateScrollAndScrollTranslationNodes(
    const TransformPaintPropertyNode* scroll_offset_node) {
  DCHECK(scroll_offset_node->ScrollNode());
  int scroll_node_id =
      EnsureCompositorScrollNode(scroll_offset_node->ScrollNode());
  auto& compositor_scroll_node = *GetScrollTree().Node(scroll_node_id);
  int transform_node_id = EnsureCompositorTransformNode(scroll_offset_node);
  auto& compositor_transform_node = *GetTransformTree().Node(transform_node_id);

  auto compositor_element_id = scroll_offset_node->GetCompositorElementId();
  if (compositor_element_id) {
    compositor_scroll_node.element_id = compositor_element_id;
    property_trees_.element_id_to_scroll_node_index[compositor_element_id] =
        scroll_node_id;
  }

  compositor_scroll_node.transform_id = transform_node_id;
  // TODO(pdr): Set the scroll node's non_fast_scrolling_region value.

  // Blink creates a 2d transform node just for scroll offset whereas cc's
  // transform node has a special scroll offset field. To handle this we adjust
  // cc's transform node to remove the 2d scroll translation and instead set the
  // scroll_offset field.
  auto scroll_offset_size = scroll_offset_node->Matrix().To2DTranslation();
  auto scroll_offset = gfx::ScrollOffset(-scroll_offset_size.Width(),
                                         -scroll_offset_size.Height());
  DCHECK(compositor_transform_node.local.IsIdentityOr2DTranslation());
  compositor_transform_node.scroll_offset = scroll_offset;
  compositor_transform_node.local.MakeIdentity();
  compositor_transform_node.scrolls = true;
  GetTransformTree().set_needs_update(true);
  // TODO(pdr): Because of a layer dependancy, the scroll tree scroll offset is
  // set in updateLayerScrollMapping but that should occur here.
}

void PropertyTreeManager::UpdateLayerScrollMapping(
    cc::Layer* layer,
    const TransformPaintPropertyNode* transform) {
  auto* enclosing_scroll_node = transform->FindEnclosingScrollNode();
  int scroll_node_id = EnsureCompositorScrollNode(enclosing_scroll_node);
  layer->SetScrollTreeIndex(scroll_node_id);
  auto& compositor_scroll_node = *GetScrollTree().Node(scroll_node_id);

  if (!transform->IsScrollTranslation())
    return;

  auto& compositor_transform_node =
      *GetTransformTree().Node(compositor_scroll_node.transform_id);
  // TODO(pdr): Set this in updateScrollAndScrollTranslationNodes once the
  // layer id is no longer needed.
  GetScrollTree().SetScrollOffset(transform->GetCompositorElementId(),
                                  compositor_transform_node.scroll_offset);

  // TODO(pdr): This approach of setting a callback on all Layers with a scroll
  // node is wrong because only the base scrollable layer needs this callback.
  // This should be fixed as part of correctly creating scrollable layers in
  // https://crbug.com/738613.
  if (auto* scroll_client = enclosing_scroll_node->ScrollClient()) {
    layer->set_did_scroll_callback(
        base::Bind(&blink::WebLayerScrollClient::DidScroll,
                   base::Unretained(scroll_client)));
  }
}

RRectContentLayerClient::RRectContentLayerClient(const FloatRoundedRect& rrect)
    : layer_(cc::PictureLayer::Create(this)),
      rrect_(static_cast<SkRRect>(rrect)) {
  IntRect layer_bounds = EnclosingIntRect(rrect.Rect());

  layer_->set_offset_to_transform_parent(
      gfx::Vector2dF(layer_bounds.X(), layer_bounds.Y()));
  layer_->SetBounds(gfx::Size(layer_bounds.Width(), layer_bounds.Height()));
  layer_->SetIsDrawable(true);

  rrect_.offset(-layer_bounds.X(), -layer_bounds.Y());
}

RRectContentLayerClient::~RRectContentLayerClient() = default;

cc::Layer* RRectContentLayerClient::GetLayer() const {
  return layer_.get();
}

gfx::Rect RRectContentLayerClient::PaintableRegion() {
  return gfx::Rect(layer_->bounds());
}

scoped_refptr<cc::DisplayItemList>
RRectContentLayerClient::PaintContentsToDisplayList(
    PaintingControlSetting painting_status) {
  auto cc_list = base::MakeRefCounted<cc::DisplayItemList>(
      cc::DisplayItemList::kTopLevelDisplayItemList);
  cc_list->StartPaint();
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  cc_list->push<cc::DrawRRectOp>(rrect_, flags);
  cc_list->EndPaintOfUnpaired(gfx::Rect(layer_->bounds()));
  cc_list->Finalize();
  return cc_list;
}

void PropertyTreeManager::EmitRoundedClipMaskLayer() {
  const TransformPaintPropertyNode* transform =
      current_clip_->LocalTransformSpace();
  int clip_id = EnsureCompositorClipNode(current_clip_);

  cc::EffectNode& effect_node = *GetEffectTree().Node(
      GetEffectTree().Insert(cc::EffectNode(), current_effect_id_));
  effect_node.stable_id = GenCompositorElementIdForRoundedClip().id_;
  effect_node.clip_id = clip_id;
  effect_node.has_render_surface = true;
  effect_node.blend_mode = SkBlendMode::kDstIn;

  cc::Layer* layer =
      rrect_content_layer_clients_
          .emplace_back(std::make_unique<RRectContentLayerClient>(
              current_clip_->ClipRect()))
          ->GetLayer();
  root_layer_->AddChild(layer);
  layer->set_property_tree_sequence_number(sequence_number_);
  layer->SetTransformTreeIndex(EnsureCompositorTransformNode(transform));
  layer->SetClipTreeIndex(clip_id);
  layer->SetEffectTreeIndex(effect_node.id);
  UpdateLayerScrollMapping(layer, transform);
}

void PropertyTreeManager::ExitEffect() {
  DCHECK(effect_stack_.size());
  // TODO: insert mask layer here if type is kRoundedClip.
  const EffectStackEntry& previous_state = effect_stack_.back();
  bool clear_synthetic_effects =
      previous_state.type == EffectStackEntry::EffectType::kEffect &&
      current_effect_->BlendMode() != SkBlendMode::kSrcOver;

  if (IsCurrentEffectSynthetic())
    EmitRoundedClipMaskLayer();

  current_effect_ = previous_state.effect;
  current_clip_ = previous_state.clip;
  current_effect_id_ = previous_state.effect_id;
  effect_stack_.pop_back();

  if (clear_synthetic_effects) {
    while (IsCurrentEffectSynthetic())
      ExitEffect();
  }
}

int PropertyTreeManager::SwitchToEffectNodeWithRoundedClip(
    const EffectPaintPropertyNode& next_effect,
    const ClipPaintPropertyNode& next_clip) {
  const auto& ancestor = LowestCommonAncestor(*current_effect_, next_effect);
  while (current_effect_ != &ancestor)
    ExitEffect();

  bool newly_built = BuildEffectNodesRecursively(&next_effect);
  BuildSyntheticEffectNodeForRoundedClipIfNeeded(
      &next_clip, SkBlendMode::kSrcOver, newly_built);

  return current_effect_id_;
}

bool IsAncestorOf(const ClipPaintPropertyNode* ancestor,
                  const ClipPaintPropertyNode* node) {
  for (; node != ancestor; node = node->Parent()) {
    if (!node)
      return false;
  }
  return true;
}

bool PropertyTreeManager::IsCurrentEffectSynthetic() const {
  return effect_stack_.size() && effect_stack_.back().type ==
                                     EffectStackEntry::EffectType::kRoundedClip;
}

SkBlendMode PropertyTreeManager::BuildSyntheticEffectNodeForRoundedClipIfNeeded(
    const ClipPaintPropertyNode* target_clip,
    SkBlendMode delegated_blend,
    bool effect_is_newly_built) {
  if (delegated_blend != SkBlendMode::kSrcOver) {
    // Exit all synthetic effect node for rounded clip if the next child has
    // exotic blending mode because it has to access the backdrop of enclosing
    // effect.
    while (IsCurrentEffectSynthetic())
      ExitEffect();

    // An effect node can't omit render surface if it has child with exotic
    // blending mode. See comments below for more detail.
    // TODO(crbug.com/504464): Remove premature optimization here.
    GetEffectTree().Node(current_effect_id_)->has_render_surface = true;
  } else {
    // TODO: This is O(d^2). :(
    while (!IsAncestorOf(current_clip_, target_clip)) {
      DCHECK(IsCurrentEffectSynthetic());
      ExitEffect();
    }

    // If the effect is an existing node, i.e. already has at least one paint
    // chunk or child effect, and by reaching here it implies we are going to
    // attach either another paint chunk or child effect to it. We can no longer
    // omit render surface for it even for opacity-only node.
    // See comments in PropertyTreeManager::buildEffectNodesRecursively().
    // TODO(crbug.com/504464): Remove premature optimization here.
    if (!effect_is_newly_built && !IsCurrentEffectSynthetic() &&
        current_effect_->Opacity() != 1.f)
      GetEffectTree().Node(current_effect_id_)->has_render_surface = true;
  }

  DCHECK(IsAncestorOf(current_clip_, target_clip));

  Vector<const ClipPaintPropertyNode*> pending_clips;
  for (; target_clip != current_clip_; target_clip = target_clip->Parent()) {
    DCHECK(target_clip);
    if (target_clip->ClipRect().IsRounded())
      pending_clips.push_back(target_clip);
  }

  for (size_t i = pending_clips.size(); i--;) {
    const ClipPaintPropertyNode* next_clip = pending_clips[i];

    cc::EffectNode& effect_node = *GetEffectTree().Node(
        GetEffectTree().Insert(cc::EffectNode(), current_effect_id_));

    effect_node.stable_id = GenCompositorElementIdForRoundedClip().id_;
    effect_node.clip_id = EnsureCompositorClipNode(next_clip);
    effect_node.has_render_surface = true;
    effect_node.blend_mode = delegated_blend;
    delegated_blend = SkBlendMode::kSrcOver;

    effect_stack_.emplace_back(
        EffectStackEntry{EffectStackEntry::EffectType::kRoundedClip,
                         current_effect_, current_clip_, current_effect_id_});
    current_clip_ = next_clip;
    current_effect_id_ = effect_node.id;
  }

  return delegated_blend;
}

bool PropertyTreeManager::BuildEffectNodesRecursively(
    const EffectPaintPropertyNode* next_effect) {
  if (next_effect == current_effect_)
    return false;
  DCHECK(next_effect);

  bool newly_built = BuildEffectNodesRecursively(next_effect->Parent());
  DCHECK_EQ(next_effect->Parent(), current_effect_);

#if DCHECK_IS_ON()
  DCHECK(!effect_nodes_converted_.Contains(next_effect))
      << "Malformed paint artifact. Paint chunks under the same effect should "
         "be contiguous.";
  effect_nodes_converted_.insert(next_effect);
#endif

  SkBlendMode used_blend_mode = BuildSyntheticEffectNodeForRoundedClipIfNeeded(
      next_effect->OutputClip(), next_effect->BlendMode(), newly_built);

  // We currently create dummy layers to host effect nodes and corresponding
  // render surfaces. This should be removed once cc implements better support
  // for freestanding property trees.
  scoped_refptr<cc::Layer> dummy_layer = next_effect->EnsureDummyLayer();
  root_layer_->AddChild(dummy_layer);

  int output_clip_id = EnsureCompositorClipNode(next_effect->OutputClip());

  cc::EffectNode& effect_node = *GetEffectTree().Node(
      GetEffectTree().Insert(cc::EffectNode(), current_effect_id_));
  effect_node.stable_id = next_effect->GetCompositorElementId().id_;
  effect_node.clip_id = output_clip_id;
  // Every effect is supposed to have render surface enabled for grouping,
  // but we can get away without one if the effect is opacity-only and has only
  // one compositing child with kSrcOver blend mode. This is both for
  // optimization and not introducing sub-pixel differences in layout tests.
  // See PropertyTreeManager::switchToEffectNode() and above where we
  // retrospectively enable render surface when more than one compositing child
  // or a child with exotic blend mode is detected.
  // TODO(crbug.com/504464): There is ongoing work in cc to delay render surface
  // decision until later phase of the pipeline. Remove premature optimization
  // here once the work is ready.
  if (!next_effect->Filter().IsEmpty() ||
      used_blend_mode != SkBlendMode::kSrcOver)
    effect_node.has_render_surface = true;
  effect_node.opacity = next_effect->Opacity();
  if (next_effect->GetColorFilter() != kColorFilterNone) {
    // Currently color filter is only used by SVG masks.
    // We are cutting corner here by support only specific configuration.
    DCHECK(next_effect->GetColorFilter() == kColorFilterLuminanceToAlpha);
    DCHECK(used_blend_mode == SkBlendMode::kDstIn);
    DCHECK(next_effect->Filter().IsEmpty());
    effect_node.filters.Append(cc::FilterOperation::CreateReferenceFilter(
        SkColorFilterImageFilter::Make(SkLumaColorFilter::Make(), nullptr)));
  } else {
    effect_node.filters = next_effect->Filter().AsCcFilterOperations();
  }
  effect_node.blend_mode = used_blend_mode;
  CompositorElementId compositor_element_id =
      next_effect->GetCompositorElementId();
  if (compositor_element_id) {
    DCHECK(property_trees_.element_id_to_effect_node_index.find(
               compositor_element_id) ==
           property_trees_.element_id_to_effect_node_index.end());
    property_trees_.element_id_to_effect_node_index[compositor_element_id] =
        effect_node.id;
  }
  effect_stack_.emplace_back(
      EffectStackEntry{EffectStackEntry::EffectType::kEffect, current_effect_,
                       current_clip_, current_effect_id_});
  current_effect_ = next_effect;
  current_clip_ = next_effect->OutputClip();
  current_effect_id_ = effect_node.id;

  dummy_layer->set_property_tree_sequence_number(sequence_number_);
  dummy_layer->SetTransformTreeIndex(kSecondaryRootNodeId);
  dummy_layer->SetClipTreeIndex(output_clip_id);
  dummy_layer->SetEffectTreeIndex(effect_node.id);
  dummy_layer->SetScrollTreeIndex(kRealRootNodeId);

  return true;
}

}  // namespace blink
