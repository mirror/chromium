'use strict';

function assertTestRunner() {
  assert_true(window.testRunner instanceof Object,
    "This test can not be run without the window.testRunner.");
}

function generateMotionData(accelerationX, accelerationY, accelerationZ,
                            accelerationIncludingGravityX,
                            accelerationIncludingGravityY,
                            accelerationIncludingGravityZ,
                            rotationRateAlpha, rotationRateBeta, rotationRateGamma,
                            interval) {
  var motionData = {accelerationX: accelerationX,
                    accelerationY: accelerationY,
                    accelerationZ: accelerationZ,
                    accelerationIncludingGravityX: accelerationIncludingGravityX,
                    accelerationIncludingGravityY: accelerationIncludingGravityY,
                    accelerationIncludingGravityZ: accelerationIncludingGravityZ,
                    rotationRateAlpha: rotationRateAlpha,
                    rotationRateBeta: rotationRateBeta,
                    rotationRateGamma: rotationRateGamma,
                    interval: interval};
  return motionData;
}

function setMockMotion(motionData) {
  testRunner.setMockDeviceMotion(null != motionData.accelerationX,
      null == motionData.accelerationX ? 0 : motionData.accelerationX,
      null != motionData.accelerationY,
      null == motionData.accelerationY ? 0 : motionData.accelerationY,
      null != motionData.accelerationZ,
      null == motionData.accelerationZ ? 0 : motionData.accelerationZ,
      null != motionData.accelerationIncludingGravityX,
      null == motionData.accelerationIncludingGravityX ? 0 : motionData.accelerationIncludingGravityX,
      null != motionData.accelerationIncludingGravityY,
      null == motionData.accelerationIncludingGravityY ? 0 : motionData.accelerationIncludingGravityY,
      null != motionData.accelerationIncludingGravityZ,
      null == motionData.accelerationIncludingGravityZ ? 0 : motionData.accelerationIncludingGravityZ,
      null != motionData.rotationRateAlpha,
      null == motionData.rotationRateAlpha ? 0 : motionData.rotationRateAlpha,
      null != motionData.rotationRateBeta,
      null == motionData.rotationRateBeta ? 0 : motionData.rotationRateBeta,
      null != motionData.rotationRateGamma,
      null == motionData.rotationRateGamma ? 0 : motionData.rotationRateGamma,
      motionData.interval);
}

function checkMotion(event, expectedMotionData) {
  assert_equals(event.acceleration.x, expectedMotionData.accelerationX);
  assert_equals(event.acceleration.y, expectedMotionData.accelerationY);
  assert_equals(event.acceleration.z, expectedMotionData.accelerationZ);

  assert_equals(event.accelerationIncludingGravity.x, expectedMotionData.accelerationIncludingGravityX);
  assert_equals(event.accelerationIncludingGravity.y, expectedMotionData.accelerationIncludingGravityY);
  assert_equals(event.accelerationIncludingGravity.z, expectedMotionData.accelerationIncludingGravityZ);

  assert_equals(event.rotationRate.alpha, expectedMotionData.rotationRateAlpha);
  assert_equals(event.rotationRate.beta, expectedMotionData.rotationRateBeta);
  assert_equals(event.rotationRate.gamma, expectedMotionData.rotationRateGamma);

  assert_equals(event.interval, expectedMotionData.interval);
}
