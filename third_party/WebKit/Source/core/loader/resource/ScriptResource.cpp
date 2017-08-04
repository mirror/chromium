/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    This class provides all functionality needed for loading images, style
    sheets and html pages from the web. It has a memory cache for these objects.
*/

#include "core/loader/resource/ScriptResource.h"

#include "core/dom/Document.h"
#include "core/loader/SubresourceIntegrityHelper.h"
#include "platform/SharedBuffer.h"
#include "platform/instrumentation/tracing/web_memory_allocator_dump.h"
#include "platform/instrumentation/tracing/web_process_memory_dump.h"
#include "platform/loader/SubresourceIntegrity.h"
#include "platform/loader/fetch/FetchParameters.h"
#include "platform/loader/fetch/IntegrityMetadata.h"
#include "platform/loader/fetch/ResourceClientWalker.h"
#include "platform/loader/fetch/ResourceFetcher.h"

namespace blink {

ScriptResource* ScriptResource::Fetch(FetchParameters& params,
                                      ResourceFetcher* fetcher) {
  DCHECK_EQ(params.GetResourceRequest().GetFrameType(),
            WebURLRequest::kFrameTypeNone);
  params.SetRequestContext(WebURLRequest::kRequestContextScript);
  ScriptResource* resource = ToScriptResource(
      fetcher->RequestResource(params, ScriptResourceFactory()));
  if (resource && !params.IntegrityMetadata().IsEmpty())
    resource->SetIntegrityMetadata(params.IntegrityMetadata());
  return resource;
}

ScriptResource::ScriptResource(
    const ResourceRequest& resource_request,
    const ResourceLoaderOptions& options,
    const TextResourceDecoderOptions& decoder_options)
    : TextResource(resource_request, kScript, options, decoder_options) {}

ScriptResource::~ScriptResource() {}

void ScriptResource::DidAddClient(ResourceClient* client) {
  DCHECK(ScriptResourceClient::IsExpectedType(client));
  Resource::DidAddClient(client);
}

void ScriptResource::AppendData(const char* data, size_t length) {
  Resource::AppendData(data, length);
  ResourceClientWalker<ScriptResourceClient> walker(Clients());
  while (ScriptResourceClient* client = walker.Next())
    client->NotifyAppendData(this);
}

void ScriptResource::OnMemoryDump(WebMemoryDumpLevelOfDetail level_of_detail,
                                  WebProcessMemoryDump* memory_dump) const {
  Resource::OnMemoryDump(level_of_detail, memory_dump);
  const String name = GetMemoryDumpName() + "/decoded_script";
  auto dump = memory_dump->CreateMemoryAllocatorDump(name);
  dump->AddScalar("size", "bytes", DecodedSize());
  memory_dump->AddSuballocation(
      dump->Guid(), String(WTF::Partitions::kAllocatedObjectPoolName));
}

const ScriptResourceData* ScriptResource::ResourceData() {
  DCHECK(IsLoaded());
  if (script_data_)
    return script_data_;

  AtomicString atomic_source_text;
  if (Data()) {
    String source_text = DecodedText();
    ClearData();
    SetDecodedSize(source_text.CharactersSizeInBytes());
    atomic_source_text = AtomicString(source_text);
  }

  script_data_ = new ScriptResourceData(
      Url(), ResourceRequest().GetFetchCredentialsMode(), GetResponse(),
      ErrorOccurred(), atomic_source_text, CacheHandler(), GetCORSStatus());

  return script_data_;
}

void ScriptResource::DestroyDecodedDataForFailedRevalidation() {
  script_data_ = nullptr;
}

DEFINE_TRACE(ScriptResource) {
  visitor->Trace(script_data_);
  TextResource::Trace(visitor);
}

}  // namespace blink
