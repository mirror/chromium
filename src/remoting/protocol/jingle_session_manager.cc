// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/jingle_session_manager.h"

#include <limits>

#include "base/message_loop.h"
#include "base/string_util.h"
#include "base/task.h"
#include "remoting/base/constants.h"
#include "remoting/jingle_glue/http_port_allocator.h"
#include "remoting/jingle_glue/jingle_info_request.h"
#include "remoting/jingle_glue/jingle_signaling_connector.h"
#include "remoting/jingle_glue/signal_strategy.h"
#include "third_party/libjingle/source/talk/base/basicpacketsocketfactory.h"
#include "third_party/libjingle/source/talk/p2p/base/sessionmanager.h"
#include "third_party/libjingle/source/talk/p2p/base/transport.h"
#include "third_party/libjingle/source/talk/p2p/base/constants.h"
#include "third_party/libjingle/source/talk/p2p/base/transport.h"
#include "third_party/libjingle/source/talk/xmllite/xmlelement.h"

using buzz::XmlElement;

namespace remoting {
namespace protocol {

// static
JingleSessionManager* JingleSessionManager::CreateNotSandboxed() {
  return new JingleSessionManager(NULL, NULL, NULL);
}

// static
JingleSessionManager* JingleSessionManager::CreateSandboxed(
    talk_base::NetworkManager* network_manager,
    talk_base::PacketSocketFactory* socket_factory,
    PortAllocatorSessionFactory* port_allocator_session_factory) {
  return new JingleSessionManager(network_manager, socket_factory,
                                  port_allocator_session_factory);
}

JingleSessionManager::JingleSessionManager(
    talk_base::NetworkManager* network_manager,
    talk_base::PacketSocketFactory* socket_factory,
    PortAllocatorSessionFactory* port_allocator_session_factory)
    : network_manager_(network_manager),
      socket_factory_(socket_factory),
      port_allocator_session_factory_(port_allocator_session_factory),
      signal_strategy_(NULL),
      enable_nat_traversing_(false),
      allow_local_ips_(false),
      http_port_allocator_(NULL),
      closed_(false),
      ALLOW_THIS_IN_INITIALIZER_LIST(task_factory_(this)) {
}

JingleSessionManager::~JingleSessionManager() {
  Close();
}

void JingleSessionManager::Init(
    const std::string& local_jid,
    SignalStrategy* signal_strategy,
    IncomingSessionCallback* incoming_session_callback,
    crypto::RSAPrivateKey* private_key,
    scoped_refptr<net::X509Certificate> certificate) {
  DCHECK(CalledOnValidThread());

  DCHECK(signal_strategy);
  DCHECK(incoming_session_callback);

  local_jid_ = local_jid;
  certificate_ = certificate;
  private_key_.reset(private_key);
  incoming_session_callback_.reset(incoming_session_callback);
  signal_strategy_ = signal_strategy;

  if (!network_manager_.get()) {
    VLOG(1) << "Creating talk_base::NetworkManager.";
    network_manager_.reset(new talk_base::NetworkManager());
  }
  if (!socket_factory_.get()) {
    VLOG(1) << "Creating talk_base::BasicPacketSocketFactory.";
    socket_factory_.reset(new talk_base::BasicPacketSocketFactory(
        talk_base::Thread::Current()));
  }

  if (enable_nat_traversing_) {
    http_port_allocator_ = new remoting::HttpPortAllocator(
        network_manager_.get(), socket_factory_.get(),
        port_allocator_session_factory_.get(), "transp2");
    port_allocator_.reset(http_port_allocator_);

    jingle_info_request_.reset(
        new JingleInfoRequest(signal_strategy_->CreateIqRequest()));
    jingle_info_request_->SetCallback(
        NewCallback(this, &JingleSessionManager::OnJingleInfo));
    jingle_info_request_->Run(
        task_factory_.NewRunnableMethod(
            &JingleSessionManager::DoStartSessionManager));
  } else {
    port_allocator_.reset(
        new cricket::BasicPortAllocator(
            network_manager_.get(), socket_factory_.get()));
    port_allocator_->set_flags(cricket::PORTALLOCATOR_DISABLE_STUN |
                               cricket::PORTALLOCATOR_DISABLE_RELAY);
    DoStartSessionManager();
  }
}

void JingleSessionManager::Close() {
  DCHECK(CalledOnValidThread());

  // Close() can be called only after all sessions are destroyed.
  DCHECK(sessions_.empty());

  if (!closed_) {
    cricket_session_manager_->RemoveClient(kChromotingXmlNamespace);
    jingle_signaling_connector_.reset();
    cricket_session_manager_.reset();
    closed_ = true;
  }
}

void JingleSessionManager::set_allow_local_ips(bool allow_local_ips) {
  allow_local_ips_ = allow_local_ips;
}

Session* JingleSessionManager::Connect(
    const std::string& host_jid,
    const std::string& host_public_key,
    const std::string& receiver_token,
    CandidateSessionConfig* candidate_config,
    Session::StateChangeCallback* state_change_callback) {
  DCHECK(CalledOnValidThread());

  // Can be called from any thread.
  JingleSession* jingle_session =
      JingleSession::CreateClientSession(this, host_public_key);
  jingle_session->set_candidate_config(candidate_config);
  jingle_session->set_receiver_token(receiver_token);

  cricket::Session* cricket_session = cricket_session_manager_->CreateSession(
      local_jid_, kChromotingXmlNamespace);

  // Initialize connection object before we send initiate stanza.
  jingle_session->SetStateChangeCallback(state_change_callback);
  jingle_session->Init(cricket_session);
  sessions_.push_back(jingle_session);

  cricket_session->Initiate(host_jid, CreateClientSessionDescription(
      jingle_session->candidate_config()->Clone(), receiver_token,
      jingle_session->GetEncryptedMasterKey()));

  return jingle_session;
}

void JingleSessionManager::OnSessionCreate(
    cricket::Session* cricket_session, bool incoming) {
  DCHECK(CalledOnValidThread());

  // Allow local connections if neccessary.
  cricket_session->set_allow_local_ips(allow_local_ips_);

  // If this is an outcoming session the session object is already created.
  if (incoming) {
    DCHECK(certificate_);
    DCHECK(private_key_.get());

    JingleSession* jingle_session = JingleSession::CreateServerSession(
        this, certificate_, private_key_.get());
    sessions_.push_back(jingle_session);
    jingle_session->Init(cricket_session);
  }
}

void JingleSessionManager::OnSessionDestroy(cricket::Session* cricket_session) {
  DCHECK(CalledOnValidThread());

  std::list<JingleSession*>::iterator it;
  for (it = sessions_.begin(); it != sessions_.end(); ++it) {
    if ((*it)->HasSession(cricket_session)) {
      (*it)->ReleaseSession();
      return;
    }
  }
}

bool JingleSessionManager::AcceptConnection(
    JingleSession* jingle_session,
    cricket::Session* cricket_session) {
  DCHECK(CalledOnValidThread());

  // Reject connection if we are closed.
  if (closed_) {
    cricket_session->Reject(cricket::STR_TERMINATE_DECLINE);
    return false;
  }

  const cricket::SessionDescription* session_description =
      cricket_session->remote_description();
  const cricket::ContentInfo* content =
      session_description->FirstContentByType(kChromotingXmlNamespace);

  CHECK(content);

  const ContentDescription* content_description =
      static_cast<const ContentDescription*>(content->description);
  jingle_session->set_candidate_config(content_description->config()->Clone());
  jingle_session->set_initiator_token(content_description->auth_token());

  // Always reject connection if there is no callback.
  IncomingSessionResponse response = protocol::SessionManager::DECLINE;

  // Use the callback to generate a response.
  if (incoming_session_callback_.get())
    incoming_session_callback_->Run(jingle_session, &response);

  switch (response) {
    case protocol::SessionManager::ACCEPT: {
      // Connection must be configured by the callback.
      DCHECK(jingle_session->config());
      CandidateSessionConfig* candidate_config =
          CandidateSessionConfig::CreateFrom(jingle_session->config());
      cricket_session->Accept(
          CreateHostSessionDescription(candidate_config,
                                       jingle_session->local_certificate()));
      break;
    }

    case protocol::SessionManager::INCOMPATIBLE: {
      cricket_session->Reject(cricket::STR_TERMINATE_INCOMPATIBLE_PARAMETERS);
      return false;
    }

    case protocol::SessionManager::DECLINE: {
      cricket_session->Reject(cricket::STR_TERMINATE_DECLINE);
      return false;
    }

    default: {
      NOTREACHED();
    }
  }

  return true;
}

void JingleSessionManager::SessionDestroyed(JingleSession* jingle_session) {
  std::list<JingleSession*>::iterator it =
      std::find(sessions_.begin(), sessions_.end(), jingle_session);
  CHECK(it != sessions_.end());
  cricket::Session* cricket_session = jingle_session->ReleaseSession();
  cricket_session_manager_->DestroySession(cricket_session);
  sessions_.erase(it);
}

void JingleSessionManager::OnJingleInfo(
    const std::string& token,
    const std::vector<std::string>& relay_hosts,
    const std::vector<talk_base::SocketAddress>& stun_hosts) {
  DCHECK(CalledOnValidThread());

  if (http_port_allocator_) {
    // TODO(ajwong): Avoid string processing if log-level is low.
    std::string stun_servers;
    for (size_t i = 0; i < stun_hosts.size(); ++i) {
      stun_servers += stun_hosts[i].ToString() + "; ";
    }
    LOG(INFO) << "Configuring with relay token: " << token
              << ", relays: " << JoinString(relay_hosts, ';')
              << ", stun: " << stun_servers;
    http_port_allocator_->SetRelayToken(token);
    http_port_allocator_->SetStunHosts(stun_hosts);
    http_port_allocator_->SetRelayHosts(relay_hosts);
  } else {
    LOG(INFO) << "Jingle info found but no port allocator.";
  }
}

void JingleSessionManager::DoStartSessionManager() {
  DCHECK(CalledOnValidThread());

  cricket_session_manager_.reset(
      new cricket::SessionManager(port_allocator_.get()));
  cricket_session_manager_->AddClient(kChromotingXmlNamespace, this);

  jingle_signaling_connector_.reset(new JingleSignalingConnector(
      signal_strategy_, cricket_session_manager_.get()));
}

// Parse content description generated by WriteContent().
bool JingleSessionManager::ParseContent(
    cricket::SignalingProtocol protocol,
    const XmlElement* element,
    const cricket::ContentDescription** content,
    cricket::ParseError* error) {
  *content = ContentDescription::ParseXml(element);
  return *content != NULL;
}

bool JingleSessionManager::WriteContent(
    cricket::SignalingProtocol protocol,
    const cricket::ContentDescription* content,
    XmlElement** elem,
    cricket::WriteError* error) {
  const ContentDescription* desc =
      static_cast<const ContentDescription*>(content);

  *elem = desc->ToXml();
  return true;
}

// static
cricket::SessionDescription*
JingleSessionManager::CreateClientSessionDescription(
    const CandidateSessionConfig* config,
    const std::string& auth_token,
    const std::string& master_key) {
  cricket::SessionDescription* desc = new cricket::SessionDescription();
  desc->AddContent(
      JingleSession::kChromotingContentName, kChromotingXmlNamespace,
      new ContentDescription(config, auth_token, master_key, NULL));
  return desc;
}

// static
cricket::SessionDescription* JingleSessionManager::CreateHostSessionDescription(
    const CandidateSessionConfig* config,
    scoped_refptr<net::X509Certificate> certificate) {
  cricket::SessionDescription* desc = new cricket::SessionDescription();
  desc->AddContent(
      JingleSession::kChromotingContentName, kChromotingXmlNamespace,
      new ContentDescription(config, "", "", certificate));
  return desc;
}

}  // namespace protocol
}  // namespace remoting
