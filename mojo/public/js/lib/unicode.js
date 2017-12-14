// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Defines functions for translating between JavaScript strings and UTF8 strings
 * stored in ArrayBuffers. There is much room for optimization in this code if
 * it proves necessary.
 */
(function() {
  var internal = mojo.internal;

  var decoder = new TextDecoder();
  var encoder = new TextEncoder();

  /**
   * Decodes the UTF8 string from the given buffer.
   * @param {ArrayBufferView} buffer The buffer containing UTF8 string data.
   * @return {string} The corresponding JavaScript string.
   */
  function decodeUtf8String(buffer) {
    return decoder.decode(buffer);
  }

  /**
   * Encodes the given JavaScript string into UTF8.
   * @param {string} str The string to encode.
   * @param {ArrayBufferView} outputBuffer The buffer to contain the result.
   * Should be pre-allocated to hold enough space. Use |utf8Length| to determine
   * how much space is required.
   * @return {number} The number of bytes written to |outputBuffer|.
   */
  function encodeUtf8String(str, outputBuffer) {
    var buffer = encoder.encode(str);
    outputBuffer.set(buffer);
    return buffer.byteLength;
  }

  /**
   * Returns the number of bytes that a UTF8 encoding of the JavaScript string
   * |str| would occupy.
   */
  function utf8Length(str) {
    return encoder.encode(str).byteLength;
  }

  internal.decodeUtf8String = decodeUtf8String;
  internal.encodeUtf8String = encodeUtf8String;
  internal.utf8Length = utf8Length;
})();
