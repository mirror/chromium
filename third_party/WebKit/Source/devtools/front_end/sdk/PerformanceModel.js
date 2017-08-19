// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

SDK.PerformanceModel = class extends SDK.SDKModel {
  /**
   * @param {!SDK.Target} target
   */
  constructor(target) {
    super(target);
    this._enabled = false;
    this._agent = target.performanceAgent();
  }

  enable() {
    this._agent.enable();
  }

  disable() {
    this._agent.disable();
  }

  /**
   * @return {!Promise<!Array<!Protocol.Performance.Metric>>}
   */
  async requestMetrics() {
    return await this._agent.getMetrics() || [];
  }
};

SDK.SDKModel.register(SDK.PerformanceModel, SDK.Target.Capability.Browser, false);
