// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_CDM_SERVICE_CONTEXT_H_
#define MEDIA_MOJO_SERVICES_MOJO_CDM_SERVICE_CONTEXT_H_

#include <stdint.h>

#include <map>
#include <memory>

#include "base/macros.h"
#include "media/mojo/services/media_mojo_export.h"

namespace media {

class CdmProxy;
class CdmContextRef;
class MojoCdmService;
class MojoCdmProxyService;

// A class that creates, owns and manages all MojoCdmService instances.
// TODO(xhwang): Update this class to manage generic CdmContext objects instead
// of concrete MojoCdmService or MojoCdmProxyService objects. This would require
// having CdmContext holding reference to the CDM or CdmProxy it is associated
// with.
class MEDIA_MOJO_EXPORT MojoCdmServiceContext {
 public:
  MojoCdmServiceContext();
  ~MojoCdmServiceContext();

  // Registers the |cdm_service| and returns a unique (per-process) CDM ID.
  int RegisterCdm(MojoCdmService* cdm_service);
  int RegisterCdmProxy(MojoCdmProxyService* cdm_proxy_service);

  // Unregisters the CDM. Must be called before the CDM is destroyed.
  void UnregisterCdm(int cdm_id);
  void UnregisterCdmProxy(int cdm_id);

  // Returns the CdmContextRef associated with |cdm_id|.
  std::unique_ptr<CdmContextRef> GetCdmContextRef(int cdm_id);

 private:
  // CDM ID to be assigned to the next successfully initialized CDM. This ID is
  // unique per process. It will be used to locate the CDM by the media players
  // living in the same process.
  static int next_cdm_id_;

  // A map between CDM ID and MojoCdmService.
  std::map<int, MojoCdmService*> cdm_services_;

  // A map between CDM ID and MojoCdmProxyService.
  std::map<int, MojoCdmProxyService*> cdm_proxy_services_;

  DISALLOW_COPY_AND_ASSIGN(MojoCdmServiceContext);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_CDM_SERVICE_CONTEXT_H_
