// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_EXTENSIONS_MOJOM_ACCESSIBILITY_TRAITS_H_
#define CHROME_COMMON_EXTENSIONS_MOJOM_ACCESSIBILITY_TRAITS_H_

#include "base/strings/string_split.h"
#include "chrome/common/extensions/mojom/accessibility.mojom-shared.h"
#include "mojo/public/cpp/bindings/enum_traits.h"
#include "mojo/public/cpp/bindings/struct_traits.h"
#include "ui/accessibility/ax_enums.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/accessibility/ax_relative_bounds.h"
#include "ui/accessibility/ax_tree_data.h"
#include "ui/accessibility/ax_tree_update.h"
#include "ui/gfx/transform.h"

namespace mojo {

// TODO(catmullings): Do EnumTraits for AXEventFrom, AXEvent.

template <>
struct EnumTraits<extensions::mojom::AXEvent, ui::AXEvent> {
  static extensions::mojom::AXEvent ToMojom(ui::AXEvent ax_event);
  static bool FromMojom(extensions::mojom::AXEvent ax_event, ui::AXEvent* out);
};

template <>
struct EnumTraits<extensions::mojom::AXStringAttribute, ui::AXStringAttribute> {
  static extensions::mojom::AXStringAttribute ToMojom(
      ui::AXStringAttribute ax_string_attribute);
  static bool FromMojom(
      extensions::mojom::AXStringAttribute ax_string_attribute,
      ui::AXStringAttribute* out);
};

template <>
struct EnumTraits<extensions::mojom::AXIntAttribute, ui::AXIntAttribute> {
  static extensions::mojom::AXIntAttribute ToMojom(
      ui::AXIntAttribute ax_int_attribute);
  static bool FromMojom(extensions::mojom::AXIntAttribute ax_int_attribute,
                        ui::AXIntAttribute* out);
};

template <>
struct EnumTraits<extensions::mojom::AXFloatAttribute, ui::AXFloatAttribute> {
  static extensions::mojom::AXFloatAttribute ToMojom(
      ui::AXFloatAttribute ax_float_attribute);
  static bool FromMojom(extensions::mojom::AXFloatAttribute ax_float_attribute,
                        ui::AXFloatAttribute* out);
};

template <>
struct EnumTraits<extensions::mojom::AXBoolAttribute, ui::AXBoolAttribute> {
  static extensions::mojom::AXBoolAttribute ToMojom(
      ui::AXBoolAttribute ax_bool_attribute);
  static bool FromMojom(extensions::mojom::AXBoolAttribute ax_bool_attribute,
                        ui::AXBoolAttribute* out);
};

template <>
struct EnumTraits<extensions::mojom::AXIntListAttribute,
                  ui::AXIntListAttribute> {
  static extensions::mojom::AXIntListAttribute ToMojom(
      ui::AXIntListAttribute ax_int_list_attribute);
  static bool FromMojom(
      extensions::mojom::AXIntListAttribute ax_int_list_attribute,
      ui::AXIntListAttribute* out);
};

template <>
struct EnumTraits<extensions::mojom::AXRole, ui::AXRole> {
  static extensions::mojom::AXRole ToMojom(ui::AXRole ax_role);
  static bool FromMojom(extensions::mojom::AXRole ax_role, ui::AXRole* out);
};

template <>
struct EnumTraits<extensions::mojom::AXTextAffinity, ui::AXTextAffinity> {
  static extensions::mojom::AXTextAffinity ToMojom(
      ui::AXTextAffinity ax_text_affinity);
  static bool FromMojom(extensions::mojom::AXTextAffinity ax_text_affinity,
                        ui::AXTextAffinity* out);
};

template <>
struct StructTraits<extensions::mojom::AXTreeDataDataView, ui::AXTreeData> {
  static int32_t tree_id(const ui::AXTreeData& r) { return r.tree_id; }
  static int32_t parent_tree_id(const ui::AXTreeData& r) {
    return r.parent_tree_id;
  }
  static int32_t focused_tree_id(const ui::AXTreeData& r) {
    return r.focused_tree_id;
  }
  static const std::string& url(const ui::AXTreeData& r) { return r.url; }
  static const std::string& title(const ui::AXTreeData& r) { return r.title; }
  static const std::string& mimetype(const ui::AXTreeData& r) {
    return r.mimetype;
  }
  static const std::string& doctype(const ui::AXTreeData& r) {
    return r.doctype;
  }
  static bool loaded(const ui::AXTreeData& r) { return r.loaded; }
  static float loading_progress(const ui::AXTreeData& r) {
    return r.loading_progress;
  }
  static int32_t focus_id(const ui::AXTreeData& r) { return r.focus_id; }
  static int32_t sel_anchor_object_id(const ui::AXTreeData& r) {
    return r.sel_anchor_object_id;
  }
  static int32_t sel_anchor_offset(const ui::AXTreeData& r) {
    return r.sel_anchor_offset;
  }
  static ui::AXTextAffinity sel_anchor_affinity(const ui::AXTreeData& r) {
    return r.sel_anchor_affinity;
  }
  static int32_t sel_focus_object_id(const ui::AXTreeData& r) {
    return r.sel_focus_object_id;
  }
  static int32_t sel_focus_offset(const ui::AXTreeData& r) {
    return r.sel_focus_offset;
  }
  static ui::AXTextAffinity sel_focus_affinity(const ui::AXTreeData& r) {
    return r.sel_focus_affinity;
  }
  static bool Read(extensions::mojom::AXTreeDataDataView data,
                   ui::AXTreeData* out) {
    // TODO(catmullings): Determine when the received data is invalid and return
    // false.
    out = new ui::AXTreeData();
    return true;
  }
};

template <>
struct StructTraits<extensions::mojom::AXRelativeBoundsDataView,
                    ui::AXRelativeBounds> {
  static int32_t offset_container_id(const ui::AXRelativeBounds& r) {
    return r.offset_container_id;
  }
  static const gfx::RectF bounds(const ui::AXRelativeBounds& r) {
    return r.bounds;
  }
  static const gfx::Transform transform(const ui::AXRelativeBounds& r) {
    return *r.transform.get();
  }
  static bool Read(extensions::mojom::AXRelativeBoundsDataView data,
                   ui::AXRelativeBounds* out) {
    // TODO(catmullings): Determine when the received data is invalid and return
    // false.
    out = new ui::AXRelativeBounds();
    return true;
  }
};

template <>
struct StructTraits<extensions::mojom::AXStringAttributePairDataView,
                    std::pair<ui::AXStringAttribute, std::string>> {
  static ui::AXStringAttribute attribute(
      const std::pair<ui::AXStringAttribute, std::string>& out) {
    return out.first;
  }
  static std::string value(
      const std::pair<ui::AXStringAttribute, std::string>& out) {
    return out.second;
  }
  static bool Read(extensions::mojom::AXStringAttributePairDataView data,
                   std::pair<ui::AXStringAttribute, std::string>* out) {
    // TODO(catmullings): Determine when the received data is invalid and return
    // false.
    out = new std::pair<ui::AXStringAttribute, std::string>;
    return true;
  }
};

template <>
struct StructTraits<extensions::mojom::StringPairDataView,
                    std::pair<std::string, std::string>> {
  static std::string pair1(const std::pair<std::string, std::string>& out) {
    return out.first;
  }
  static std::string pair2(const std::pair<std::string, std::string>& out) {
    return out.second;
  }
  static bool Read(extensions::mojom::StringPairDataView data,
                   std::pair<std::string, std::string>* out) {
    // TODO(catmullings): Determine when the received data is invalid and return
    // false.
    out = new std::pair<std::string, std::string>;
    return true;
  }
};

template <>
struct StructTraits<extensions::mojom::AXIntAttributePairDataView,
                    std::pair<ui::AXIntAttribute, int32_t>> {
  static ui::AXIntAttribute attribute(
      const std::pair<ui::AXIntAttribute, int32_t>& out) {
    return out.first;
  }
  static int32_t value(const std::pair<ui::AXIntAttribute, int32_t>& out) {
    return out.second;
  }
  static bool Read(extensions::mojom::AXIntAttributePairDataView data,
                   std::pair<ui::AXIntAttribute, int32_t>* out) {
    // TODO(catmullings): Determine when the received data is invalid and return
    // false.
    out = new std::pair<ui::AXIntAttribute, int32_t>;
    return true;
  }
};

template <>
struct StructTraits<extensions::mojom::AXFloatAttributePairDataView,
                    std::pair<ui::AXFloatAttribute, float>> {
  static ui::AXFloatAttribute attribute(
      const std::pair<ui::AXFloatAttribute, float>& out) {
    return out.first;
  }
  static float value(const std::pair<ui::AXFloatAttribute, float>& out) {
    return out.second;
  }
  static bool Read(extensions::mojom::AXFloatAttributePairDataView data,
                   std::pair<ui::AXFloatAttribute, float>* out) {
    // TODO(catmullings): Determine when the received data is
    // invalid and return
    // false.
    out = new std::pair<ui::AXFloatAttribute, float>;
    return true;
  }
};

template <>
struct StructTraits<extensions::mojom::AXBoolAttributePairDataView,
                    std::pair<ui::AXBoolAttribute, bool>> {
  static ui::AXBoolAttribute attribute(
      const std::pair<ui::AXBoolAttribute, bool>& out) {
    return out.first;
  }
  static bool value(const std::pair<ui::AXBoolAttribute, bool>& out) {
    return out.second;
  }
  static bool Read(extensions::mojom::AXBoolAttributePairDataView data,
                   std::pair<ui::AXBoolAttribute, bool>* out) {
    // TODO(catmullings): Determine when the received data is invalid and return
    // false.
    out = new std::pair<ui::AXBoolAttribute, bool>;
    return true;
  }
};

template <>
struct StructTraits<extensions::mojom::AXIntListAttributePairDataView,
                    std::pair<ui::AXIntListAttribute, std::vector<int32_t>>> {
  static ui::AXIntListAttribute attribute(
      const std::pair<ui::AXIntListAttribute, std::vector<int32_t>>& out) {
    return out.first;
  }
  static std::vector<int32_t> values(
      const std::pair<ui::AXIntListAttribute, std::vector<int32_t>>& out) {
    return out.second;
  }
  static bool Read(
      extensions::mojom::AXIntListAttributePairDataView data,
      std::pair<ui::AXIntListAttribute, std::vector<int32_t>>* out) {
    // TODO(catmullings): Determine when the received data is invalid and return
    // false.
    out = new std::pair<ui::AXIntListAttribute, std::vector<int32_t>>;
    return true;
  }
};

template <>
struct StructTraits<extensions::mojom::AXNodeDataDataView, ui::AXNodeData> {
  static int32_t id(const ui::AXNodeData& r) { return r.id; }
  static ui::AXRole role(const ui::AXNodeData& r) { return r.role; }
  static int32_t state(const ui::AXNodeData& r) { return r.state; }
  static int32_t actions(const ui::AXNodeData& r) { return r.actions; }
  static const gfx::RectF location(const ui::AXNodeData& r) {
    return r.location;
  }
  static const gfx::Transform transform(const ui::AXNodeData& r) {
    return *r.transform.get();
  }
  static std::vector<std::pair<ui::AXStringAttribute, std::string>>
  string_attributes(const ui::AXNodeData& r) {
    return r.string_attributes;
  }
  static std::vector<std::pair<ui::AXIntAttribute, int32_t>> int_attributes(
      const ui::AXNodeData& r) {
    return r.int_attributes;
  }
  static std::vector<std::pair<ui::AXFloatAttribute, float>> float_attributes(
      const ui::AXNodeData& r) {
    return r.float_attributes;
  }
  static std::vector<std::pair<ui::AXBoolAttribute, bool>> bool_attributes(
      const ui::AXNodeData& r) {
    return r.bool_attributes;
  }
  static std::vector<std::pair<ui::AXIntListAttribute, std::vector<int32_t>>>
  intlist_attributes(const ui::AXNodeData& r) {
    return r.intlist_attributes;
  }
  static base::StringPairs html_attributes(const ui::AXNodeData& r) {
    return r.html_attributes;
  }
  static std::vector<int32_t> child_ids(const ui::AXNodeData& r) {
    return r.child_ids;
  }
  static int32_t offset_container_id(const ui::AXNodeData& r) {
    return r.offset_container_id;
  }
  static bool Read(extensions::mojom::AXNodeDataDataView data,
                   ui::AXNodeData* out) {
    // TODO(catmullings): Determine when the received data is invalid and return
    // false.
    out = new ui::AXNodeData();
    return true;
  }
};

template <>
struct StructTraits<extensions::mojom::AXTreeUpdateDataView, ui::AXTreeUpdate> {
  static bool has_tree_data(const ui::AXTreeUpdate& r) {
    return r.has_tree_data;
  }
  static ui::AXTreeData tree_data(const ui::AXTreeUpdate& r) {
    return r.tree_data;
  }
  static int32_t node_id_to_clear(const ui::AXTreeUpdate& r) {
    return r.node_id_to_clear;
  }
  static int32_t root_id(const ui::AXTreeUpdate& r) { return r.root_id; }
  static std::vector<ui::AXNodeData> nodes(const ui::AXTreeUpdate& r) {
    return r.nodes;
  }
  static bool Read(extensions::mojom::AXTreeUpdateDataView data,
                   ui::AXTreeUpdate* out) {
    // TODO(catmullings): Determine when the received data is invalid and return
    // false.
    out = new ui::AXTreeUpdate();
    return true;
  }
};

}  // namespace mojo

#endif  // CHROME_COMMON_EXTENSIONS_MOJOM_ACCESSIBILITY_TRAITS_H_
