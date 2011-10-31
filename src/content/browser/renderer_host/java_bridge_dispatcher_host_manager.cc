// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/java_bridge_dispatcher_host_manager.h"

#include "base/utf_string_conversions.h"
#include "content/browser/renderer_host/java_bridge_dispatcher_host.h"
#include "content/browser/tab_contents/tab_contents.h"

JavaBridgeDispatcherHostManager::JavaBridgeDispatcherHostManager(
    TabContents* tab_contents)
    : TabContentsObserver(tab_contents) {
}

JavaBridgeDispatcherHostManager::~JavaBridgeDispatcherHostManager() {
  DCHECK_EQ(0U, instances_.size());
}

void JavaBridgeDispatcherHostManager::AddNamedObject(const string16& name,
    NPObject* object) {
  // Record this object in a map so that we can add it into RenderViewHosts
  // created later. We don't need to take a reference because each
  // JavaBridgeDispatcherHost will do so. The object can't be deleted until
  // after RemoveNamedObject() is called, at which time we remove the entry
  // from the map.
  DCHECK(!instances_.empty()) << "There must be at least one instance present";
  objects_[name] = object;

  for (InstanceMap::iterator iter = instances_.begin();
      iter != instances_.end(); ++iter) {
    iter->second->AddNamedObject(name, object);
  }
}

void JavaBridgeDispatcherHostManager::RemoveNamedObject(const string16& name) {
  ObjectMap::iterator iter = objects_.find(name);
  if (iter == objects_.end()) {
    return;
  }

  objects_.erase(iter);

  for (InstanceMap::iterator iter = instances_.begin();
      iter != instances_.end(); ++iter) {
    iter->second->RemoveNamedObject(name);
  }
}

void JavaBridgeDispatcherHostManager::RenderViewCreated(
    RenderViewHost* render_view_host) {
  // Creates a JavaBridgeDispatcherHost for the specified RenderViewHost and
  // adds all currently registered named objects to the new instance.
  scoped_refptr<JavaBridgeDispatcherHost> instance =
      new JavaBridgeDispatcherHost(render_view_host);

  for (ObjectMap::const_iterator iter = objects_.begin();
      iter != objects_.end(); ++iter) {
    instance->AddNamedObject(iter->first, iter->second);
  }

  instances_[render_view_host] = instance;
}

void JavaBridgeDispatcherHostManager::RenderViewDeleted(
    RenderViewHost* render_view_host) {
  instances_.erase(render_view_host);
}

void JavaBridgeDispatcherHostManager::TabContentsDestroyed(
    TabContents* tab_contents) {
  // When the tab is shutting down, the TabContents clears its observers before
  // it kills all of its RenderViewHosts, so we won't get a call to
  // RenderViewDeleted() for all RenderViewHosts.
  instances_.clear();
}
