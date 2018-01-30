// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/script/ClassicPendingScript.h"

#include "bindings/core/v8/ScriptSourceCode.h"
#include "bindings/core/v8/ScriptStreamer.h"
#include "bindings/core/v8/V8BindingForCore.h"
#include "core/dom/Document.h"
#include "core/dom/ScriptableDocumentParser.h"
#include "core/frame/LocalFrame.h"
#include "core/loader/AllowedByNosniff.h"
#include "core/loader/DocumentLoader.h"
#include "core/loader/SubresourceIntegrityHelper.h"
#include "core/loader/resource/ScriptResource.h"
#include "core/script/DocumentWriteIntervention.h"
#include "core/script/ScriptLoader.h"
#include "platform/bindings/ScriptState.h"
#include "platform/loader/fetch/CachedMetadata.h"
#include "platform/loader/fetch/MemoryCache.h"
#include "platform/loader/fetch/RawResource.h"
#include "platform/loader/fetch/ResourceClient.h"
#include "platform/wtf/text/Base64.h"
#include "public/platform/TaskType.h"

namespace blink {

ClassicPendingScript* ClassicPendingScript::Fetch(
    const KURL& url,
    Document& element_document,
    const ScriptFetchOptions& options,
    const WTF::TextEncoding& encoding,
    ScriptElementBase* element,
    FetchParameters::DeferOption defer) {
  FetchParameters params = options.CreateFetchParameters(
      url, element_document.GetSecurityOrigin(), encoding, defer);

  ClassicPendingScript* pending_script = new ClassicPendingScript(
      element, TextPosition(), ScriptSourceLocationType::kExternalFile, options,
      true /* is_external */);

  // [Intervention]
  // For users on slow connections, we want to avoid blocking the parser in
  // the main frame on script loads inserted via document.write, since it
  // can add significant delays before page content is displayed on the
  // screen.
  pending_script->intervened_ =
      MaybeDisallowFetchForDocWrittenScript(params, element_document);

  // https://html.spec.whatwg.org/#fetch-a-classic-script
  // Step 2. Set request's client to settings object. [spec text]
  //
  // Note: |element_document| corresponds to the settings object.
  ScriptResource::Fetch(params, element_document.Fetcher(), pending_script);
  pending_script->CheckState();
  return pending_script;
}

ClassicPendingScript* ClassicPendingScript::CreateInline(
    ScriptElementBase* element,
    const TextPosition& starting_position,
    ScriptSourceLocationType source_location_type,
    const ScriptFetchOptions& options) {
  ClassicPendingScript* pending_script =
      new ClassicPendingScript(element, starting_position, source_location_type,
                               options, false /* is_external */);
  pending_script->CheckState();
  return pending_script;
}

ClassicPendingScript::ClassicPendingScript(
    ScriptElementBase* element,
    const TextPosition& starting_position,
    ScriptSourceLocationType source_location_type,
    const ScriptFetchOptions& options,
    bool is_external)
    : PendingScript(element, starting_position),
      options_(options),
      base_url_for_inline_script_(
          is_external ? KURL() : element->GetDocument().BaseURL()),
      source_location_type_(source_location_type),
      is_external_(is_external),
      ready_state_(is_external ? kWaitingForResource : kReady),
      integrity_failure_(false),
      is_currently_streaming_(false) {
  CHECK(GetElement());
  MemoryCoordinator::Instance().RegisterClient(this);
}

ClassicPendingScript::~ClassicPendingScript() {}

NOINLINE void ClassicPendingScript::CheckState() const {
  // TODO(hiroshige): Turn these CHECK()s into DCHECK() before going to beta.
  CHECK(!prefinalizer_called_);
  CHECK(GetElement());
  CHECK_EQ(is_external_, !!GetResource());
  CHECK(GetResource() || !streamer_);
}

void ClassicPendingScript::Prefinalize() {
  // TODO(hiroshige): Consider moving this to ScriptStreamer's prefinalizer.
  // https://crbug.com/715309
  CancelStreaming();
  prefinalizer_called_ = true;
}

void ClassicPendingScript::DisposeInternal() {
  MemoryCoordinator::Instance().UnregisterClient(this);
  ClearResource();
  integrity_failure_ = false;
  CancelStreaming();
}

void ClassicPendingScript::StreamingFinished() {
  CheckState();
  DCHECK(streamer_);  // Should only be called by ScriptStreamer.
  DCHECK(IsCurrentlyStreaming());

  if (ready_state_ == kWaitingForStreaming) {
    FinishWaitingForStreaming();
  } else if (ready_state_ == kReadyStreaming) {
    FinishReadyStreaming();
  } else {
    NOTREACHED();
  }

  DCHECK(!IsCurrentlyStreaming());
}

void ClassicPendingScript::FinishWaitingForStreaming() {
  CheckState();
  DCHECK(GetResource());
  DCHECK_EQ(ready_state_, kWaitingForStreaming);

  bool error_occurred = GetResource()->ErrorOccurred() || integrity_failure_;
  AdvanceReadyState(error_occurred ? kErrorOccurred : kReady);
}

void ClassicPendingScript::FinishReadyStreaming() {
  CheckState();
  DCHECK(GetResource());
  DCHECK_EQ(ready_state_, kReadyStreaming);
  AdvanceReadyState(kReady);
}

void ClassicPendingScript::CancelStreaming() {
  if (!streamer_)
    return;

  streamer_->Cancel();
  streamer_ = nullptr;
  streamer_done_.Reset();
  is_currently_streaming_ = false;
  DCHECK(!IsCurrentlyStreaming());
}

void ClassicPendingScript::NotifyFinished(Resource* resource) {
  // The following SRI checks need to be here because, unfortunately, fetches
  // are not done purely according to the Fetch spec. In particular,
  // different requests for the same resource do not have different
  // responses; the memory cache can (and will) return the exact same
  // Resource object.
  //
  // For different requests, the same Resource object will be returned and
  // will not be associated with the particular request.  Therefore, when the
  // body of the response comes in, there's no way to validate the integrity
  // of the Resource object against a particular request (since there may be
  // several pending requests all tied to the identical object, and the
  // actual requests are not stored).
  //
  // In order to simulate the correct behavior, Blink explicitly does the SRI
  // checks here, when a PendingScript tied to a particular request is
  // finished (and in the case of a StyleSheet, at the point of execution),
  // while having proper Fetch checks in the fetch module for use in the
  // fetch JavaScript API. In a future world where the ResourceFetcher uses
  // the Fetch algorithm, this should be fixed by having separate Response
  // objects (perhaps attached to identical Resource objects) per request.
  //
  // See https://crbug.com/500701 for more information.
  CheckState();
  ScriptElementBase* element = GetElement();
  if (element) {
    SubresourceIntegrityHelper::DoReport(element->GetDocument(),
                                         GetResource()->IntegrityReportInfo());

    // It is possible to get back a script resource with integrity metadata
    // for a request with an empty integrity attribute. In that case, the
    // integrity check should be skipped, so this check ensures that the
    // integrity attribute isn't empty in addition to checking if the
    // resource has empty integrity metadata.
    if (!element->IntegrityAttributeValue().IsEmpty()) {
      integrity_failure_ = GetResource()->IntegrityDisposition() !=
                           ResourceIntegrityDisposition::kPassed;
    }
  }

  if (intervened_) {
    PossiblyFetchBlockedDocWriteScript(resource, element->GetDocument(),
                                       options_);
  }

  // We are now waiting for script streaming to finish.
  // If there is no script streamer, this step completes immediately.
  AdvanceReadyState(kWaitingForStreaming);
  if (streamer_)
    streamer_->NotifyFinished();
  else
    FinishWaitingForStreaming();
}

void ClassicPendingScript::DataReceived(Resource* resource,
                                        const char*,
                                        size_t) {
  if (streamer_)
    streamer_->NotifyAppendData(ToScriptResource(resource));
}

void ClassicPendingScript::Trace(blink::Visitor* visitor) {
  visitor->Trace(streamer_);
  ResourceClient::Trace(visitor);
  MemoryCoordinatorClient::Trace(visitor);
  PendingScript::Trace(visitor);
}

static const size_t kKeySize = 32;
static const size_t kKeyEntrySize =
    kKeySize + sizeof(uint32_t) + sizeof(size_t);
class CORE_EXPORT WrappingCacheMetadataHandler : public CachedMetadataHandler {
  static const uint32_t MAP_DATA_TYPE = 0xff;

 public:
  // Encoding of keyed map:
  //   - num_keys
  //     - key 1
  //     - type data 1
  //     - len data 1
  //       ...
  //     - key N
  //     - type data N
  //     - len data N
  //   - data for key 1
  //     ...
  //   - data for key N

  WrappingCacheMetadataHandler(CachedMetadataHandler* parent,
                               const DigestValue& key)
      : parent_(parent) {
    DCHECK_EQ(key.size(), kKeySize);
    memcpy(key_, key.data(), kKeySize);
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(parent_);
    CachedMetadataHandler::Trace(visitor);
  }

  static std::string KeyStr(const uint8_t key[kKeySize]) {
    Vector<char> result;
    WTF::Base64Encode(reinterpret_cast<const char*>(key), kKeySize, result);
    return {result.data(), result.size()};
  }

  void SetCachedMetadata(uint32_t data_type_id,
                         const char* new_key_data,
                         size_t new_key_data_size,
                         CacheType cache_type = kSendToPlatform) override {
    VLOG(4) << "CacheMux::SetCachedMetadata " << KeyStr(key_) << " "
            << data_type_id;

    scoped_refptr<CachedMetadata> parent_data = ParentData();
    Vector<char> new_data;

    if (!parent_data) {
      VLOG(4) << "CacheMux::SetCachedMetadata creating new parent data";
      new_data.ReserveInitialCapacity(sizeof(int) + kKeyEntrySize +
                                      new_key_data_size);

      int new_num_keys = 1;
      new_data.Append(reinterpret_cast<char*>(&new_num_keys), sizeof(int));
      new_data.Append(reinterpret_cast<char*>(key_), kKeySize);
      new_data.Append(reinterpret_cast<char*>(&data_type_id), sizeof(uint32_t));
      new_data.Append(reinterpret_cast<char*>(&new_key_data_size),
                      sizeof(size_t));
      new_data.Append(new_key_data, new_key_data_size);
    } else {
      VLOG(4) << "CacheMux::SetCachedMetadata modifying parent data";
      LogParentHeader(parent_data->Data(), parent_data->size());
      CHECK_GE(parent_data->size(), sizeof(int));

      const char* data = parent_data->Data();
      int num_keys = *reinterpret_cast<const int*>(data);
      data += sizeof(int);
      CHECK_GT(num_keys, 0);
      CHECK_GE(parent_data->size(), sizeof(int) + num_keys * kKeyEntrySize);

      size_t data_offset = 0;
      size_t data_len = 0;

      int i;
      for (i = 0; i < num_keys; ++i) {
        const uint8_t* key = reinterpret_cast<const uint8_t*>(data);
        data += kKeySize;
        // Ignore type
        data += sizeof(uint32_t);
        size_t key_data_len = *reinterpret_cast<const size_t*>(data);
        data += sizeof(size_t);

        if (memcmp(key, key_, kKeySize) == 0) {
          data_len = key_data_len;
          break;
        }

        data_offset += key_data_len;
      }
      size_t new_size = parent_data->size() - data_len + new_key_data_size;
      if (i == num_keys)
        new_size += kKeyEntrySize;

      new_data.ReserveInitialCapacity(new_size);

      if (i == num_keys) {
        int new_num_keys = num_keys + 1;
        new_data.Append(reinterpret_cast<char*>(&new_num_keys), sizeof(int));
        new_data.Append(parent_data->Data() + sizeof(int), i * kKeyEntrySize);
      } else {
        new_data.Append(parent_data->Data(), sizeof(int) + i * kKeyEntrySize);
      }
      new_data.Append(reinterpret_cast<char*>(key_), kKeySize);
      new_data.Append(reinterpret_cast<char*>(&data_type_id), sizeof(uint32_t));
      new_data.Append(reinterpret_cast<char*>(&new_key_data_size),
                      sizeof(size_t));
      if (i == num_keys) {
        new_data.AppendRange(
            parent_data->Data() + sizeof(int) + num_keys * kKeyEntrySize,
            parent_data->Data() + sizeof(int) + num_keys * kKeyEntrySize +
                data_offset);
      } else {
        new_data.AppendRange(
            parent_data->Data() + sizeof(int) + (i + 1) * kKeyEntrySize,
            parent_data->Data() + sizeof(int) + num_keys * kKeyEntrySize +
                data_offset);
      }
      new_data.Append(new_key_data, new_key_data_size);
      new_data.AppendRange(parent_data->Data() + sizeof(int) +
                               num_keys * kKeyEntrySize + data_offset +
                               data_len,
                           parent_data->Data() + parent_data->size());

      CHECK_EQ(new_data.size(), new_size);
    }

    LogParentHeader(new_data.data(), new_data.size());

    if (parent_data) {
      parent_->ClearCachedMetadata(kCacheLocally);
    }
    parent_->SetCachedMetadata(MAP_DATA_TYPE, new_data.data(), new_data.size(),
                               cache_type);
  }

  void ClearCachedMetadata(CacheType cache_type = kCacheLocally) override {
    VLOG(4) << "CacheMux::ClearCachedMetadata";

    scoped_refptr<CachedMetadata> parent_data = ParentData();
    if (!parent_data)
      return;

    CHECK_GE(parent_data->size(), sizeof(int));

    const char* data = parent_data->Data();
    int num_keys = *reinterpret_cast<const int*>(data);
    data += sizeof(int);
    CHECK_GT(num_keys, 0);
    CHECK_GE(parent_data->size(), sizeof(int) + num_keys * kKeyEntrySize);

    size_t data_offset = 0;
    size_t data_len = 0;

    int i;
    for (i = 0; i < num_keys; ++i) {
      const uint8_t* key = reinterpret_cast<const uint8_t*>(data);
      data += kKeySize;
      data += sizeof(uint32_t);
      size_t key_data_len = *reinterpret_cast<const size_t*>(data);
      data += sizeof(size_t);

      if (memcmp(key, key_, kKeySize) == 0) {
        data_len = key_data_len;
        break;
      }

      data_offset += key_data_len;
    }
    if (i == num_keys)
      return;

    if (num_keys == 1)
      return parent_->ClearCachedMetadata(cache_type);

    VLOG(4) << "CacheMux::ClearCachedMetadata modifying parent data";

    Vector<char> new_data;
    new_data.ReserveInitialCapacity(parent_data->size() - kKeyEntrySize -
                                    data_len);
    int new_num_keys = num_keys - 1;
    new_data.Append(reinterpret_cast<char*>(&new_num_keys), sizeof(int));
    new_data.AppendRange(parent_data->Data() + sizeof(int),
                         parent_data->Data() + sizeof(int) + i * kKeyEntrySize);
    new_data.AppendRange(
        parent_data->Data() + sizeof(int) + (i + 1) * kKeyEntrySize,
        parent_data->Data() + sizeof(int) + num_keys * kKeyEntrySize +
            data_offset);
    new_data.AppendRange(parent_data->Data() + sizeof(int) +
                             num_keys * kKeyEntrySize + data_offset + data_len,
                         parent_data->Data() + parent_data->size());

    CHECK_EQ(new_data.size(), parent_data->size() - kKeyEntrySize - data_len);

    LogParentHeader(parent_data->Data(), parent_data->size());
    LogParentHeader(new_data.data(), new_data.size());

    parent_->ClearCachedMetadata(kCacheLocally);
    parent_->SetCachedMetadata(MAP_DATA_TYPE, new_data.data(), new_data.size(),
                               cache_type);
  }

  scoped_refptr<CachedMetadata> GetCachedMetadata(
      uint32_t data_type_id) const override {
    VLOG(4) << "CacheMux::GetCachedMetadata " << KeyStr(key_) << " "
            << data_type_id;

    scoped_refptr<CachedMetadata> parent_data = ParentData();
    if (!parent_data)
      return nullptr;

    VLOG(4) << "CacheMux::GetCachedMetadata found parent_data";
    LogParentHeader(parent_data->Data(), parent_data->size());

    CHECK_GE(parent_data->size(), sizeof(int));

    const char* data = parent_data->Data();
    int num_keys = *reinterpret_cast<const int*>(data);
    data += sizeof(int);
    CHECK_GT(num_keys, 0);
    CHECK_GE(parent_data->size(), sizeof(int) + num_keys * kKeyEntrySize);

    VLOG(4) << "CacheMux::GetCachedMetadata parent has " << num_keys << " keys";

    size_t data_offset = 0;
    size_t data_len = 0;
    int i;
    for (i = 0; i < num_keys; ++i) {
      const uint8_t* key = reinterpret_cast<const uint8_t*>(data);
      data += kKeySize;
      uint32_t key_data_type = *reinterpret_cast<const uint32_t*>(data);
      data += sizeof(uint32_t);
      size_t key_data_len = *reinterpret_cast<const size_t*>(data);
      data += sizeof(size_t);

      VLOG(4) << "CacheMux::GetCachedMetadata parent has " << KeyStr(key)
              << " with data_type " << key_data_type;

      if (memcmp(key, key_, kKeySize) == 0) {
        if (key_data_type != data_type_id)
          return nullptr;
        data_len = key_data_len;
        break;
      }

      data_offset += key_data_len;
    }
    if (i == num_keys)
      return nullptr;

    VLOG(4) << "CacheMux::GetCachedMetadata found data";

    data = parent_data->Data() + sizeof(int) + num_keys * kKeyEntrySize +
           data_offset;
    CHECK_GE(parent_data->size(),
             sizeof(int) + num_keys * kKeyEntrySize + data_offset + data_len);

    return CachedMetadata::Create(data_type_id, data, data_len);
  }

  String Encoding() const override { return parent_->Encoding(); }

  bool IsServedFromCacheStorage() const override {
    return parent_->IsServedFromCacheStorage();
  }

  void LogParentHeader(const char* data, size_t size) const {
    int num_keys = *reinterpret_cast<const int*>(data);
    data += sizeof(int);
    CHECK_GT(num_keys, 0);
    CHECK_GE(size, sizeof(int) + num_keys * kKeyEntrySize);

    VLOG(4) << "CacheMux::LogParentHeader | num_keys: " << num_keys;

    int i;
    for (i = 0; i < num_keys; ++i) {
      const uint8_t* key = reinterpret_cast<const uint8_t*>(data);
      data += kKeySize;
      uint32_t key_data_type = *reinterpret_cast<const uint32_t*>(data);
      data += sizeof(uint32_t);
      size_t key_data_len = *reinterpret_cast<const size_t*>(data);
      data += sizeof(size_t);

      if (memcmp(key, key_, kKeySize) == 0) {
        VLOG(4) << "CacheMux::LogParentHeader | * " << KeyStr(key) << "["
                << key_data_type << "," << key_data_len << "]";
      } else {
        VLOG(4) << "CacheMux::LogParentHeader | - " << KeyStr(key) << "["
                << key_data_type << "," << key_data_len << "]";
      }
    }
  }

 private:
  Member<CachedMetadataHandler> parent_;
  uint8_t key_[kKeySize];

  scoped_refptr<CachedMetadata> ParentData() const {
    return parent_->GetCachedMetadata(MAP_DATA_TYPE);
  }
};

bool ClassicPendingScript::CheckMIMETypeBeforeRunScript(
    Document* context_document) const {
  if (!is_external_)
    return true;

  return AllowedByNosniff::MimeTypeAsScript(context_document,
                                            GetResource()->GetResponse());
}

static CachedMetadataHandler* GetInlineCacheHandler(const String& source,
                                                    Document& document) {
  CachedMetadataHandler* document_cache_handler =
      document.GetScriptableDocumentParser()->GetInlineScriptCacheHandler();

  if (!document_cache_handler)
    return nullptr;

  DigestValue digest_value;
  if (!ComputeDigest(kHashAlgorithmSha256,
                     static_cast<const char*>(source.Bytes()), source.length(),
                     digest_value))
    return nullptr;

  return new WrappingCacheMetadataHandler(document_cache_handler, digest_value);
}

ClassicScript* ClassicPendingScript::GetSource(const KURL& document_url,
                                               bool& error_occurred) const {
  CheckState();
  DCHECK(IsReady());

  error_occurred = ErrorOccurred();
  if (!is_external_) {
    CachedMetadataHandler* cache_handler = nullptr;
    String source = GetElement()->TextFromChildren();
    if (source_location_type_ == ScriptSourceLocationType::kInline) {
      cache_handler =
          GetInlineCacheHandler(source, GetElement()->GetDocument());
    }
    ScriptSourceCode source_code(source, source_location_type_, cache_handler,
                                 document_url, StartingPosition());
    return ClassicScript::Create(source_code, base_url_for_inline_script_,
                                 options_, kSharableCrossOrigin);
  }

  DCHECK(GetResource()->IsLoaded());
  ScriptResource* resource = ToScriptResource(GetResource());
  bool streamer_ready = (ready_state_ == kReady) && streamer_ &&
                        !streamer_->StreamingSuppressed();
  ScriptSourceCode source_code(streamer_ready ? streamer_ : nullptr, resource);
  // The base URL for external classic script is
  // "the URL from which the script was obtained" [spec text]
  // https://html.spec.whatwg.org/multipage/webappapis.html#concept-script-base-url
  const KURL& base_url = source_code.Url();
  return ClassicScript::Create(source_code, base_url, options_,
                               resource->CalculateAccessControlStatus());
}

void ClassicPendingScript::SetStreamer(ScriptStreamer* streamer) {
  DCHECK(streamer);
  DCHECK(!streamer_);
  DCHECK(!IsWatchingForLoad() || ready_state_ != kWaitingForResource);
  DCHECK(!streamer->IsFinished());
  DCHECK(ready_state_ == kWaitingForResource || ready_state_ == kReady);

  streamer_ = streamer;
  is_currently_streaming_ = true;
  if (streamer && ready_state_ == kReady)
    AdvanceReadyState(kReadyStreaming);

  CheckState();
}

bool ClassicPendingScript::IsReady() const {
  CheckState();
  return ready_state_ >= kReady;
}

bool ClassicPendingScript::ErrorOccurred() const {
  CheckState();
  return ready_state_ == kErrorOccurred;
}

void ClassicPendingScript::AdvanceReadyState(ReadyState new_ready_state) {
  // We will allow exactly these state transitions:
  //
  // kWaitingForResource -> kWaitingForStreaming -> [kReady, kErrorOccurred]
  // kReady -> kReadyStreaming -> kReady
  switch (ready_state_) {
    case kWaitingForResource:
      CHECK_EQ(new_ready_state, kWaitingForStreaming);
      break;
    case kWaitingForStreaming:
      CHECK(new_ready_state == kReady || new_ready_state == kErrorOccurred);
      break;
    case kReady:
      CHECK_EQ(new_ready_state, kReadyStreaming);
      break;
    case kReadyStreaming:
      CHECK_EQ(new_ready_state, kReady);
      break;
    case kErrorOccurred:
      NOTREACHED();
      break;
  }

  bool old_is_ready = IsReady();
  ready_state_ = new_ready_state;

  // Did we transition into a 'ready' state?
  if (IsReady() && !old_is_ready && IsWatchingForLoad())
    Client()->PendingScriptFinished(this);

  // Did we finish streaming?
  if (IsCurrentlyStreaming()) {
    if (ready_state_ == kReady || ready_state_ == kErrorOccurred) {
      // Call the streamer_done_ callback. Ensure that is_currently_streaming_
      // is reset only after the callback returns, to prevent accidentally
      // start streaming by work done within the callback. (crbug.com/754360)
      base::OnceClosure done = std::move(streamer_done_);
      if (done)
        std::move(done).Run();
      is_currently_streaming_ = false;
    }
  }

  // Streaming-related post conditions:

  // To help diagnose crbug.com/78426, we'll temporarily add some DCHECKs
  // that are a subset of the DCHECKs below:
  if (IsCurrentlyStreaming()) {
    DCHECK(streamer_);
    DCHECK(!streamer_->IsFinished());
  }

  // IsCurrentlyStreaming should match what streamer_ thinks.
  DCHECK_EQ(IsCurrentlyStreaming(), streamer_ && !streamer_->IsFinished());
  // IsCurrentlyStreaming should match the ready_state_.
  DCHECK_EQ(IsCurrentlyStreaming(),
            ready_state_ == kReadyStreaming ||
                (streamer_ && (ready_state_ == kWaitingForResource ||
                               ready_state_ == kWaitingForStreaming)));
  // We can only have a streamer_done_ callback if we are actually streaming.
  DCHECK(IsCurrentlyStreaming() || !streamer_done_);
}

void ClassicPendingScript::OnPurgeMemory() {
  CheckState();
  CancelStreaming();
}

bool ClassicPendingScript::StartStreamingIfPossible(
    ScriptStreamer::Type streamer_type,
    base::OnceClosure done) {
  if (IsCurrentlyStreaming())
    return false;

  // We can start streaming in two states: While still loading
  // (kWaitingForResource), or after having loaded (kReady).
  if (ready_state_ != kWaitingForResource && ready_state_ != kReady)
    return false;

  Document* document = &GetElement()->GetDocument();
  if (!document || !document->GetFrame())
    return false;

  ScriptState* script_state = ToScriptStateForMainWorld(document->GetFrame());
  if (!script_state)
    return false;

  // To support streaming re-try, we'll clear the existing streamer if
  // it exists; it claims to be finished; but it's finished because streaming
  // has been suppressed.
  if (streamer_ && streamer_->StreamingSuppressed() &&
      streamer_->IsFinished()) {
    DCHECK_EQ(ready_state_, kReady);
    DCHECK(!streamer_done_);
    DCHECK(!IsCurrentlyStreaming());
    streamer_.Clear();
  }

  if (streamer_)
    return false;

  // The two checks above should imply that we're not presently streaming.
  DCHECK(!IsCurrentlyStreaming());

  // Parser blocking scripts tend to do a lot of work in the 'finished'
  // callbacks, while async + in-order scripts all do control-like activities
  // (like posting new tasks). Use the 'control' queue only for control tasks.
  // (More details in discussion for cl 500147.)
  auto task_type = streamer_type == ScriptStreamer::kParsingBlocking
                       ? TaskType::kNetworking
                       : TaskType::kNetworkingControl;

  DCHECK(!streamer_);
  DCHECK(!IsCurrentlyStreaming());
  DCHECK(!streamer_done_);
  ScriptStreamer::StartStreaming(
      this, streamer_type, document->GetFrame()->GetSettings(), script_state,
      document->GetTaskRunner(task_type));
  bool success = streamer_ && !streamer_->IsStreamingFinished();

  // If we have successfully started streaming, we are required to call the
  // callback.
  DCHECK_EQ(success, IsCurrentlyStreaming());
  if (success)
    streamer_done_ = std::move(done);
  return success;
}

bool ClassicPendingScript::IsCurrentlyStreaming() const {
  return is_currently_streaming_;
}

bool ClassicPendingScript::WasCanceled() const {
  if (!is_external_)
    return false;
  return GetResource()->WasCanceled();
}

KURL ClassicPendingScript::UrlForTracing() const {
  if (!is_external_ || !GetResource())
    return NullURL();

  return GetResource()->Url();
}

}  // namespace blink
