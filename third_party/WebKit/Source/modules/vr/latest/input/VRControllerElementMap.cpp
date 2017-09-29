// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/latest/input/VRControllerElementMap.h"

namespace blink {

class VRControllerElementMapIterationSource final
    : public PairIterable<String, VRControllerElement*>::IterationSource {
 public:
  VRControllerElementMapIterationSource(
      const HeapHashMap<String, Member<VRControllerElement>>& map) {
    LOG(ERROR) << "Created VRControllerElementMapIterationSource";
    for (const auto name : map.Keys()) {
      element_names_.push_back(name);
      element_objects_.push_back(map.at(name));
    }
  }

  bool Next(ScriptState* scrip_state,
            String& key,
            VRControllerElement*& element,
            ExceptionState&) override {
    LOG(ERROR) << __FUNCTION__;
    if (current_index_ == element_names_.size()) {
      LOG(ERROR) << " (X) No elements left";
      return false;
    }
    key = element_names_[current_index_];
    element = element_objects_[current_index_];
    ++current_index_;
    LOG(ERROR) << " (!) Returning " << key << ", " << element;
    return true;
  }

  DEFINE_INLINE_VIRTUAL_TRACE() {
    visitor->Trace(element_objects_);
    PairIterable<String, VRControllerElement*>::IterationSource::Trace(visitor);
  }

 private:
  // For sequential iteration (e.g. Next()).
  Vector<String> element_names_;
  HeapVector<Member<VRControllerElement>> element_objects_;
  unsigned current_index_;
};

VRControllerElementMap::VRControllerElementMap(
    const HeapHashMap<String, Member<VRControllerElement>>& element_map)
    : element_map_(element_map) {
  LOG(ERROR) << "Created VRControllerElementMap";
  for (const auto name : element_map_.Keys()) {
    LOG(ERROR) << " - " << name;
  }
}

PairIterable<String, VRControllerElement*>::IterationSource*
VRControllerElementMap::StartIteration(ScriptState*, ExceptionState&) {
  LOG(ERROR) << __FUNCTION__;
  return new VRControllerElementMapIterationSource(element_map_);
}

bool VRControllerElementMap::GetMapEntry(ScriptState*,
                                         const String& key,
                                         VRControllerElement*& element,
                                         ExceptionState&) {
  LOG(ERROR) << __FUNCTION__ << ", key: " << key;
  if (element_map_.Contains(key)) {
    element = element_map_.at(key);
    return true;
  }

  return false;
}

}  // namespace blink
