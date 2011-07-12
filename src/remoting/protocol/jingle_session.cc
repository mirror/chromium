// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/jingle_session.h"

#include "base/message_loop.h"
#include "base/rand_util.h"
#include "crypto/hmac.h"
#include "crypto/rsa_private_key.h"
#include "jingle/glue/channel_socket_adapter.h"
#include "jingle/glue/pseudotcp_adapter.h"
#include "net/base/cert_status_flags.h"
#include "net/base/cert_verifier.h"
#include "net/base/host_port_pair.h"
#include "net/base/net_errors.h"
#include "net/base/ssl_config_service.h"
#include "net/base/x509_certificate.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/ssl_client_socket.h"
#include "net/socket/ssl_server_socket.h"
#include "remoting/base/constants.h"
#include "remoting/protocol/jingle_session_manager.h"
#include "third_party/libjingle/source/talk/base/thread.h"
#include "third_party/libjingle/source/talk/p2p/base/session.h"
#include "third_party/libjingle/source/talk/p2p/base/p2ptransportchannel.h"

using cricket::BaseSession;

namespace remoting {

namespace protocol {

const char JingleSession::kChromotingContentName[] = "chromoting";

namespace {

const char kControlChannelName[] = "control";
const char kEventChannelName[] = "event";
const char kVideoChannelName[] = "video";
const char kVideoRtpChannelName[] = "videortp";
const char kVideoRtcpChannelName[] = "videortcp";

const int kMasterKeyLength = 16;
const int kChannelKeyLength = 16;

// Value is choosen to balance the extra latency against the reduced
// load due to ACK traffic.
const int kTcpAckDelayMilliseconds = 10;

// Helper method to create a SSL client socket.
net::SSLClientSocket* CreateSSLClientSocket(
    net::StreamSocket* socket, scoped_refptr<net::X509Certificate> cert,
    net::CertVerifier* cert_verifier) {
  net::SSLConfig ssl_config;
  ssl_config.cached_info_enabled = false;
  ssl_config.false_start_enabled = false;
  ssl_config.ssl3_enabled = true;
  ssl_config.tls1_enabled = true;

  // Certificate provided by the host doesn't need authority.
  net::SSLConfig::CertAndStatus cert_and_status;
  cert_and_status.cert_status = net::CERT_STATUS_AUTHORITY_INVALID;
  cert_and_status.cert = cert;
  ssl_config.allowed_bad_certs.push_back(cert_and_status);

  // SSLClientSocket takes ownership of the adapter.
  net::HostPortPair host_and_pair(JingleSession::kChromotingContentName, 0);
  net::SSLClientSocket* ssl_socket =
      net::ClientSocketFactory::GetDefaultFactory()->CreateSSLClientSocket(
          socket, host_and_pair, ssl_config, NULL, cert_verifier);
  return ssl_socket;
}

std::string GenerateRandomMasterKey() {
  std::string result;
  result.resize(kMasterKeyLength);
  base::RandBytes(&result[0], result.size());
  return result;
}

std::string EncryptMasterKey(const std::string& host_public_key,
                             const std::string& master_key) {
  // TODO(sergeyu): Implement RSA public key encryption in src/crypto
  // and actually encrypt the key here.
  return master_key;
}

bool DecryptMasterKey(const crypto::RSAPrivateKey* private_key,
                      const std::string& encrypted_master_key,
                      std::string* master_key) {
  // TODO(sergeyu): Implement RSA public key encryption in src/crypto
  // and actually encrypt the key here.
  *master_key = encrypted_master_key;
  return true;
}

// Generates channel key from master key and channel name. Must be
// used to generate channel key so that we don't use the same key for
// different channels. The key is calculated as
//   HMAC_SHA256(master_key, channel_name)
bool GetChannelKey(const std::string& channel_name,
                   const std::string& master_key,
                   std::string* channel_key) {
  crypto::HMAC hmac(crypto::HMAC::SHA256);
  hmac.Init(channel_name);
  channel_key->resize(kChannelKeyLength);
  if (!hmac.Sign(master_key,
                 reinterpret_cast<unsigned char*>(&(*channel_key)[0]),
                 channel_key->size())) {
    *channel_key = "";
    return false;
  }
  return true;
}

}  // namespace

// static
JingleSession* JingleSession::CreateClientSession(
    JingleSessionManager* manager, const std::string& host_public_key) {
  return new JingleSession(manager, NULL, NULL, host_public_key);
}

// static
JingleSession* JingleSession::CreateServerSession(
    JingleSessionManager* manager,
    scoped_refptr<net::X509Certificate> certificate,
    crypto::RSAPrivateKey* key) {
  return new JingleSession(manager, certificate, key, "");
}

JingleSession::JingleSession(
    JingleSessionManager* jingle_session_manager,
    scoped_refptr<net::X509Certificate> local_cert,
    crypto::RSAPrivateKey* local_private_key,
    const std::string& peer_public_key)
    : jingle_session_manager_(jingle_session_manager),
      local_cert_(local_cert),
      master_key_(GenerateRandomMasterKey()),
      state_(INITIALIZING),
      closed_(false),
      closing_(false),
      cricket_session_(NULL),
      ALLOW_THIS_IN_INITIALIZER_LIST(connect_callback_(
          this, &JingleSession::OnConnect)),
      ALLOW_THIS_IN_INITIALIZER_LIST(ssl_connect_callback_(
          this, &JingleSession::OnSSLConnect)),
      ALLOW_THIS_IN_INITIALIZER_LIST(task_factory_(this)) {
  // TODO(hclam): Need a better way to clone a key.
  if (local_private_key) {
    std::vector<uint8> key_bytes;
    CHECK(local_private_key->ExportPrivateKey(&key_bytes));
    local_private_key_.reset(
        crypto::RSAPrivateKey::CreateFromPrivateKeyInfo(key_bytes));
    CHECK(local_private_key_.get());
  }
}

JingleSession::~JingleSession() {
  // Reset the callback so that it's not called from Close().
  state_change_callback_.reset();
  Close();
  jingle_session_manager_->SessionDestroyed(this);
}

void JingleSession::Init(cricket::Session* cricket_session) {
  DCHECK(CalledOnValidThread());

  cricket_session_ = cricket_session;
  jid_ = cricket_session_->remote_name();
  cert_verifier_.reset(new net::CertVerifier());
  cricket_session_->SignalState.connect(
      this, &JingleSession::OnSessionState);
  cricket_session_->SignalError.connect(
      this, &JingleSession::OnSessionError);
}

std::string JingleSession::GetEncryptedMasterKey() const {
  DCHECK(CalledOnValidThread());
  return EncryptMasterKey(peer_public_key_, master_key_);
}

void JingleSession::CloseInternal(int result, bool failed) {
  DCHECK(CalledOnValidThread());

  if (!closed_ && !closing_) {
    closing_ = true;

    // Inform the StateChangeCallback, so calling code knows not to touch any
    // channels.
    if (failed)
      SetState(FAILED);
    else
      SetState(CLOSED);

    // Now tear down the remoting channel resources.
    control_channel_.reset();
    control_socket_.reset();
    control_ssl_socket_.reset();
    event_channel_.reset();
    event_socket_.reset();
    event_ssl_socket_.reset();
    video_channel_.reset();
    video_socket_.reset();
    video_ssl_socket_.reset();
    video_rtp_channel_.reset();
    video_rtcp_channel_.reset();

    // Tear down the cricket session, including the cricket transport channels.
    if (cricket_session_) {
      cricket_session_->Terminate();
      cricket_session_->SignalState.disconnect(this);
    }

    closed_ = true;
  }
  cert_verifier_.reset();
}

bool JingleSession::HasSession(cricket::Session* cricket_session) {
  DCHECK(CalledOnValidThread());
  return cricket_session_ == cricket_session;
}

cricket::Session* JingleSession::ReleaseSession() {
  DCHECK(CalledOnValidThread());

  // Session may be destroyed only after it is closed.
  DCHECK(closed_);

  cricket::Session* session = cricket_session_;
  if (cricket_session_)
    cricket_session_->SignalState.disconnect(this);
  cricket_session_ = NULL;
  return session;
}

void JingleSession::SetStateChangeCallback(StateChangeCallback* callback) {
  DCHECK(CalledOnValidThread());
  DCHECK(callback);
  state_change_callback_.reset(callback);
}

net::Socket* JingleSession::control_channel() {
  DCHECK(CalledOnValidThread());
  return control_ssl_socket_.get();
}

net::Socket* JingleSession::event_channel() {
  DCHECK(CalledOnValidThread());
  return event_ssl_socket_.get();
}

// TODO(sergeyu): Remove this method after we switch to RTP.
net::Socket* JingleSession::video_channel() {
  DCHECK(CalledOnValidThread());
  return video_ssl_socket_.get();
}

net::Socket* JingleSession::video_rtp_channel() {
  DCHECK(CalledOnValidThread());
  return video_rtp_channel_.get();
}

net::Socket* JingleSession::video_rtcp_channel() {
  DCHECK(CalledOnValidThread());
  return video_rtcp_channel_.get();
}

const std::string& JingleSession::jid() {
  // TODO(sergeyu): Fix ChromotingHost so that it doesn't call this
  // method on invalid thread and uncomment this DCHECK.
  // See crbug.com/88600 .
  // DCHECK(CalledOnValidThread());
  return jid_;
}

const CandidateSessionConfig* JingleSession::candidate_config() {
  DCHECK(CalledOnValidThread());
  DCHECK(candidate_config_.get());
  return candidate_config_.get();
}

void JingleSession::set_candidate_config(
    const CandidateSessionConfig* candidate_config) {
  DCHECK(CalledOnValidThread());
  DCHECK(!candidate_config_.get());
  DCHECK(candidate_config);
  candidate_config_.reset(candidate_config);
}

scoped_refptr<net::X509Certificate> JingleSession::local_certificate() const {
  DCHECK(CalledOnValidThread());
  return local_cert_;
}

const SessionConfig* JingleSession::config() {
  // TODO(sergeyu): Fix ChromotingHost so that it doesn't call this
  // method on invalid thread and uncomment this DCHECK.
  // See crbug.com/88600 .
  // DCHECK(CalledOnValidThread());
  DCHECK(config_.get());
  return config_.get();
}

void JingleSession::set_config(const SessionConfig* config) {
  DCHECK(CalledOnValidThread());
  DCHECK(!config_.get());
  DCHECK(config);
  config_.reset(config);
}

const std::string& JingleSession::initiator_token() {
  DCHECK(CalledOnValidThread());
  return initiator_token_;
}

void JingleSession::set_initiator_token(const std::string& initiator_token) {
  DCHECK(CalledOnValidThread());
  initiator_token_ = initiator_token;
}

const std::string& JingleSession::receiver_token() {
  DCHECK(CalledOnValidThread());
  return receiver_token_;
}

void JingleSession::set_receiver_token(const std::string& receiver_token) {
  DCHECK(CalledOnValidThread());
  receiver_token_ = receiver_token;
}

void JingleSession::Close() {
  DCHECK(CalledOnValidThread());

  CloseInternal(net::ERR_CONNECTION_CLOSED, false);
}

void JingleSession::OnSessionState(
    BaseSession* session, BaseSession::State state) {
  DCHECK(CalledOnValidThread());
  DCHECK_EQ(cricket_session_, session);

  if (closed_) {
    // Don't do anything if we already closed.
    return;
  }

  switch (state) {
    case cricket::Session::STATE_SENTINITIATE:
    case cricket::Session::STATE_RECEIVEDINITIATE:
      OnInitiate();
      break;

    case cricket::Session::STATE_SENTACCEPT:
    case cricket::Session::STATE_RECEIVEDACCEPT:
      OnAccept();
      break;

    case cricket::Session::STATE_SENTTERMINATE:
    case cricket::Session::STATE_RECEIVEDTERMINATE:
    case cricket::Session::STATE_SENTREJECT:
    case cricket::Session::STATE_RECEIVEDREJECT:
      OnTerminate();
      break;

    case cricket::Session::STATE_DEINIT:
      // Close() must have been called before this.
      NOTREACHED();
      break;

    default:
      // We don't care about other steates.
      break;
  }
}

void JingleSession::OnSessionError(
    BaseSession* session, BaseSession::Error error) {
  DCHECK(CalledOnValidThread());
  DCHECK_EQ(cricket_session_, session);

  if (error != cricket::Session::ERROR_NONE) {
    CloseInternal(net::ERR_CONNECTION_ABORTED, true);
  }
}

void JingleSession::OnInitiate() {
  DCHECK(CalledOnValidThread());
  jid_ = cricket_session_->remote_name();

  const cricket::SessionDescription* session_description;
  // If we initiate the session, we get to specify the content name. When
  // accepting one, the remote end specifies it.
  if (cricket_session_->initiator()) {
    session_description = cricket_session_->local_description();
  } else {
    session_description = cricket_session_->remote_description();
  }
  const cricket::ContentInfo* content =
      session_description->FirstContentByType(kChromotingXmlNamespace);
  CHECK(content);
  const ContentDescription* content_description =
      static_cast<const ContentDescription*>(content->description);
  std::string content_name = content->name;

  if (!cricket_session_->initiator()) {
    if (!DecryptMasterKey(local_private_key_.get(),
                          content_description->master_key(), &master_key_)) {
      LOG(ERROR) << "Failed to decrypt master-key";
      CloseInternal(net::ERR_CONNECTION_FAILED, true);
      return;
    }
  }

  // Create video RTP channels.
  raw_video_rtp_channel_ =
      cricket_session_->CreateChannel(content_name, kVideoRtpChannelName);
  video_rtp_channel_.reset(
      new jingle_glue::TransportChannelSocketAdapter(raw_video_rtp_channel_));
  raw_video_rtcp_channel_ =
      cricket_session_->CreateChannel(content_name, kVideoRtcpChannelName);
  video_rtcp_channel_.reset(
      new jingle_glue::TransportChannelSocketAdapter(raw_video_rtcp_channel_));

  // Create control channel.
  raw_control_channel_ =
      cricket_session_->CreateChannel(content_name, kControlChannelName);
  control_channel_.reset(
      new jingle_glue::TransportChannelSocketAdapter(raw_control_channel_));

  // Create event channel.
  raw_event_channel_ =
      cricket_session_->CreateChannel(content_name, kEventChannelName);
  event_channel_.reset(
      new jingle_glue::TransportChannelSocketAdapter(raw_event_channel_));

  // Create video channel.
  // TODO(wez): When we have RTP video support, we'll need to negotiate the
  // type of video channel to allocate, for legacy compatibility.
  raw_video_channel_ =
      cricket_session_->CreateChannel(content_name, kVideoChannelName);
  video_channel_.reset(
      new jingle_glue::TransportChannelSocketAdapter(raw_video_channel_));

  if (!cricket_session_->initiator()) {
    if (!jingle_session_manager_->AcceptConnection(this, cricket_session_)) {
      Close();
      // Release session so that
      // JingleSessionManager::SessionDestroyed() doesn't try to call
      // cricket::SessionManager::DestroySession() for it.
      ReleaseSession();
      delete this;
      return;
    }
  }

  // Set state to CONNECTING if the session is being accepted.
  SetState(CONNECTING);
}

bool JingleSession::EstablishPseudoTcp(
    net::Socket* channel,
    scoped_ptr<net::StreamSocket>* stream) {
  jingle_glue::PseudoTcpAdapter* adapter =
      new jingle_glue::PseudoTcpAdapter(channel);
  adapter->SetAckDelay(kTcpAckDelayMilliseconds);
  adapter->SetNoDelay(true);

  stream->reset(adapter);
  int result = (*stream)->Connect(&connect_callback_);
  return (result == net::OK) || (result == net::ERR_IO_PENDING);
}

bool JingleSession::EstablishSSLConnection(
    net::StreamSocket* socket,
    scoped_ptr<net::StreamSocket>* ssl_socket) {
  DCHECK(socket);
  DCHECK(socket->IsConnected());
  if (cricket_session_->initiator()) {
    // Create client SSL socket.
    net::SSLClientSocket* ssl_client_socket = CreateSSLClientSocket(
        socket, remote_cert_, cert_verifier_.get());
    ssl_socket->reset(ssl_client_socket);

    int ret = ssl_client_socket->Connect(&ssl_connect_callback_);
    if (ret == net::ERR_IO_PENDING) {
      return true;
    } else if (ret != net::OK) {
      LOG(ERROR) << "Failed to establish SSL connection";
      cricket_session_->Terminate();
      return false;
    }
  } else {
    // Create server SSL socket.
    net::SSLConfig ssl_config;
    net::SSLServerSocket* ssl_server_socket = net::CreateSSLServerSocket(
        socket, local_cert_, local_private_key_.get(), ssl_config);
    ssl_socket->reset(ssl_server_socket);

    int ret = ssl_server_socket->Handshake(&ssl_connect_callback_);
    if (ret == net::ERR_IO_PENDING) {
      return true;
    } else if (ret != net::OK) {
      LOG(ERROR) << "Failed to establish SSL connection";
      cricket_session_->Terminate();
      return false;
    }
  }
  // Reach here if net::OK is received.
  ssl_connect_callback_.Run(net::OK);
  return true;
}

bool JingleSession::InitializeConfigFromDescription(
    const cricket::SessionDescription* description) {
  // We should only be called after ParseContent has succeeded, in which
  // case there will always be a Chromoting session configuration.
  const cricket::ContentInfo* content =
    description->FirstContentByType(kChromotingXmlNamespace);
  CHECK(content);
  const protocol::ContentDescription* content_description =
    static_cast<const protocol::ContentDescription*>(content->description);
  CHECK(content_description);

  remote_cert_ = content_description->certificate();
  if (!remote_cert_) {
    LOG(ERROR) << "Connection response does not specify certificate";
    return false;
  }

  scoped_ptr<SessionConfig> config(
    content_description->config()->GetFinalConfig());
  if (!config.get()) {
    LOG(ERROR) << "Connection response does not specify configuration";
    return false;
  }
  if (!candidate_config()->IsSupported(config.get())) {
    LOG(ERROR) << "Connection response specifies an invalid configuration";
    return false;
  }

  set_config(config.release());
  return true;
}

void JingleSession::InitializeChannels() {
  // Disable incoming connections on the host so that we don't traverse
  // the firewall.
  if (!cricket_session_->initiator()) {
    raw_control_channel_->GetP2PChannel()->set_incoming_only(true);
    raw_event_channel_->GetP2PChannel()->set_incoming_only(true);
    raw_video_channel_->GetP2PChannel()->set_incoming_only(true);
    raw_video_rtp_channel_->GetP2PChannel()->set_incoming_only(true);
    raw_video_rtcp_channel_->GetP2PChannel()->set_incoming_only(true);
  }

  // Create the Control, Event and Video connections on the channels.
  if (!EstablishPseudoTcp(control_channel_.release(),
                           &control_socket_) ||
      !EstablishPseudoTcp(event_channel_.release(),
                           &event_socket_) ||
      !EstablishPseudoTcp(video_channel_.release(),
                           &video_socket_)) {
    CloseInternal(net::ERR_CONNECTION_FAILED, true);
    return;
  }
}

void JingleSession::OnAccept() {
  DCHECK(CalledOnValidThread());

  // If we initiated the session, store the candidate configuration that the
  // host responded with, to refer to later.
  if (cricket_session_->initiator()) {
    if (!InitializeConfigFromDescription(
        cricket_session_->remote_description())) {
      CloseInternal(net::ERR_CONNECTION_FAILED, true);
      return;
    }
  }

  // TODO(sergeyu): This is a hack: Currently set_incoming_only()
  // needs to be called on each channel before the channel starts
  // creating candidates but after session is accepted (after
  // TransportChannelProxy::GetP2PChannel() starts returning actual
  // P2P channel). By posting a task here we can call it at the right
  // moment. This problem will go away when we switch to Pepper P2P
  // API.
  MessageLoop::current()->PostTask(FROM_HERE, task_factory_.NewRunnableMethod(
      &JingleSession::InitializeChannels));
}

void JingleSession::OnTerminate() {
  DCHECK(CalledOnValidThread());
  CloseInternal(net::ERR_CONNECTION_ABORTED, false);
}

void JingleSession::OnConnect(int result) {
  DCHECK(CalledOnValidThread());

  if (result != net::OK) {
    LOG(ERROR) << "PseudoTCP connection failed: " << result;
    CloseInternal(result, true);
    return;
  }

  if (control_socket_.get() && control_socket_->IsConnected()) {
    if (!EstablishSSLConnection(control_socket_.release(),
                                &control_ssl_socket_)) {
      LOG(ERROR) << "Establish control channel failed";
      CloseInternal(net::ERR_CONNECTION_FAILED, true);
      return;
    }
  }
  if (event_socket_.get() && event_socket_->IsConnected()) {
    if (!EstablishSSLConnection(event_socket_.release(),
                                &event_ssl_socket_)) {
      LOG(ERROR) << "Establish control event failed";
      CloseInternal(net::ERR_CONNECTION_FAILED, true);
      return;
    }
  }
  if (video_socket_.get() && video_socket_->IsConnected()) {
    if (!EstablishSSLConnection(video_socket_.release(),
                                &video_ssl_socket_)) {
      LOG(ERROR) << "Establish control video failed";
      CloseInternal(net::ERR_CONNECTION_FAILED, true);
      return;
    }
  }
}

void JingleSession::OnSSLConnect(int result) {
  DCHECK(CalledOnValidThread());

  DCHECK(!closed_);
  if (result != net::OK) {
    LOG(ERROR) << "Error during SSL connection: " << result;
    CloseInternal(result, true);
    return;
  }

  if (event_ssl_socket_.get() && event_ssl_socket_->IsConnected() &&
      control_ssl_socket_.get() && control_ssl_socket_->IsConnected() &&
      video_ssl_socket_.get() && video_ssl_socket_->IsConnected()) {
    SetState(CONNECTED);
  }
}

void JingleSession::SetState(State new_state) {
  DCHECK(CalledOnValidThread());

  if (new_state != state_) {
    DCHECK_NE(state_, CLOSED);
    DCHECK_NE(state_, FAILED);

    state_ = new_state;
    if (!closed_ && state_change_callback_.get())
      state_change_callback_->Run(new_state);
  }
}

}  // namespace protocol

}  // namespace remoting
