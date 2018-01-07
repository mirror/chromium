// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_INTERFACES_NETWORK_PARAM_IPC_TRAITS_H_
#define SERVICES_NETWORK_PUBLIC_INTERFACES_NETWORK_PARAM_IPC_TRAITS_H_

#include "ipc/ipc_param_traits.h"
#include "ipc/param_traits_macros.h"

IPC_ENUM_TRAITS_MAX_VALUE(net::NetworkChangeNotifier::ConnectionType,
                          net::NetworkChangeNotifier::CONNECTION_LAST)
IPC_ENUM_TRAITS_MAX_VALUE(net::EffectiveConnectionType,
                          net::EFFECTIVE_CONNECTION_TYPE_LAST - 1)

#endif  // SERVICES_NETWORK_PUBLIC_INTERFACES_NETWORK_PARAM_IPC_TRAITS_H_
