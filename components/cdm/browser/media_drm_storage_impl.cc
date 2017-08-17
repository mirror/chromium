// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cdm/browser/media_drm_storage_impl.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/value_conversions.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/navigation_handle.h"

// The storage will be managed by PrefService. All data will be stored in a
// dictionary under the key "media.media_drm_storage". The dictionary is
// structured as follows:
//
// {
//     $origin: {
//         "origin_id": $unguessable_origin_id
//         "creation_time": $creation_time
//         "sessions" : {
//             $session_id: {
//                 "key_set_id": $key_set_id,
//                 "mime_type": $mime_type,
//                 "creation_time": $creation_time
//             },
//             # more session_id map...
//         }
//     },
//     # more origin map...
// }

namespace cdm {

namespace {

const char kMediaDrmStorage[] = "media.media_drm_storage";
const char kCreationTime[] = "creation_time";
const char kSessions[] = "sessions";
const char kKeySetId[] = "key_set_id";
const char kMimeType[] = "mime_type";
const char kOriginId[] = "origin_id";

// Data in origin dict without sessions dict.
struct OriginData {
  base::UnguessableToken origin_id;
  base::Time provision_time;

  OriginData() = default;

  OriginData(const base::UnguessableToken& origin_id)
      : origin_id(origin_id), provision_time(base::Time::Now()) {
    DCHECK(origin_id);
  }

  std::unique_ptr<base::Value> ToValue() const {
    auto dict = base::MakeUnique<base::DictionaryValue>();

    dict->Set(kOriginId, base::CreateUnguessableTokenValue(origin_id));
    dict->SetDouble(kCreationTime, provision_time.ToDoubleT());

    return dict;
  }
};

// Data in session dict.
struct SessionData {
  std::vector<uint8_t> key_set_id;
  std::string mime_type;
  base::Time creation_time;

  SessionData() = default;

  SessionData(const std::vector<uint8_t>& key_set_id,
              const std::string& mime_type)
      : key_set_id(key_set_id),
        mime_type(mime_type),
        creation_time(base::Time::Now()) {}

  std::unique_ptr<base::Value> ToValue() const {
    auto dict = base::MakeUnique<base::DictionaryValue>();

    dict->SetString(
        kKeySetId, std::string(reinterpret_cast<const char*>(key_set_id.data()),
                               key_set_id.size()));
    dict->SetString(kMimeType, mime_type);
    dict->SetDouble(kCreationTime, creation_time.ToDoubleT());

    return dict;
  }
};

bool GetTimeFromDict(const base::DictionaryValue& dict,
                     const std::string& key,
                     base::Time* time) {
  DCHECK(time);

  double time_double = 0.;
  if (!dict.GetDouble(key, &time_double))
    return false;

  base::Time time_maybe_null = base::Time::FromDoubleT(time_double);
  if (time_maybe_null.is_null())
    return false;

  *time = time_maybe_null;
  return true;
}

// Convert |session_dict| to SessionData. |session_dict| contains information
// for an offline license session. Return false if |session_dict| has any
// corruption, e.g. format error, missing fields, invalid data.
bool GetDictValueAsSessionData(const base::DictionaryValue& session_dict,
                               SessionData* session_data) {
  DCHECK(session_data);

  std::string key_set_id_string;
  if (!session_dict.GetString(kKeySetId, &key_set_id_string))
    return false;

  std::string mime_type;
  if (!session_dict.GetString(kMimeType, &mime_type))
    return false;

  base::Time time;
  if (!GetTimeFromDict(session_dict, kCreationTime, &time))
    return false;

  session_data->key_set_id.assign(key_set_id_string.begin(),
                                  key_set_id_string.end());
  session_data->mime_type = std::move(mime_type);
  session_data->creation_time = time;

  return true;
}

// Convert |origin_dict| to OriginData. |origin_dict| contains information
// related to origin provision. Return false if |origin_dict| has any
// corruption, e.g. format error, missing fields, invalid value.
bool GetDictValueAsOriginData(const base::DictionaryValue& origin_dict,
                              OriginData* origin_data) {
  DCHECK(origin_data);

  const base::Value* origin_id_value = nullptr;
  if (!origin_dict.Get(kOriginId, &origin_id_value))
    return false;

  base::UnguessableToken origin_id;
  if (!base::GetValueAsUnguessableToken(*origin_id_value, &origin_id))
    return false;

  base::Time time;
  if (!GetTimeFromDict(origin_dict, kCreationTime, &time))
    return false;

  origin_data->origin_id = origin_id;
  origin_data->provision_time = time;

  return true;
}

#if DCHECK_IS_ON()
// Returns whether |dict| has a value assocaited with the |key|.
bool HasEntry(const base::DictionaryValue& dict, const std::string& key) {
  return dict.GetDictionaryWithoutPathExpansion(key, nullptr);
}
#endif

// Clear sessions whose creation time falls in [start, end] from
// |sessions_dict|. This function also cleans corruption data and should never
// fail.
void ClearSessionDataForTimePeriod(base::DictionaryValue* sessions_dict,
                                   base::Time start,
                                   base::Time end) {
  std::vector<std::string> sessions_to_clear;
  for (const auto& key_value : *sessions_dict) {
    const std::string& session_id = key_value.first;

    base::DictionaryValue* session_dict;
    if (!key_value.second->GetAsDictionary(&session_dict)) {
      DVLOG(1) << "Session dict for " << session_id
               << " is corrupted, removing.";
      sessions_to_clear.push_back(session_id);
      continue;
    }

    SessionData session_data;
    if (!GetDictValueAsSessionData(*session_dict, &session_data)) {
      DVLOG(1) << "Session data for " << session_id
               << " is corrupted, removing.";
      sessions_to_clear.push_back(session_id);
      continue;
    }

    if (session_data.creation_time >= start &&
        session_data.creation_time <= end) {
      sessions_to_clear.push_back(session_id);
      continue;
    }
  }

  // Remove session data.
  for (const auto& session_id : sessions_to_clear)
    sessions_dict->RemoveWithoutPathExpansion(session_id, nullptr);
}

// 1. Removes the session data from origin dict if the session's creation time
// falls in [|start|, |end|] and |filter| returns true on its origin.
// 2. Removes the origin data if all of the sessions are removed.
// 3. Returns a list of origin IDs to unprovision.
std::vector<base::UnguessableToken> ClearMatchingLicenseData(
    base::DictionaryValue* storage_dict,
    base::Time start,
    base::Time end,
    const base::RepeatingCallback<bool(const GURL&)>& filter) {
  std::vector<std::string> origins_to_delete;
  std::vector<base::UnguessableToken> origin_ids_to_unprovision;

  for (const auto& key_value : *storage_dict) {
    const std::string& origin_str = key_value.first;

    if (filter && !filter.Run(GURL(origin_str)))
      continue;

    base::DictionaryValue* origin_dict;
    if (!key_value.second->GetAsDictionary(&origin_dict)) {
      DVLOG(1) << "Origin dict for " << origin_str
               << " is corrupted, removing.";
      origins_to_delete.push_back(origin_str);
      continue;
    }

    OriginData origin_data;
    if (!GetDictValueAsOriginData(*origin_dict, &origin_data)) {
      DVLOG(1) << "Origin data for " << origin_str
               << " is corrupted, removing.";
      origins_to_delete.push_back(origin_str);
      continue;
    }

    if (origin_data.provision_time > end)
      continue;

    base::DictionaryValue* sessions;
    if (!origin_dict->GetDictionary(kSessions, &sessions)) {
      // The origin is provisioned, but no persistent license is installed.
      origins_to_delete.push_back(origin_str);
      origin_ids_to_unprovision.push_back(origin_data.origin_id);
      continue;
    }

    ClearSessionDataForTimePeriod(sessions, start, end);

    if (sessions->empty()) {
      // Session data will be removed when removing origin data.
      origins_to_delete.push_back(origin_str);
      origin_ids_to_unprovision.push_back(origin_data.origin_id);
    }
  }

  // Remove origin data.
  for (const auto& origin_str : origins_to_delete)
    storage_dict->RemoveWithoutPathExpansion(origin_str, nullptr);

  return origin_ids_to_unprovision;
}

}  // namespace

// static
void MediaDrmStorageImpl::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kMediaDrmStorage);
}

// static
std::vector<base::UnguessableToken> MediaDrmStorageImpl::ClearMatchingLicenses(
    PrefService* pref_service,
    base::Time start,
    base::Time end,
    const base::RepeatingCallback<bool(const GURL&)>& filter) {
  DVLOG(1) << __func__ << ": Clear licenses [" << start << ", " << end << "]";

  DictionaryPrefUpdate update(pref_service, kMediaDrmStorage);
  base::DictionaryValue* storage_dict = update.Get();
  DCHECK(storage_dict);

  return ClearMatchingLicenseData(storage_dict, start, end, filter);
}

MediaDrmStorageImpl::MediaDrmStorageImpl(
    content::RenderFrameHost* render_frame_host,
    PrefService* pref_service,
    const url::Origin& origin,
    media::mojom::MediaDrmStorageRequest request)
    : render_frame_host_(render_frame_host),
      pref_service_(pref_service),
      origin_string_(origin.Serialize()),
      binding_(this, std::move(request)) {
  DVLOG(1) << __func__ << ": origin = " << origin;
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(pref_service_);
  DCHECK(!origin_string_.empty());

  // |this| owns |binding_|, so unretained is safe.
  binding_.set_connection_error_handler(
      base::Bind(&MediaDrmStorageImpl::Close, base::Unretained(this)));
}

MediaDrmStorageImpl::~MediaDrmStorageImpl() {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());
}

void MediaDrmStorageImpl::Initialize(InitializeCallback callback) {
  DVLOG(1) << __func__ << ": origin = " << origin_string_;
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!origin_id_);

  DictionaryPrefUpdate update(pref_service_, kMediaDrmStorage);
  base::DictionaryValue* storage_dict = update.Get();
  DCHECK(storage_dict);

  base::DictionaryValue* origin_dict = nullptr;
  // The origin string may contain dots. Do not use path expansion.
  bool exist = storage_dict->GetDictionaryWithoutPathExpansion(origin_string_,
                                                               &origin_dict);

  base::UnguessableToken origin_id;
  if (exist) {
    DCHECK(origin_dict);

    OriginData origin_data;
    if (GetDictValueAsOriginData(*origin_dict, &origin_data))
      origin_id = origin_data.origin_id;
  }

  // |origin_id| can be empty even if |origin_dict| exists. This can happen if
  // |origin_dict| is created with an old version app.
  if (origin_id.is_empty())
    origin_id = base::UnguessableToken::Create();

  origin_id_ = origin_id;

  DCHECK(origin_id);
  std::move(callback).Run(origin_id);
}

void MediaDrmStorageImpl::OnProvisioned(OnProvisionedCallback callback) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!IsInitialized()) {
    DVLOG(1) << __func__ << ": Not initialized.";
    std::move(callback).Run(false);
    return;
  }

  DictionaryPrefUpdate update(pref_service_, kMediaDrmStorage);
  base::DictionaryValue* storage_dict = update.Get();
  DCHECK(storage_dict);

  // The origin string may contain dots. Do not use path expansion.
  DVLOG_IF(1, HasEntry(*storage_dict, origin_string_))
      << __func__ << ": Entry for origin " << origin_string_
      << " already exists and will be cleared";

  storage_dict->SetWithoutPathExpansion(origin_string_,
                                        OriginData(origin_id_).ToValue());
  std::move(callback).Run(true);
}

void MediaDrmStorageImpl::SavePersistentSession(
    const std::string& session_id,
    media::mojom::SessionDataPtr session_data,
    SavePersistentSessionCallback callback) {
  DVLOG(2) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!IsInitialized()) {
    DVLOG(1) << __func__ << ": Not initialized.";
    std::move(callback).Run(false);
    return;
  }

  DictionaryPrefUpdate update(pref_service_, kMediaDrmStorage);
  base::DictionaryValue* storage_dict = update.Get();
  DCHECK(storage_dict);

  base::DictionaryValue* origin_dict = nullptr;
  // The origin string may contain dots. Do not use path expansion.
  storage_dict->GetDictionaryWithoutPathExpansion(origin_string_, &origin_dict);

  // This could happen if the profile is removed, but the device is still
  // provisioned for the origin. In this case, just create a new entry.
  // Since we're using random origin ID in MediaDrm, it's rare to enter the if
  // branch. Deleting the profile causes reprovisioning of the origin.
  if (!origin_dict) {

    DVLOG(1) << __func__ << ": Entry for origin " << origin_string_
             << " does not exist; create a new one.";
    storage_dict->SetWithoutPathExpansion(origin_string_,
                                          OriginData(origin_id_).ToValue());
    storage_dict->GetDictionaryWithoutPathExpansion(origin_string_,
                                                    &origin_dict);
    DCHECK(origin_dict);
  }

  base::DictionaryValue* sessions_dict = nullptr;
  if (!origin_dict->GetDictionary(kSessions, &sessions_dict)) {
    DVLOG(2) << __func__ << ": No session exists; creating a new dict.";
    origin_dict->Set(kSessions, base::MakeUnique<base::DictionaryValue>());
    origin_dict->GetDictionary(kSessions, &sessions_dict);
    DCHECK(sessions_dict);
  }

  DVLOG_IF(1, HasEntry(*sessions_dict, session_id))
      << __func__ << ": Session ID already exists and will be replaced.";

  sessions_dict->SetWithoutPathExpansion(
      session_id,
      SessionData(session_data->key_set_id, session_data->mime_type).ToValue());

  std::move(callback).Run(true);
}

void MediaDrmStorageImpl::LoadPersistentSession(
    const std::string& session_id,
    LoadPersistentSessionCallback callback) {
  DVLOG(2) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!IsInitialized()) {
    DVLOG(1) << __func__ << ": Not initialized.";
    std::move(callback).Run(nullptr);
    return;
  }

  const base::DictionaryValue* storage_dict =
      pref_service_->GetDictionary(kMediaDrmStorage);

  const base::DictionaryValue* origin_dict = nullptr;
  // The origin string may contain dots. Do not use path expansion.
  storage_dict->GetDictionaryWithoutPathExpansion(origin_string_, &origin_dict);
  if (!origin_dict) {
    DVLOG(1) << __func__
             << ": Failed to save persistent session data; entry for origin "
             << origin_string_ << " does not exist.";
    std::move(callback).Run(nullptr);
    return;
  }

  const base::DictionaryValue* sessions_dict = nullptr;
  if (!origin_dict->GetDictionary(kSessions, &sessions_dict)) {
    DVLOG(2) << __func__ << ": Sessions dictionary does not exist.";
    std::move(callback).Run(nullptr);
    return;
  }

  const base::DictionaryValue* session_dict = nullptr;
  if (!sessions_dict->GetDictionaryWithoutPathExpansion(session_id,
                                                        &session_dict)) {
    DVLOG(2) << __func__ << ": Session dictionary does not exist.";
    std::move(callback).Run(nullptr);
    return;
  }

  SessionData session_data;
  if (!GetDictValueAsSessionData(*session_dict, &session_data)) {
    DVLOG(2) << __func__ << ": Failed to read session data.";
    std::move(callback).Run(nullptr);
    return;
  }

  std::move(callback).Run(media::mojom::SessionData::New(
      session_data.key_set_id, session_data.mime_type));
}

void MediaDrmStorageImpl::RemovePersistentSession(
    const std::string& session_id,
    RemovePersistentSessionCallback callback) {
  DVLOG(2) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!IsInitialized()) {
    DVLOG(1) << __func__ << ": Not initialized.";
    std::move(callback).Run(false);
    return;
  }

  DictionaryPrefUpdate update(pref_service_, kMediaDrmStorage);
  base::DictionaryValue* storage_dict = update.Get();
  DCHECK(storage_dict);

  base::DictionaryValue* origin_dict = nullptr;
  // The origin string may contain dots. Do not use path expansion.
  storage_dict->GetDictionaryWithoutPathExpansion(origin_string_, &origin_dict);
  if (!origin_dict) {
    DVLOG(1) << __func__ << ": Entry for rigin " << origin_string_
             << " does not exist.";
    std::move(callback).Run(true);
    return;
  }

  base::DictionaryValue* sessions_dict = nullptr;
  if (!origin_dict->GetDictionary(kSessions, &sessions_dict)) {
    DVLOG(2) << __func__ << ": Sessions dictionary does not exist.";
    std::move(callback).Run(true);
    return;
  }

  sessions_dict->RemoveWithoutPathExpansion(session_id, nullptr);
  std::move(callback).Run(true);
}

void MediaDrmStorageImpl::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (render_frame_host == render_frame_host_) {
    DVLOG(1) << __func__ << ": RenderFrame destroyed.";
    Close();
  }
}

void MediaDrmStorageImpl::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (navigation_handle->GetRenderFrameHost() == render_frame_host_) {
    DVLOG(1) << __func__ << ": Close connection on navigation.";
    Close();
  }
}

void MediaDrmStorageImpl::Close() {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  delete this;
}

}  // namespace cdm
