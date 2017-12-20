// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_CRONET_URL_REQUEST_CONTEXT_H_
#define COMPONENTS_CRONET_CRONET_URL_REQUEST_CONTEXT_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread.h"
#include "components/prefs/json_pref_store.h"
#include "net/nqe/effective_connection_type.h"
#include "net/nqe/effective_connection_type_observer.h"
#include "net/nqe/network_quality_estimator.h"
#include "net/nqe/network_quality_observation_source.h"
#include "net/nqe/rtt_throughput_estimates_observer.h"

class PrefService;

namespace base {
class SingleThreadTaskRunner;
class TimeTicks;
}  // namespace base

namespace net {
enum EffectiveConnectionType;
class CertVerifier;
class NetLog;
class ProxyConfigService;
class URLRequestContext;
class FileNetLogObserver;
}  // namespace net

namespace cronet {
class CronetPrefsManager;
class TestUtil;

struct URLRequestContextConfig;

// Wrapper around net::URLRequestContext.
class CronetURLRequestContext
    : public net::EffectiveConnectionTypeObserver,
      public net::RTTAndThroughputEstimatesObserver,
      public net::NetworkQualityEstimator::RTTObserver,
      public net::NetworkQualityEstimator::ThroughputObserver {
 public:
  // Delegate implemented by owner of CronetURLRequestContext.
  class Delegate {
   public:
    virtual void OnInitNetworkThread(CronetURLRequestContext* context) = 0;

    virtual void OnInitCertVerifierData(
        net::CertVerifier* cert_verifier,
        const std::string& cert_verifier_data) = 0;

    virtual void OnGetCertVerifierData(net::CertVerifier* cert_verifier) = 0;

    virtual void OnEffectiveConnectionTypeChanged(
        net::EffectiveConnectionType effective_connection_type) = 0;

    virtual void OnRTTOrThroughputEstimatesComputed(
        int32_t http_rtt_ms,
        int32_t transport_rtt_ms,
        int32_t downstream_throughput_kbps) = 0;

    virtual void OnRTTObservation(
        int32_t rtt_ms,
        int32_t timestamp_ms,
        net::NetworkQualityObservationSource source) = 0;

    virtual void OnThroughputObservation(
        int32_t throughput_kbps,
        int32_t timestamp_ms,
        net::NetworkQualityObservationSource source) = 0;

    virtual void OnStopNetLogCompleted() = 0;
  };

  CronetURLRequestContext(
      std::unique_ptr<URLRequestContextConfig> context_config,
      Delegate* delegate);

  ~CronetURLRequestContext() override;

  // Called on init thread to initialize URLRequestContext.
  void InitRequestContextOnInitThread();

  // Releases all resources for the request context and deletes the object.
  // Blocks until network thread is destroyed after running all pending tasks.
  void Destroy();

  // Posts a task that might depend on the context being initialized
  // to the network thread.
  void PostTaskToNetworkThread(const base::Location& posted_from,
                               const base::Closure& callback);

  bool IsOnNetworkThread() const;

  net::URLRequestContext* GetURLRequestContext();

  // TODO(xunjieli): Keep only one version of StartNetLog().

  // Starts NetLog logging to file. This can be called on any thread.
  // Return false if |file_name| cannot be opened.
  bool StartNetLogToFile(const std::string& file_name, bool log_all);

  // Starts NetLog logging to disk with a bounded amount of disk space. This
  // can be called on any thread.
  void StartNetLogToDisk(const std::string& dir_name,
                         bool log_all,
                         int max_size);

  // Stops NetLog logging to file. This can be called on any thread. This will
  // flush any remaining writes to disk.
  void StopNetLog();

  // Posts a task to Network thread to get serialized results of certificate
  // verifications of |context_|'s |cert_verifier|.
  void GetCertVerifierData();

  // Default net::LOAD flags used to create requests.
  int default_load_flags() const;

  // Configures the network quality estimator to observe requests to localhost,
  // to use smaller responses when estimating throughput, and to disable the
  // device offline checks when computing the effective connection type or when
  // writing the prefs. This should only be used for testing. This can be
  // called only after the network quality estimator has been enabled.
  void ConfigureNetworkQualityEstimatorForTesting(bool use_local_host_requests,
                                                  bool use_smaller_responses,
                                                  bool disable_offline_check);

  // Request that RTT and/or throughput observations should or should not be
  // provided by the network quality estimator.
  void ProvideRTTObservations(bool should);
  void ProvideThroughputObservations(bool should);

 private:
  // friend class CronetURLRequestContextAdapter;
  friend class TestUtil;

  // Initializes |context_| on the Network thread.
  void InitializeOnNetworkThread(
      std::unique_ptr<URLRequestContextConfig> config);

  // Runs a task that might depend on the context being initialized.
  // This method should only be run on the network thread.
  void RunTaskAfterContextInitOnNetworkThread(
      const base::Closure& task_to_run_after_context_init);

  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner() const;

  // Serializes results of certificate verifications of |context_|'s
  // |cert_verifier| on the Network thread.
  void GetCertVerifierDataOnNetworkThread();

  // Gets the file thread. Create one if there is none.
  base::Thread* GetFileThread();

  // Configures the network quality estimator to observe requests to localhost,
  // to use smaller responses when estimating throughput, and to disable the
  // device offline checks when computing the effective connection type or when
  // writing the prefs. This should only be used for testing.
  void ConfigureNetworkQualityEstimatorOnNetworkThreadForTesting(
      bool use_local_host_requests,
      bool use_smaller_responses,
      bool disable_offline_check);

  void ProvideRTTObservationsOnNetworkThread(bool should);
  void ProvideThroughputObservationsOnNetworkThread(bool should);

  // net::NetworkQualityEstimator::EffectiveConnectionTypeObserver
  // implementation.
  void OnEffectiveConnectionTypeChanged(
      net::EffectiveConnectionType effective_connection_type) override;

  // net::NetworkQualityEstimator::RTTAndThroughputEstimatesObserver
  // implementation.
  void OnRTTOrThroughputEstimatesComputed(
      base::TimeDelta http_rtt,
      base::TimeDelta transport_rtt,
      int32_t downstream_throughput_kbps) override;

  // net::NetworkQualityEstimator::RTTObserver implementation.
  void OnRTTObservation(int32_t rtt_ms,
                        const base::TimeTicks& timestamp,
                        net::NetworkQualityObservationSource source) override;

  // net::NetworkQualityEstimator::ThroughputObserver implementation.
  void OnThroughputObservation(
      int32_t throughput_kbps,
      const base::TimeTicks& timestamp,
      net::NetworkQualityObservationSource source) override;

  // Same as StartNetLogToDisk, but called only on the network thread.
  void StartNetLogToBoundedFileOnNetworkThread(const std::string& dir_path,
                                               bool include_socket_bytes,
                                               int size);

  // Same as StartNetLogToFile, but called only on the network thread.
  void StartNetLogOnNetworkThread(const base::FilePath& file_path,
                                  bool include_socket_bytes);

  // Stops NetLog logging on the network thread.
  void StopNetLogOnNetworkThread();

  // Callback for StopObserving() that unblocks the Java ConditionVariable and
  // signals that it is safe to access the NetLog files.
  void StopNetLogCompleted();

  // Initializes Network Quality Estimator (NQE) prefs manager on network
  // thread.
  void InitializeNQEPrefsOnNetworkThread() const;

  std::unique_ptr<base::DictionaryValue> GetNetLogInfo() const;

  // Network thread is owned by |this|, but is destroyed from java thread.
  base::Thread* network_thread_;

  // File thread should be destroyed last.
  std::unique_ptr<base::Thread> file_thread_;

  std::unique_ptr<net::FileNetLogObserver> net_log_file_observer_;

  // A network quality estimator. This member variable has to be destroyed after
  // destroying |cronet_prefs_manager_|, which owns NetworkQualityPrefsManager
  // that weakly references |network_quality_estimator_|.
  std::unique_ptr<net::NetworkQualityEstimator> network_quality_estimator_;

  // Manages the PrefService and all associated persistence managers
  // such as NetworkQualityPrefsManager, HostCachePersistenceManager, etc.
  // It should be destroyed before |network_quality_estimator_| and
  // after |context_|.
  std::unique_ptr<CronetPrefsManager> cronet_prefs_manager_;

  std::unique_ptr<net::URLRequestContext> context_;

  // Context config is only valid until context is initialized.
  std::unique_ptr<URLRequestContextConfig> context_config_;
  // As is the proxy config service, as ownership is passed to the
  // URLRequestContextBuilder.
  std::unique_ptr<net::ProxyConfigService> proxy_config_service_;

  // Effective experimental options. Kept for NetLog.
  std::unique_ptr<base::DictionaryValue> effective_experimental_options_;

  // A queue of tasks that need to be run after context has been initialized.
  base::queue<base::Closure> tasks_waiting_for_context_;
  bool is_context_initialized_;
  int default_load_flags_;

  // Object that owns this CronetURLRequestContext.
  Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(CronetURLRequestContext);
};

}  // namespace cronet

#endif  // COMPONENTS_CRONET_CRONET_URL_REQUEST_CONTEXT_H_
