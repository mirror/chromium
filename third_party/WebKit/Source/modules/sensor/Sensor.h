// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef Sensor_h
#define Sensor_h

#include "bindings/core/v8/ActiveScriptWrappable.h"
#include "common/feature_policy/feature_policy.h"
#include "core/dom/ContextLifecycleObserver.h"
#include "core/dom/DOMHighResTimeStamp.h"
#include "core/dom/DOMTimeStamp.h"
#include "core/dom/PausableObject.h"
#include "core/frame/PlatformEventController.h"
#include "modules/EventTargetModules.h"
#include "modules/sensor/SensorOptions.h"
#include "modules/sensor/SensorProxy.h"
#include "modules/sensor/SpatialSensorOptions.h"
#include "platform/WebTaskRunner.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"

namespace blink {

class DOMException;
class ExceptionState;
class ExecutionContext;

class Sensor : public EventTargetWithInlineData,
               public ActiveScriptWrappable<Sensor>,
               public ContextLifecycleObserver,
               public SensorProxy::Observer {
  USING_GARBAGE_COLLECTED_MIXIN(Sensor);
  DEFINE_WRAPPERTYPEINFO();

 public:
  enum class SensorState { kIdle, kActivating, kActivated };

  ~Sensor() override;

  void start();
  void stop();

  // EventTarget overrides.
  const AtomicString& InterfaceName() const override {
    return EventTargetNames::Sensor;
  }
  ExecutionContext* GetExecutionContext() const override {
    return ContextLifecycleObserver::GetExecutionContext();
  }

  // Getters
  bool activated() const;
  bool hasReading() const;
  DOMHighResTimeStamp timestamp(ScriptState*, bool& is_null) const;

  DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(reading);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(activate);

  // ActiveScriptWrappable overrides.
  bool HasPendingActivity() const override;

  virtual void Trace(blink::Visitor*);

 protected:
  Sensor(ExecutionContext*,
         const SensorOptions&,
         ExceptionState&,
         device::mojom::blink::SensorType,
         const Vector<FeaturePolicyFeature>&);

  Sensor(ExecutionContext*,
         const SpatialSensorOptions&,
         ExceptionState&,
         device::mojom::blink::SensorType,
         const Vector<FeaturePolicyFeature>&);

  using SensorConfigurationPtr = device::mojom::blink::SensorConfigurationPtr;
  using SensorConfiguration = device::mojom::blink::SensorConfiguration;

  // The default implementation will init frequency configuration parameter,
  // concrete sensor implementations can override this method to handle other
  // parameters if needed.
  virtual SensorConfigurationPtr CreateSensorConfig();

  bool IsActivated() const { return state_ == SensorState::kActivated; }
  bool IsIdleOrErrored() const;
  const device::SensorReading& GetReading() const;

  // SensorProxy::Observer overrides.
  void OnSensorInitialized() override;
  void OnSensorReadingChanged() override;
  void OnSensorError(ExceptionCode,
                     const String& sanitized_message,
                     const String& unsanitized_message) override;

 private:
  void InitSensorProxyIfNeeded();

  // ContextLifecycleObserver overrides.
  void ContextDestroyed(ExecutionContext*) override;

  void OnAddConfigurationRequestCompleted(bool);

  void Activate();
  void Deactivate();

  void RequestAddConfiguration();

  void HandleError(ExceptionCode = kUnknownError,
                   const String& sanitized_message = String(),
                   const String& unsanitized_message = String());

  void NotifyReading();
  void NotifyActivated();
  void NotifyError(DOMException* error);

 private:
  double frequency_;
  device::mojom::blink::SensorType type_;
  SensorState state_;
  Member<SensorProxy> sensor_proxy_;
  double last_reported_timestamp_;
  SensorConfigurationPtr configuration_;
  TaskHandle pending_reading_notification_;
  TaskHandle pending_activated_notification_;
  TaskHandle pending_error_notification_;
  bool use_screen_coords_ = false;
};

}  // namespace blink

// To be used in getters in concrete sensors
// bindings code.
#define INIT_IS_NULL_AND_RETURN(is_null, x) \
  is_null = !hasReading();                  \
  if (is_null)                              \
  return (x)

#endif  // Sensor_h
