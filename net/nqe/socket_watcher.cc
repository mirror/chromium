// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/nqe/socket_watcher.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "net/base/address_list.h"
#include "net/base/ip_address.h"

namespace net {

namespace nqe {

namespace internal {

namespace {

base::Optional<uint64_t> CalculateSubnetID(const AddressList& address_list) {
  if (address_list.empty())
    return base::nullopt;

  const IPAddress& ip_addr = address_list.front().address();

  if (!ip_addr.IsValid() || ip_addr.IsReserved())
    return base::nullopt;

  IPAddressBytes bytes = ip_addr.bytes();
  if (ip_addr.IsIPv4()) {
    // If the IP address is IPv4, take the /24 prefix of the IP address as
    // the unique identifier since the latency will not vary across hosts
    // inside the subnet.
    return ((static_cast<uint64_t>(bytes[0]) << 16) & 0x0000000000FF0000ULL) |
           ((static_cast<uint64_t>(bytes[1]) << 8) & 0x000000000000FF00ULL) |
           (static_cast<uint64_t>(bytes[2]) & 0x00000000000000FFULL);
  }

  if (ip_addr.IsIPv4MappedIPv6()) {
    // If the IP address is an IPv4 embedded in an IPv6 address, extract the
    // /24 prefix address using the 13th, 14th and 15th bytes.
    return ((static_cast<uint64_t>(bytes[12]) << 16) & 0x0000000000FF0000ULL) |
           ((static_cast<uint64_t>(bytes[13]) << 8) & 0x000000000000FF00ULL) |
           (static_cast<uint64_t>(bytes[14]) & 0x00000000000000FFULL);
  }

  // Assume that the address is IPv6 since it is valid and isn't IPv4 or an
  // IPv4 mapped to an IPv6. The /48 prefix of an IPv6 address is the customer
  // site and it is okay to assume that the latency variance is minimal for
  // a single customer site.
  return ((static_cast<uint64_t>(bytes[0]) << 40) & 0x0000FF0000000000ULL) |
         ((static_cast<uint64_t>(bytes[1]) << 32) & 0x000000FF00000000ULL) |
         ((static_cast<uint64_t>(bytes[2]) << 24) & 0x00000000FF000000ULL) |
         ((static_cast<uint64_t>(bytes[3]) << 16) & 0x0000000000FF0000ULL) |
         ((static_cast<uint64_t>(bytes[4]) << 8) & 0x000000000000FF00ULL) |
         (static_cast<uint64_t>(bytes[5]) & 0x00000000000000FFULL);
}

}  // namespace

SocketWatcher::SocketWatcher(
    SocketPerformanceWatcherFactory::Protocol protocol,
    const AddressList& address_list,
    base::TimeDelta min_notification_interval,
    bool allow_rtt_private_address,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    OnUpdatedRTTAvailableCallback updated_rtt_observation_callback,
    base::TickClock* tick_clock)
    : protocol_(protocol),
      task_runner_(std::move(task_runner)),
      updated_rtt_observation_callback_(updated_rtt_observation_callback),
      rtt_notifications_minimum_interval_(min_notification_interval),
      run_rtt_callback_(allow_rtt_private_address ||
                        (!address_list.empty() &&
                         !address_list.front().address().IsReserved())),
      tick_clock_(tick_clock),
      subnet_id_(CalculateSubnetID(address_list)) {
  DCHECK(tick_clock_);
}

SocketWatcher::~SocketWatcher() {}

bool SocketWatcher::ShouldNotifyUpdatedRTT() const {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Do not allow incoming notifications if the last notification was more
  // recent than |rtt_notifications_minimum_interval_| ago. This helps in
  // reducing the overhead of obtaining the RTT values.
  return run_rtt_callback_ &&
         tick_clock_->NowTicks() - last_rtt_notification_ >=
             rtt_notifications_minimum_interval_;
}

void SocketWatcher::OnUpdatedRTTAvailable(const base::TimeDelta& rtt) {
  DCHECK(thread_checker_.CalledOnValidThread());

  last_rtt_notification_ = tick_clock_->NowTicks();
  task_runner_->PostTask(
      FROM_HERE, base::Bind(updated_rtt_observation_callback_, protocol_, rtt,
                            subnet_id_));
}

void SocketWatcher::OnConnectionChanged() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

}  // namespace internal

}  // namespace nqe

}  // namespace net
