// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MULTIDEVICE_SERVICE_H_
#define COMPONENTS_MULTIDEVICE_SERVICE_H_

namespace multidevice {

class MultiDeviceService : public service_manager::Service {
 public:
  MultiDeviceService();
  ~MultiDeviceService() override;
  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

 private:
  void CreateDeviceSyncImpl(
      service_manager::ServiceContextRefFactory* ref_factory,
      device_sync::mojom::DeviceSyncRequest request);

  std::unique_ptr<service_manager::ServiceContextRefFactory> ref_factory_;
  service_manager::BinderRegistry registry_;
  base::WeakPtrFactory<MultiDeviceService> weak_ptr_factory_;
};

}  // namespace multidevice

#endif  // COMPONENTS_MULTIDEVICE_SERVICE_H_