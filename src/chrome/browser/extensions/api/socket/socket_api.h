// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_SOCKET_SOCKET_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_SOCKET_SOCKET_API_H_
#pragma once

#include "base/memory/ref_counted.h"
#include "chrome/browser/extensions/api/api_function.h"
#include "chrome/common/extensions/api/experimental_socket.h"
#include "net/base/address_list.h"
#include "net/base/host_resolver.h"

#include <string>

namespace net {
class IOBuffer;
}

class IOThread;

namespace extensions {

class APIResourceController;
class APIResourceEventNotifier;

class SocketExtensionFunction : public AsyncAPIFunction {
 protected:
  virtual ~SocketExtensionFunction() {}

  // AsyncAPIFunction:
  virtual void Work() OVERRIDE;
  virtual bool Respond() OVERRIDE;
};

class SocketExtensionWithDnsLookupFunction : public SocketExtensionFunction {
 protected:
  SocketExtensionWithDnsLookupFunction();
  virtual ~SocketExtensionWithDnsLookupFunction();

  void StartDnsLookup(const std::string& hostname);
  virtual void AfterDnsLookup(int lookup_result) = 0;

  std::string resolved_address_;

 private:
  void OnDnsLookup(int resolve_result);

  // This instance is widely available through BrowserProcess, but we need to
  // acquire it on the UI thread and then use it on the IO thread, so we keep a
  // plain pointer to it here as we move from thread to thread.
  IOThread* io_thread_;

  scoped_ptr<net::HostResolver::RequestHandle> request_handle_;
  scoped_ptr<net::AddressList> addresses_;
};

class SocketCreateFunction : public SocketExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.socket.create")

  SocketCreateFunction();

 protected:
  virtual ~SocketCreateFunction();

  // AsyncAPIFunction:
  virtual bool Prepare() OVERRIDE;
  virtual void Work() OVERRIDE;

 private:
  enum SocketType {
    kSocketTypeInvalid = -1,
    kSocketTypeTCP,
    kSocketTypeUDP
  };

  SocketType socket_type_;
  int src_id_;
  APIResourceEventNotifier* event_notifier_;
};

class SocketDestroyFunction : public SocketExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.socket.destroy")

 protected:
  virtual ~SocketDestroyFunction() {}

  // AsyncAPIFunction:
  virtual bool Prepare() OVERRIDE;
  virtual void Work() OVERRIDE;

 private:
  int socket_id_;
};

class SocketConnectFunction : public SocketExtensionWithDnsLookupFunction {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.socket.connect")

  SocketConnectFunction();

 protected:
  virtual ~SocketConnectFunction();

  // AsyncAPIFunction:
  virtual bool Prepare() OVERRIDE;
  virtual void AsyncWorkStart() OVERRIDE;

  // SocketExtensionWithDnsLookupFunction:
  virtual void AfterDnsLookup(int lookup_result) OVERRIDE;

 private:
  void StartConnect();
  void OnConnect(int result);

  int socket_id_;
  std::string hostname_;
  int port_;
};

class SocketDisconnectFunction : public SocketExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.socket.disconnect")

 protected:
  virtual ~SocketDisconnectFunction() {}

  // AsyncAPIFunction:
  virtual bool Prepare() OVERRIDE;
  virtual void Work() OVERRIDE;

 private:
  int socket_id_;
};

class SocketBindFunction : public SocketExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.socket.bind")

 protected:
  virtual ~SocketBindFunction() {}

  // AsyncAPIFunction:
  virtual bool Prepare() OVERRIDE;
  virtual void Work() OVERRIDE;

 private:
  int socket_id_;
  std::string address_;
  int port_;
};

class SocketReadFunction : public SocketExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.socket.read")

  SocketReadFunction();

 protected:
  virtual ~SocketReadFunction();

  // AsyncAPIFunction:
  virtual bool Prepare() OVERRIDE;
  virtual void AsyncWorkStart() OVERRIDE;
  void OnCompleted(int result, scoped_refptr<net::IOBuffer> io_buffer);

 private:
  scoped_ptr<api::experimental_socket::Read::Params> params_;
};

class SocketWriteFunction : public SocketExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.socket.write")

  SocketWriteFunction();

 protected:
  virtual ~SocketWriteFunction();

  // AsyncAPIFunction:
  virtual bool Prepare() OVERRIDE;
  virtual void AsyncWorkStart() OVERRIDE;
  void OnCompleted(int result);

 private:
  int socket_id_;
  scoped_refptr<net::IOBuffer> io_buffer_;
  size_t io_buffer_size_;
};

class SocketRecvFromFunction : public SocketExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.socket.recvFrom")

  SocketRecvFromFunction();

 protected:
  virtual ~SocketRecvFromFunction();

  // AsyncAPIFunction
  virtual bool Prepare() OVERRIDE;
  virtual void AsyncWorkStart() OVERRIDE;
  void OnCompleted(int result,
                   scoped_refptr<net::IOBuffer> io_buffer,
                   const std::string& address,
                   int port);

 private:
  scoped_ptr<api::experimental_socket::RecvFrom::Params> params_;
};

class SocketSendToFunction : public SocketExtensionWithDnsLookupFunction {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.socket.sendTo")

  SocketSendToFunction();

 protected:
  virtual ~SocketSendToFunction();

  // AsyncAPIFunction:
  virtual bool Prepare() OVERRIDE;
  virtual void AsyncWorkStart() OVERRIDE;
  void OnCompleted(int result);

  // SocketExtensionWithDnsLookupFunction:
  virtual void AfterDnsLookup(int lookup_result) OVERRIDE;

 private:
  void StartSendTo();

  int socket_id_;
  scoped_refptr<net::IOBuffer> io_buffer_;
  size_t io_buffer_size_;
  std::string hostname_;
  int port_;
};

class SocketSetKeepAliveFunction : public SocketExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.socket.setKeepAlive")

  SocketSetKeepAliveFunction();

 protected:
  virtual ~SocketSetKeepAliveFunction();

  // AsyncAPIFunction:
  virtual bool Prepare() OVERRIDE;
  virtual void Work() OVERRIDE;

 private:
  scoped_ptr<api::experimental_socket::SetKeepAlive::Params> params_;
};

class SocketSetNoDelayFunction : public SocketExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.socket.setNoDelay")

  SocketSetNoDelayFunction();

 protected:
  virtual ~SocketSetNoDelayFunction();

  // AsyncAPIFunction:
  virtual bool Prepare() OVERRIDE;
  virtual void Work() OVERRIDE;

 private:
  scoped_ptr<api::experimental_socket::SetNoDelay::Params> params_;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_SOCKET_SOCKET_API_H_
