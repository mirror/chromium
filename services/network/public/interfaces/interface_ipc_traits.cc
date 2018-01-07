// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/network_param_ipc_traits.h"

// Generation of IPC definitions.

// Generate constructors.
#undef SERVICES_NETWORK_PUBLIC_CPP_NETWORK_PARAM_IPC_TRAITS_H_
#include "ipc/struct_constructor_macros.h"
#include "network_param_ipc_traits.h"

// Generate destructors.
#undef SERVICES_NETWORK_PUBLIC_CPP_NETWORK_PARAM_IPC_TRAITS_H_
#include "ipc/struct_destructor_macros.h"
#include "network_param_ipc_traits.h"

// Generate param traits write methods.
#undef SERVICES_NETWORK_PUBLIC_CPP_NETWORK_PARAM_IPC_TRAITS_H_
#include "ipc/param_traits_write_macros.h"
namespace IPC {
#include "network_param_ipc_traits.h"
}  // namespace IPC

// Generate param traits read methods.
#undef SERVICES_NETWORK_PUBLIC_CPP_NETWORK_PARAM_IPC_TRAITS_H_
#include "ipc/param_traits_read_macros.h"
namespace IPC {
#include "network_param_ipc_traits.h"
}  // namespace IPC

// Generate param traits log methods.
#undef SERVICES_NETWORK_PUBLIC_CPP_NETWORK_PARAM_IPC_TRAITS_H_
#include "ipc/param_traits_log_macros.h"
namespace IPC {
#include "network_param_ipc_traits.h"
}  // namespace IPC
