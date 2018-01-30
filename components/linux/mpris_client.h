// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LINUX_MPRIS_CLIENT_H_
#define COMPONENTS_LINUX_MPRIS_CLIENT_H_

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"

namespace dbus {
class Bus;
class ExportedObject;
class ObjectManager;
}

// A D-Bus client conforming to the MPRIS spec:
// https://specifications.freedesktop.org/mpris-spec/latest/
// It is currently under development (https://crbug.com/804121)
class MprisClient {
 public:
  MprisClient();
  ~MprisClient();

 private:
  void OnOwnership(const std::string& service_name, bool success);

  scoped_refptr<dbus::Bus> bus_;

  scoped_refptr<dbus::ExportedObject> exported_object_;

  //dbus::ObjectManager* mp2_manager_;
  //dbus::ObjectManager* mp2_player_manager_;

  base::WeakPtrFactory<MprisClient> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MprisClient);
};

#endif  // COMPONENTS_LINUX_MPRIS_CLIENT_H_
