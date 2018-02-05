// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

if ((typeof mojo === 'undefined') || !mojo.bindingsLibraryInitialized) {
  loadScript('mojo_bindings');
}
mojo.config.autoLoadMojomDeps = false;

loadScript('media/mojo/interfaces/mirror_service.mojom');

/**
 * Creates a new MirrorService.
 * @param {!media.mojom.MirrorServiceProxyPtr} service
 * @constructor
 */
function MirrorService(service) {
  /**
   * The Mojo service proxy. Allows extension code to connect to the mirror
   * service.
   * @type {!media.mojom.MirrorServiceProxyPtr}
   */
  this.service_ = service;
}

/**
 * Returns definitions of Mojo core and generated Mojom classes that can be
 * used directly by the component.
 * @return {!Object}
 */
MirrorService.prototype.getMojoExports = function() {
  return {
    MirrorServiceProxyPtr: media.mojom.MirrorServiceProxyPtr,
    MirrorClient: media.mojom.MirrorClient,
    MirrorClientPtr: media.mojom.MirrorClientPtr,
    SenderConfig: media.mojom.SenderConfig,
    CaptureType: media.mojom.CaptureType,
    CaptureParam: media.mojom.CaptureParam,
    UdpOptions: media.mojom.UdpOptions,
    StreamingParams: media.mojom.StreamingParams,
    MirrorError: media.mojom.MirrorError,
  };
};

MirrorService.prototype.getSupportedSenderConfigs = function(
    hasAudio, hasVideo) {
  return this.service_.getSupportedSenderConfigs(hasAudio, hasVideo);
};

MirrorService.prototype.start = function(capture_param, udp_param,
    audio_config, video_config, streaming_param, client) {
  return this.service_.start(capture_param, udp_param, audio_config,
                             video_config, streaming_param, client);
};

MirrorService.prototype.stop = function() {
  this.service_.stop();
};

MirrorService.prototype.shutdown = function() {
  this.service_.shutdown();
}

var ptr = new media.mojom.MirrorServiceProxyPtr;
Mojo.bindInterface(media.mojom.MirrorServiceProxy.name,
                   mojo.makeRequest(ptr).handle);
exports.$set('returnValue', new MirrorService(ptr));

