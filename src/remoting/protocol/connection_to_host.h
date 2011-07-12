// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_CONNECTION_TO_HOST_H_
#define REMOTING_PROTOCOL_CONNECTION_TO_HOST_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/task.h"
#include "remoting/jingle_glue/signal_strategy.h"
#include "remoting/proto/internal.pb.h"
#include "remoting/protocol/connection_to_host.h"
#include "remoting/protocol/message_reader.h"
#include "remoting/protocol/session.h"
#include "remoting/protocol/session_manager.h"

class MessageLoop;

namespace talk_base {
class NetworkManager;
class PacketSocketFactory;
}  // namespace talk_base

namespace remoting {

class JingleThread;
class PortAllocatorSessionFactory;
class XmppProxy;
class VideoPacket;

namespace protocol {

class ClientMessageDispatcher;
class ClientControlSender;
class ClientStub;
class HostControlSender;
class HostStub;
class InputSender;
class InputStub;
class SessionConfig;
class VideoReader;
class VideoStub;

class ConnectionToHost : public SignalStrategy::StatusObserver {
 public:
  enum State {
    STATE_EMPTY,
    STATE_CONNECTED,
    STATE_AUTHENTICATED,
    STATE_FAILED,
    STATE_CLOSED,
  };

  class HostEventCallback {
   public:
    virtual ~HostEventCallback() {}

    // Called when the network connection is opened.
    virtual void OnConnectionOpened(ConnectionToHost* conn) = 0;

    // Called when the network connection is closed.
    virtual void OnConnectionClosed(ConnectionToHost* conn) = 0;

    // Called when the network connection has failed.
    virtual void OnConnectionFailed(ConnectionToHost* conn) = 0;
  };

  // Takes ownership of |network_manager| and |socket_factory|. Both
  // |network_manager| and |socket_factory| may be set to NULL.
  //
  // TODO(sergeyu): Constructor shouldn't need thread here.
  ConnectionToHost(MessageLoop* network_message_loop,
                   talk_base::NetworkManager* network_manager,
                   talk_base::PacketSocketFactory* socket_factory,
                   PortAllocatorSessionFactory* session_factory);
  virtual ~ConnectionToHost();

  virtual void Connect(scoped_refptr<XmppProxy> xmpp_proxy,
                       const std::string& your_jid,
                       const std::string& host_jid,
                       const std::string& host_public_key,
                       const std::string& access_code,
                       HostEventCallback* event_callback,
                       ClientStub* client_stub,
                       VideoStub* video_stub);

  virtual void Disconnect(const base::Closure& shutdown_task);

  virtual const SessionConfig* config();

  virtual InputStub* input_stub();

  virtual HostStub* host_stub();

  // SignalStrategy::StatusObserver interface.
  virtual void OnStateChange(
      SignalStrategy::StatusObserver::State state) OVERRIDE;
  virtual void OnJidChange(const std::string& full_jid) OVERRIDE;

  // Callback for chromotocol SessionManager.
  void OnNewSession(
      Session* connection,
      SessionManager::IncomingSessionResponse* response);

  // Callback for chromotocol Session.
  void OnSessionStateChange(Session::State state);

  // Called when the host accepts the client authentication.
  void OnClientAuthenticated();

  // Return the current state of ConnectionToHost.
  State state() const;

 private:
  // Called on the jingle thread after we've successfully to XMPP server. Starts
  // P2P connection to the host.
  void InitSession();

  // Callback for |video_reader_|.
  void OnVideoPacket(VideoPacket* packet);

  // Stops writing in the channels.
  void CloseChannels();

  // Internal state of the connection.
  State state_;

  MessageLoop* message_loop_;

  scoped_ptr<talk_base::NetworkManager> network_manager_;
  scoped_ptr<talk_base::PacketSocketFactory> socket_factory_;
  scoped_ptr<PortAllocatorSessionFactory> port_allocator_session_factory_;

  scoped_ptr<SignalStrategy> signal_strategy_;
  std::string local_jid_;
  scoped_ptr<SessionManager> session_manager_;
  scoped_ptr<Session> session_;

  scoped_ptr<VideoReader> video_reader_;

  HostEventCallback* event_callback_;

  std::string host_jid_;
  std::string host_public_key_;
  std::string access_code_;

  scoped_ptr<ClientMessageDispatcher> dispatcher_;

  ////////////////////////////////////////////////////////////////////////////
  // User input event channel interface

  // Stub for sending input event messages to the host.
  scoped_ptr<InputSender> input_sender_;

  ////////////////////////////////////////////////////////////////////////////
  // Protocol control channel interface

  // Stub for sending control messages to the host.
  scoped_ptr<HostControlSender> host_control_sender_;

  // Stub for receiving control messages from the host.
  ClientStub* client_stub_;

  ////////////////////////////////////////////////////////////////////////////
  // Video channel interface

  // Stub for receiving video packets from the host.
  VideoStub* video_stub_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ConnectionToHost);
};

}  // namespace protocol
}  // namespace remoting

DISABLE_RUNNABLE_METHOD_REFCOUNT(remoting::protocol::ConnectionToHost);

#endif  // REMOTING_PROTOCOL_CONNECTION_TO_HOST_H_
