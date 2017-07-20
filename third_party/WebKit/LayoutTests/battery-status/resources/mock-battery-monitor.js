"use strict";

// A helper for forwarding MojoHandle instances from one frame to another.
class CrossFrameHandleProxy {
  constructor(callback) {
    let {handle0, handle1} = Mojo.createMessagePipe();
    this.sender_ = handle0;
    this.receiver_ = handle1;
    this.receiver_.watch({readable: true}, () => {
      var message = this.receiver_.readMessage();
      callback(message.handles[0]);
    });
  }

  forwardHandle(handle) {
    this.sender_.writeMessage(new ArrayBuffer, [handle]);
  }
}

class BatteryMonitorImpl {
  constructor(mock) {
    this.mock_ = mock;
    this.firstQuery_ = true;
  }

  queryNextStatus() {
    // The first call to queryNextStatus() for a connection should return the
    // last available battery status if there is one.
    if (this.firstQuery_ && lastSetMockBatteryInfo) {
      return Promise.resolve({status: lastSetMockBatteryInfo});
    } else {
      return mock_.queryNextStatus();
    }
  }
}

class MockBatteryMonitor {
  constructor() {
    this.pendingRequests_ = [];
    this.status_ = null;
    this.bindingSet_ = new mojo.BindingSet(device.mojom.BatteryMonitor);

    this.interceptor_ = new MojoInterfaceInterceptor(
        device.mojom.BatteryMonitor.name);
    this.interceptor_.oninterfacerequest = e => this.bindRequest(e.handle);
    this.interceptor_.start();
    this.crossFrameHandleProxy_ = new CrossFrameHandleProxy(
        handle => this.bindRequest(handle));
  }

  attachToWindow(otherWindow) {
    otherWindow.batteryMonitorInterceptor =
        new otherWindow.MojoInterfaceInterceptor(
            device.mojom.BatteryMonitor.name);
    otherWindow.batteryMonitorInterceptor.oninterfacerequest =
        e => this.crossFrameHandleProxy_.forwardHandle(e.handle);
    otherWindow.batteryMonitorInterceptor.start();
  }

  bindRequest(handle) {
    let impl = new BatteryMonitorImpl(this);
    this.bindingSet_.addBinding(impl, handle);
  }

  queryNextStatus() {
    let result = new Promise(resolve => this.pendingRequests_.push(resolve));
    this.runCallbacks_();
    return result;
  }

  updateBatteryStatus(status) {
    this.status_ = status;
    this.runCallbacks_();
  }

  runCallbacks_() {
    if (!this.status_ || !this.pendingRequests_.length)
      return;

    while (this.pendingRequests_.length) {
      this.pendingRequests_.pop()({status: this.status_});
    }
    this.status_ = null;
  }
}

let mockBatteryMonitor = new MockBatteryMonitor();

let batteryInfo;
let lastSetMockBatteryInfo;

function setAndFireMockBatteryInfo(charging, chargingTime, dischargingTime,
                                   level) {
  lastSetMockBatteryInfo = {
    charging: charging,
    chargingTime: chargingTime,
    dischargingTime: dischargingTime,
    level: level
  };
  mockBatteryMonitor.updateBatteryStatus(lastSetMockBatteryInfo);
}

// compare obtained battery values with the mock values
function testIfBatteryStatusIsUpToDate(batteryManager) {
  batteryInfo = batteryManager;
  shouldBeDefined("batteryInfo");
  shouldBeDefined("lastSetMockBatteryInfo");
  shouldBe('batteryInfo.charging', 'lastSetMockBatteryInfo.charging');
  shouldBe('batteryInfo.chargingTime', 'lastSetMockBatteryInfo.chargingTime');
  shouldBe('batteryInfo.dischargingTime',
           'lastSetMockBatteryInfo.dischargingTime');
  shouldBe('batteryInfo.level', 'lastSetMockBatteryInfo.level');
}

function batteryStatusFailure() {
  testFailed('failed to successfully resolve the promise');
  setTimeout(finishJSTest, 0);
}
