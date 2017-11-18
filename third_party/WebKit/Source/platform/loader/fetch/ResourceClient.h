/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All
    rights reserved.

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

#ifndef ResourceClient_h
#define ResourceClient_h

#include "platform/PlatformExport.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/WebDataConsumerHandle.h"

namespace blink {
class Resource;
class ResourceTimingInfo;

class PLATFORM_EXPORT ResourceClient : public GarbageCollectedMixin {
 public:
  enum ResourceClientType {
    kBaseResourceType,
    kFontType,
    kStyleSheetType,
    kDocumentType,
    kRawResourceType,
    kScriptType
  };

  virtual ~ResourceClient() {}

  // The order of the callbacks is as follows:
  // [Case 1] A successful load:
  // 0+  redirectReceived() and/or dataSent()
  // 1   responseReceived()
  // 0-1 setSerializedCachedMetadata()
  // 0+  dataReceived() or dataDownloaded(), but never both
  // 1   notifyFinished() with errorOccurred() = false
  // [Case 2] When redirect is blocked:
  // 0+  redirectReceived() and/or dataSent()
  // 1   redirectBlocked()
  // 1   notifyFinished() with errorOccurred() = true
  // [Case 3] Other failures:
  //     notifyFinished() with errorOccurred() = true is called at any time
  //     (unless notifyFinished() is already called).
  // In all cases:
  //     No callbacks are made after notifyFinished() or
  //     removeClient() is called.
  virtual void DataSent(Resource*,
                        unsigned long long /* bytesSent */,
                        unsigned long long /* totalBytesToBeSent */) {}
  virtual void ResponseReceived(Resource*,
                                const ResourceResponse&,
                                std::unique_ptr<WebDataConsumerHandle>) {}
  virtual void SetSerializedCachedMetadata(Resource*, const char*, size_t) {}
  virtual void DataReceived(Resource*,
                            const char* /* data */,
                            size_t /* length */) {}
  virtual bool RedirectReceived(Resource*,
                                const ResourceRequest&,
                                const ResourceResponse&) {
    return true;
  }
  virtual void RedirectBlocked() {}
  virtual void DataDownloaded(Resource*, int) {}
  virtual void DidReceiveResourceTiming(Resource*, const ResourceTimingInfo&) {}
  virtual void NotifyFinished(Resource*) {}

  static bool IsExpectedType(ResourceClient*) { return true; }
  virtual ResourceClientType GetResourceClientType() const {
    return kBaseResourceType;
  }

  // Name for debugging, e.g. shown in memory-infra.
  virtual String DebugName() const = 0;

  void Trace(blink::Visitor* visitor) override {}

 protected:
  ResourceClient() {}
};

}  // namespace blink

#endif
