// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/generic_sensor/platform_sensor_fusion.h"

#include "base/logging.h"
#include "services/device/generic_sensor/platform_sensor_fusion_algorithm.h"
#include "services/device/generic_sensor/platform_sensor_provider.h"

namespace device {

PlatformSensorFusion::PlatformSensorFusion(
    mojo::ScopedSharedBufferMapping mapping,
    PlatformSensorProvider* provider,
    const PlatformSensorProviderBase::CreateSensorCallback& callback,
    std::unique_ptr<PlatformSensorFusionAlgorithm> fusion_algorithm)
    : PlatformSensor(fusion_algorithm->GetFusedType(),
                     std::move(mapping),
                     provider),
      callback_(callback),
      fusion_algorithm_(std::move(fusion_algorithm)) {
  source_sensors_.reserve(fusion_algorithm->source_types().size());
  for (mojom::SensorType type : fusion_algorithm->source_types()) {
    auto sensor = provider->GetSensor(type);
    if (sensor) {
      AddSourceSensor(std::move(sensor));
    } else {
      provider->CreateSensor(
          type, base::Bind(&PlatformSensorFusion::CreateSensorCallback, this));
    }
  }
}

mojom::ReportingMode PlatformSensorFusion::GetReportingMode() {
  return reporting_mode_;
}

PlatformSensorConfiguration PlatformSensorFusion::GetDefaultConfiguration() {
  PlatformSensorConfiguration default_configuration;
  for (const auto& sensor : source_sensors_) {
    double frequency = sensor->GetDefaultConfiguration().frequency();
    if (frequency > default_configuration.frequency())
      default_configuration.set_frequency(frequency);
  }
  return default_configuration;
}

bool PlatformSensorFusion::StartSensor(
    const PlatformSensorConfiguration& configuration) {
  for (const auto& sensor : source_sensors_) {
    if (!sensor->StartSensor(configuration))
      return false;
  }

  if (GetReportingMode() == mojom::ReportingMode::CONTINUOUS) {
    timer_.Start(
        FROM_HERE,
        base::TimeDelta::FromMicroseconds(base::Time::kMicrosecondsPerSecond /
                                          configuration.frequency()),
        base::Bind(&PlatformSensorFusion::OnSensorReadingChanged,
                   base::Unretained(this), GetType()));
  }

  fusion_algorithm_->SetFrequency(configuration.frequency());

  return true;
}

void PlatformSensorFusion::StopSensor() {
  for (const auto& sensor : source_sensors_)
    sensor->StopSensor();

  if (timer_.IsRunning())
    timer_.Stop();

  fusion_algorithm_->Reset();
}

bool PlatformSensorFusion::CheckSensorConfiguration(
    const PlatformSensorConfiguration& configuration) {
  for (const auto& sensor : source_sensors_) {
    if (!sensor->CheckSensorConfiguration(configuration))
      return false;
  }
  return true;
}

void PlatformSensorFusion::OnSensorReadingChanged(mojom::SensorType type) {
  SensorReading reading;
  reading.raw.timestamp =
      (base::TimeTicks::Now() - base::TimeTicks()).InSecondsF();

  if (!fusion_algorithm_->GetFusedData(type, &reading))
    return;

  if (GetReportingMode() == mojom::ReportingMode::ON_CHANGE &&
      !fusion_algorithm_->IsReadingSignificantlyDifferent(reading_, reading)) {
    return;
  }

  reading_ = reading;
  UpdateSensorReading(reading_,
                      GetReportingMode() == mojom::ReportingMode::ON_CHANGE);
}

void PlatformSensorFusion::OnSensorError() {
  NotifySensorError();
}

bool PlatformSensorFusion::IsSuspended() {
  for (auto& client : clients_) {
    if (!client.IsSuspended())
      return false;
  }
  return true;
}

bool PlatformSensorFusion::GetSourceReading(size_t index,
                                            SensorReading* result) {
  DCHECK_LT(index, source_sensors_.size());

  return source_sensors_[index]->GetLatestReading(result);
}

PlatformSensorFusion::~PlatformSensorFusion() {
  if (source_sensors_.size() != fusion_algorithm_->source_types().size())
    return;

  for (const auto& sensor : source_sensors_) {
    sensor->RemoveClient(this);
  }
}

void PlatformSensorFusion::CreateSensorCallback(
    scoped_refptr<PlatformSensor> sensor) {
  if (sensor)
    AddSourceSensor(std::move(sensor));
  else if (callback_)
    std::move(callback_).Run(nullptr);
}

void PlatformSensorFusion::AddSourceSensor(
    scoped_refptr<PlatformSensor> sensor) {
  DCHECK(sensor);
  source_sensors_.push_back(std::move(sensor));

  if (source_sensors_.size() != fusion_algorithm_->source_types().size())
    return;

  reporting_mode_ = mojom::ReportingMode::CONTINUOUS;

  for (const auto& sensor : source_sensors_) {
    sensor->AddClient(this);
    if (sensor->GetReportingMode() == mojom::ReportingMode::ON_CHANGE)
      reporting_mode_ = mojom::ReportingMode::ON_CHANGE;
  }

  fusion_algorithm_->set_fusion_sensor(this);

  callback_.Run(this);
}

}  // namespace device
