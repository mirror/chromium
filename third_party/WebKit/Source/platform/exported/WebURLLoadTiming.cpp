/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "public/platform/WebURLLoadTiming.h"

#include "platform/loader/fetch/ResourceLoadTiming.h"
#include "public/platform/WebString.h"

namespace blink {

void WebURLLoadTiming::Initialize() {
  private_ = ResourceLoadTiming::Create();
}

void WebURLLoadTiming::Reset() {
  private_.Reset();
}

void WebURLLoadTiming::Assign(const WebURLLoadTiming& other) {
  private_ = other.private_;
}

double WebURLLoadTiming::RequestTime() const {
  return private_->RequestTime().InSecondsF();
}

void WebURLLoadTiming::SetRequestTime(double time) {
  DCHECK_GE(time, 0.0);
  private_->SetRequestTime(TimeTicks::FromSeconds(time));
}

double WebURLLoadTiming::ProxyStart() const {
  return private_->ProxyStart().InSecondsF();
}

void WebURLLoadTiming::SetProxyStart(double start) {
  DCHECK_GE(start, 0.0);
  private_->SetProxyStart(TimeTicks::FromSeconds(start));
}

double WebURLLoadTiming::ProxyEnd() const {
  return private_->ProxyEnd().InSecondsF();
}

void WebURLLoadTiming::SetProxyEnd(double end) {
  DCHECK_GE(end, 0.0);
  private_->SetProxyEnd(TimeTicks::FromSeconds(end));
}

double WebURLLoadTiming::DnsStart() const {
  return private_->DnsStart().InSecondsF();
}

void WebURLLoadTiming::SetDNSStart(double start) {
  DCHECK_GE(start, 0.0);
  private_->SetDnsStart(TimeTicks::FromSeconds(start));
}

double WebURLLoadTiming::DnsEnd() const {
  return private_->DnsEnd().InSecondsF();
}

void WebURLLoadTiming::SetDNSEnd(double end) {
  DCHECK_GE(end, 0.0);
  private_->SetDnsEnd(TimeTicks::FromSeconds(end));
}

double WebURLLoadTiming::ConnectStart() const {
  return private_->ConnectStart().InSecondsF();
}

void WebURLLoadTiming::SetConnectStart(double start) {
  DCHECK_GE(start, 0.0);
  private_->SetConnectStart(TimeTicks::FromSeconds(start));
}

double WebURLLoadTiming::ConnectEnd() const {
  return private_->ConnectEnd().InSecondsF();
}

void WebURLLoadTiming::SetConnectEnd(double end) {
  DCHECK_GE(end, 0.0);
  private_->SetConnectEnd(TimeTicks::FromSeconds(end));
}

double WebURLLoadTiming::WorkerStart() const {
  return private_->WorkerStart().InSecondsF();
}

void WebURLLoadTiming::SetWorkerStart(double start) {
  DCHECK_GE(start, 0.0);
  private_->SetWorkerStart(TimeTicks::FromSeconds(start));
}

double WebURLLoadTiming::WorkerReady() const {
  return private_->WorkerReady().InSecondsF();
}

void WebURLLoadTiming::SetWorkerReady(double ready) {
  DCHECK_GE(ready, 0.0);
  private_->SetWorkerReady(TimeTicks::FromSeconds(ready));
}

double WebURLLoadTiming::SendStart() const {
  return private_->SendStart().InSecondsF();
}

void WebURLLoadTiming::SetSendStart(double start) {
  DCHECK_GE(start, 0.0);
  private_->SetSendStart(TimeTicks::FromSeconds(start));
}

double WebURLLoadTiming::SendEnd() const {
  return private_->SendEnd().InSecondsF();
}

void WebURLLoadTiming::SetSendEnd(double end) {
  DCHECK_GE(end, 0.0);
  private_->SetSendEnd(TimeTicks::FromSeconds(end));
}

double WebURLLoadTiming::ReceiveHeadersEnd() const {
  return private_->ReceiveHeadersEnd().InSecondsF();
}

void WebURLLoadTiming::SetReceiveHeadersEnd(double end) {
  DCHECK_GE(end, 0.0);
  private_->SetReceiveHeadersEnd(TimeTicks::FromSeconds(end));
}

double WebURLLoadTiming::SslStart() const {
  return private_->SslStart().InSecondsF();
}

void WebURLLoadTiming::SetSSLStart(double start) {
  DCHECK_GE(start, 0.0);
  private_->SetSslStart(TimeTicks::FromSeconds(start));
}

double WebURLLoadTiming::SslEnd() const {
  return private_->SslEnd().InSecondsF();
}

void WebURLLoadTiming::SetSSLEnd(double end) {
  DCHECK_GE(end, 0.0);
  private_->SetSslEnd(TimeTicks::FromSeconds(end));
}

double WebURLLoadTiming::PushStart() const {
  return private_->PushStart().InSecondsF();
}

void WebURLLoadTiming::SetPushStart(double start) {
  DCHECK_GE(start, 0.0);
  private_->SetPushStart(TimeTicks::FromSeconds(start));
}

double WebURLLoadTiming::PushEnd() const {
  return private_->PushEnd().InSecondsF();
}

void WebURLLoadTiming::SetPushEnd(double end) {
  DCHECK_GE(end, 0.0);
  private_->SetPushEnd(TimeTicks::FromSeconds(end));
}

WebURLLoadTiming::WebURLLoadTiming(scoped_refptr<ResourceLoadTiming> value)
    : private_(std::move(value)) {}

WebURLLoadTiming& WebURLLoadTiming::operator=(
    scoped_refptr<ResourceLoadTiming> value) {
  private_ = std::move(value);
  return *this;
}

WebURLLoadTiming::operator scoped_refptr<ResourceLoadTiming>() const {
  return private_.Get();
}

}  // namespace blink
