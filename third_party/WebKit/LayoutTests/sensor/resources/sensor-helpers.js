'use strict';

// Wraps callback and calls reject_func if callback throws an error.
class CallbackWrapper {
  constructor(callback, reject_func) {
    this.wrapper_func_ = (args) => {
      try {
        callback(args);
      } catch(e) {
        reject_func(e);
      }
    }
  }

  get callback() {
    return this.wrapper_func_;
  }
}

function sensor_mocks() {
  // Helper function that returns resolved promise with result.
  function sensorResponse(success) {
    return Promise.resolve({success});
  }

  // Class that mocks Sensor interface defined in sensor.mojom
  class MockSensor {
    constructor(sensorRequest, handle, offset, size, reportingMode) {
      this.client_ = null;
      this.start_should_fail_ = false;
      this.reporting_mode_ = reportingMode;
      this.sensor_reading_timer_id_ = null;
      this.update_reading_function_ = null;
      this.reading_updates_count_ = 0;
      this.suspend_called_ = null;
      this.resume_called_ = null;
      this.add_configuration_called_ = null;
      this.remove_configuration_called_ = null;
      this.active_sensor_configurations_ = [];
      let rv = handle.mapBuffer(offset, size);
      assert_equals(rv.result, Mojo.RESULT_OK, "Failed to map shared buffer");
      this.buffer_array_ = rv.buffer;
      this.buffer_ = new Float64Array(this.buffer_array_);
      this.resetBuffer();
      this.binding_ = new mojo.Binding(device.mojom.Sensor, this,
                                           sensorRequest);
      this.binding_.setConnectionErrorHandler(() => {
        this.reset();
      });
    }

    // Returns default configuration.
    getDefaultConfiguration() {
      return Promise.resolve({frequency: 5});
    }

    reading_updates_count() {
      return this.reading_updates_count_;
    }
    // Adds configuration for the sensor and starts reporting fake data
    // through update_reading_function_ callback.
    addConfiguration(configuration) {
      assert_not_equals(configuration, null, "Invalid sensor configuration.");

      this.active_sensor_configurations_.push(configuration);
      // Sort using descending order.
      this.active_sensor_configurations_.sort(
          (first, second) => { return second.frequency - first.frequency });

      if (!this.start_should_fail_ )
        this.startReading();

      if (this.add_configuration_called_ != null)
        this.add_configuration_called_(this);

      return sensorResponse(!this.start_should_fail_);
    }

    // Removes sensor configuration from the list of active configurations and
    // stops notification about sensor reading changes if
    // active_sensor_configurations_ is empty.
    removeConfiguration(configuration) {
      if (this.remove_configuration_called_ != null) {
        this.remove_configuration_called_(this);
      }

      let index = this.active_sensor_configurations_.indexOf(configuration);
      if (index !== -1) {
        this.active_sensor_configurations_.splice(index, 1);
      } else {
        return sensorResponse(false);
      }

      if (this.active_sensor_configurations_.length === 0)
        this.stopReading();

      return sensorResponse(true);
    }

    // Suspends sensor.
    suspend() {
      this.stopReading();
      if (this.suspend_called_ != null) {
        this.suspend_called_(this);
      }
    }

    // Resumes sensor.
    resume() {
      assert_equals(this.sensor_reading_timer_id_, null);
      this.startReading();
      if (this.resume_called_ != null) {
        this.resume_called_(this);
      }
    }

    // Mock functions

    // Resets mock Sensor state.
    reset() {
      this.stopReading();

      this.reading_updates_count_ = 0;
      this.start_should_fail_ = false;
      this.update_reading_function_ = null;
      this.active_sensor_configurations_ = [];
      this.suspend_called_ = null;
      this.resume_called_ = null;
      this.add_configuration_called_ = null;
      this.remove_configuration_called_ = null;
      this.resetBuffer();
      this.buffer_array_ = null;
      this.binding_.close();
    }

    // Zeroes shared buffer.
    resetBuffer() {
      for (let i = 0; i < this.buffer_.length; ++i) {
        this.buffer_[i] = 0;
      }
    }

    // Sets callback that is used to deliver sensor reading updates.
    setUpdateSensorReadingFunction(update_reading_function) {
      this.update_reading_function_ = update_reading_function;
      return Promise.resolve(this);
    }

    // Sets flag that forces sensor to fail when addConfiguration is invoked.
    setStartShouldFail(should_fail) {
      this.start_should_fail_ = should_fail;
    }

    // Returns resolved promise if suspend() was called, rejected otherwise.
    suspendCalled() {
      return new Promise((resolve, reject) => {
        this.suspend_called_ = resolve;
      });
    }

    // Returns resolved promise if resume() was called, rejected otherwise.
    resumeCalled() {
      return new Promise((resolve, reject) => {
        this.resume_called_ = resolve;
      });
    }

    // Resolves promise when addConfiguration() is called.
    addConfigurationCalled() {
      return new Promise((resolve, reject) => {
        this.add_configuration_called_ = resolve;
      });
    }

    // Resolves promise when removeConfiguration() is called.
    removeConfigurationCalled() {
      return new Promise((resolve, reject) => {
        this.remove_configuration_called_ = resolve;
      });
    }

    startReading() {
      if (this.update_reading_function_ != null) {
        this.stopReading();
        let max_frequency_used =
            this.active_sensor_configurations_[0].frequency;
        let timeout = (1 / max_frequency_used) * 1000;
        this.sensor_reading_timer_id_ = window.setInterval(() => {
          if (this.update_reading_function_) {
            this.update_reading_function_(this.buffer_);
            // For all tests sensor reading should have monotonically
            // increasing timestamp in seconds.
            this.buffer_[1] = window.performance.now() * 0.001;
            this.reading_updates_count_++;
          }
          if (this.reporting_mode_ === device.mojom.ReportingMode.ON_CHANGE) {
            this.client_.sensorReadingChanged();
          }
        }, timeout);
      }
    }

    stopReading() {
      if (this.sensor_reading_timer_id_ != null) {
        window.clearInterval(this.sensor_reading_timer_id_);
        this.sensor_reading_timer_id_ = null;
      }
    }

  }

  // Helper function that returns resolved promise for getSensor() function.
  function getSensorResponse(initParams, clientRequest) {
    return Promise.resolve({initParams, clientRequest});
  }

  // Class that mocks SensorProvider interface defined in
  // sensor_provider.mojom
  class MockSensorProvider {
    constructor() {
      this.reading_size_in_bytes_ =
          device.mojom.SensorInitParams.kReadBufferSizeForTests;
      this.shared_buffer_size_in_bytes_ = this.reading_size_in_bytes_ *
              device.mojom.SensorType.LAST;
      let rv = Mojo.createSharedBuffer(this.shared_buffer_size_in_bytes_);
      assert_equals(rv.result, Mojo.RESULT_OK, "Failed to create buffer");
      this.shared_buffer_handle_ = rv.handle;
      this.active_sensor_ = null;
      this.get_sensor_should_fail_ = false;
      this.resolve_func_ = null;
      this.is_continuous_ = false;
      this.max_frequency_ = 60;
      this.min_frequency_ = 1;
      this.binding_ = new mojo.Binding(device.mojom.SensorProvider, this);

      this.interceptor_ = new MojoInterfaceInterceptor(
          device.mojom.SensorProvider.name);
      this.interceptor_.oninterfacerequest = e => {
        this.bindToPipe(e.handle);
      };
      this.interceptor_.start();
    }

    // Returns initialized Sensor proxy to the client.
    getSensor(type, request) {
      if (this.get_sensor_should_fail_) {
        var ignored = new device.mojom.SensorClientPtr();
        return getSensorResponse(null, mojo.makeRequest(ignored));
      }

      let offset = (device.mojom.SensorType.LAST - type) *
          this.reading_size_in_bytes_;
      let reporting_mode = device.mojom.ReportingMode.ON_CHANGE;
      if (this.is_continuous_) {
          reporting_mode = device.mojom.ReportingMode.CONTINUOUS;
      }

      if (this.active_sensor_ == null) {
        let mockSensor = new MockSensor(request, this.shared_buffer_handle_,
            offset, this.reading_size_in_bytes_, reporting_mode);
        this.active_sensor_ = mockSensor;
      }

      let rv = this.shared_buffer_handle_.duplicateBufferHandle();

      assert_equals(rv.result, Mojo.RESULT_OK);

      let default_config = {frequency: 5};

      let init_params =
          new device.mojom.SensorInitParams(
              { memory: rv.handle,
                bufferOffset: offset,
                mode: reporting_mode,
                defaultConfiguration: default_config,
                minimumFrequency: this.min_frequency_,
                maximumFrequency: this.max_frequency_});

      if (this.resolve_func_ !== null) {
        this.resolve_func_(this.active_sensor_);
      }

      this.active_sensor_.client_ = new device.mojom.SensorClientPtr();
      return getSensorResponse(
          init_params, mojo.makeRequest(this.active_sensor_.client_));
    }

    // Binds object to mojo message pipe
    bindToPipe(pipe) {
      this.binding_.bind(pipe);
      this.binding_.setConnectionErrorHandler(() => {
        this.reset();
      });
    }

    // Mock functions

    // Resets state of mock SensorProvider between test runs.
    reset() {
      if (this.active_sensor_ != null) {
        this.active_sensor_.reset();
        this.active_sensor_ = null;
      }

      this.get_sensor_should_fail_ = false;
      this.resolve_func_ = null;
      this.max_frequency_ = 60;
      this.min_frequency_ = 1;
      this.is_continuous_ = false;
      this.binding_.close();
      this.interceptor_.stop();
    }

    // Sets flag that forces mock SensorProvider to fail when getSensor() is
    // invoked.
    setGetSensorShouldFail(should_fail) {
      this.get_sensor_should_fail_ = should_fail;
    }

    // Returns mock sensor that was created in getSensor to the layout test.
    getCreatedSensor() {
      if (this.active_sensor_ != null) {
        return Promise.resolve(this.active_sensor_);
      }

      return new Promise((resolve, reject) => {
        this.resolve_func_ = resolve;
       });
    }

    // Forces sensor to use |reporting_mode| as an update mode.
    setContinuousReportingMode() {
        this.is_continuous_ = true;
    }

    // Sets the maximum frequency for a concrete sensor.
    setMaximumSupportedFrequency(frequency) {
        this.max_frequency_ = frequency;
    }

    // Sets the minimum frequency for a concrete sensor.
    setMinimumSupportedFrequency(frequency) {
        this.min_frequency_ = frequency;
    }
  }

  let mockSensorProvider = new MockSensorProvider;
  return {mockSensorProvider: mockSensorProvider};
}

function sensor_test(func, name, properties) {
  mojo_test(() => {
    let sensor = sensor_mocks();

    // Clean up and reset mock sensor stubs asynchronously, so that the blink
    // side closes its proxies and notifies JS sensor objects before new test is
    // started.
    let onSuccess = () => {
      sensor.mockSensorProvider.reset();
      return new Promise((resolve, reject) => { setTimeout(resolve, 0); });
    };

    let onFailure = error => {
      sensor.mockSensorProvider.reset();
      return new Promise((resolve, reject) => { setTimeout(() => {reject(error);}, 0); });
    };

    return Promise.resolve(func(sensor)).then(onSuccess, onFailure);
  }, name, properties);
}
