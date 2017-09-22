// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/http_server_properties_pref_delegate.h"

#include "base/files/file_path.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/values.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service_factory.h"

const char kPrefPath[] = "net.http_server_properties";

namespace content {

HttpServerPropertiesPrefDelegate::HttpServerPropertiesPrefDelegate(
    const base::FilePath& file_path) {
  scoped_refptr<JsonPrefStore> json_pref_store(new JsonPrefStore(
      file_path,
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskShutdownBehavior::BLOCK_SHUTDOWN,
           base::TaskPriority::BACKGROUND})));
  PrefServiceFactory pref_service_factory;
  pref_service_factory.set_user_prefs(json_pref_store);
  scoped_refptr<PrefRegistrySimple> registry(new PrefRegistrySimple());
  registry->RegisterDictionaryPref(kPrefPath);
  pref_service_ = pref_service_factory.Create(registry.get());
  pref_change_registrar_.Init(pref_service_.get());
}

HttpServerPropertiesPrefDelegate::~HttpServerPropertiesPrefDelegate() {}

bool HttpServerPropertiesPrefDelegate::HasServerProperties() {
  return pref_service_->HasPrefPath(kPrefPath);
}

const base::DictionaryValue&
HttpServerPropertiesPrefDelegate::GetServerProperties() const {
  // Guaranteed not to return null when the pref is registered
  // (RegisterProfilePrefs was called).
  return *pref_service_->GetDictionary(kPrefPath);
}

void HttpServerPropertiesPrefDelegate::SetServerProperties(
    const base::DictionaryValue& value) {
  return pref_service_->Set(kPrefPath, value);
}

void HttpServerPropertiesPrefDelegate::StartListeningForUpdates(
    const base::Closure& callback) {
  pref_change_registrar_.Add(kPrefPath, callback);
}

void HttpServerPropertiesPrefDelegate::StopListeningForUpdates() {
  pref_change_registrar_.RemoveAll();
}

}  // namespace content
