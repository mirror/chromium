// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_flash_tcp_socket.h"
#include "ppapi/thunk/thunk.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_flash_tcp_socket_api.h"
#include "ppapi/thunk/resource_creation_api.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Resource Create(PP_Instance instance) {
  EnterFunction<ResourceCreationAPI> enter(instance, true);
  if (enter.failed())
    return 0;
  return enter.functions()->CreateFlashTCPSocket(instance);
}

PP_Bool IsFlashTCPSocket(PP_Resource resource) {
  EnterResource<PPB_Flash_TCPSocket_API> enter(resource, false);
  return PP_FromBool(enter.succeeded());
}

int32_t Connect(PP_Resource tcp_socket,
                const char* host,
                uint16_t port,
                PP_CompletionCallback callback) {
  EnterResource<PPB_Flash_TCPSocket_API> enter(tcp_socket, true);
  if (enter.failed())
    return PP_ERROR_BADRESOURCE;
  return enter.object()->Connect(host, port, callback);
}

int32_t ConnectWithNetAddress(PP_Resource tcp_socket,
                              const PP_Flash_NetAddress* addr,
                              PP_CompletionCallback callback) {
  EnterResource<PPB_Flash_TCPSocket_API> enter(tcp_socket, true);
  if (enter.failed())
    return PP_ERROR_BADRESOURCE;
  return enter.object()->ConnectWithNetAddress(addr, callback);
}

PP_Bool GetLocalAddress(PP_Resource tcp_socket,
                        PP_Flash_NetAddress* local_addr) {
  EnterResource<PPB_Flash_TCPSocket_API> enter(tcp_socket, true);
  if (enter.failed())
    return PP_FALSE;
  return enter.object()->GetLocalAddress(local_addr);
}

PP_Bool GetRemoteAddress(PP_Resource tcp_socket,
                         PP_Flash_NetAddress* remote_addr) {
  EnterResource<PPB_Flash_TCPSocket_API> enter(tcp_socket, true);
  if (enter.failed())
    return PP_FALSE;
  return enter.object()->GetRemoteAddress(remote_addr);
}

int32_t InitiateSSL(PP_Resource tcp_socket,
                    const char* server_name,
                    PP_CompletionCallback callback) {
  EnterResource<PPB_Flash_TCPSocket_API> enter(tcp_socket, true);
  if (enter.failed())
    return PP_ERROR_BADRESOURCE;
  return enter.object()->InitiateSSL(server_name, callback);
}

int32_t Read(PP_Resource tcp_socket,
             char* buffer,
             int32_t bytes_to_read,
             PP_CompletionCallback callback) {
  EnterResource<PPB_Flash_TCPSocket_API> enter(tcp_socket, true);
  if (enter.failed())
    return PP_ERROR_BADRESOURCE;
  return enter.object()->Read(buffer, bytes_to_read, callback);
}

int32_t Write(PP_Resource tcp_socket,
              const char* buffer,
              int32_t bytes_to_write,
              PP_CompletionCallback callback) {
  EnterResource<PPB_Flash_TCPSocket_API> enter(tcp_socket, true);
  if (enter.failed())
    return PP_ERROR_BADRESOURCE;
  return enter.object()->Write(buffer, bytes_to_write, callback);
}

void Disconnect(PP_Resource tcp_socket) {
  EnterResource<PPB_Flash_TCPSocket_API> enter(tcp_socket, true);
  if (enter.succeeded())
    enter.object()->Disconnect();
}

const PPB_Flash_TCPSocket g_ppb_flash_tcp_socket_thunk = {
  &Create,
  &IsFlashTCPSocket,
  &Connect,
  &ConnectWithNetAddress,
  &GetLocalAddress,
  &GetRemoteAddress,
  &InitiateSSL,
  &Read,
  &Write,
  &Disconnect
};

}  // namespace

const PPB_Flash_TCPSocket* GetPPB_Flash_TCPSocket_Thunk() {
  return &g_ppb_flash_tcp_socket_thunk;
}

}  // namespace thunk
}  // namespace ppapi

