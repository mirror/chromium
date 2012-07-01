// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// net/tools/testserver/testserver.py is picky about the format of what it
// calls its "echo" messages. One might go so far as to mutter to oneself that
// it isn't an echo server at all.
//
// The response is based on the request but obfuscated using a random key.
const request = "0100000005320000005hello";
var expectedResponsePattern = /0100000005320000005.{11}/;

const socket = chrome.experimental.socket;
var address;
var bytesWritten = 0;
var dataAsString;
var dataRead = [];
var port = -1;
var protocol = "none";
var socketId = 0;
var succeeded = false;
var waitCount = 0;

// Many thanks to Dennis for his StackOverflow answer: http://goo.gl/UDanx
// Since amended to handle BlobBuilder deprecation.
function string2ArrayBuffer(string, callback) {
  var blob = new Blob([string]);
  var f = new FileReader();
  f.onload = function(e) {
    callback(e.target.result);
  };
  f.readAsArrayBuffer(blob);
}

function arrayBuffer2String(buf, callback) {
  var blob = new Blob([new Uint8Array(buf)]);
  var f = new FileReader();
  f.onload = function(e) {
    callback(e.target.result);
  };
  f.readAsText(blob);
}

var testSocketCreation = function() {
  function onCreate(socketInfo) {
    chrome.test.assertTrue(socketInfo.socketId > 0);

    // TODO(miket): this doesn't work yet. It's possible this will become
    // automatic, but either way we can't forget to clean up.
    //
    //socket.destroy(socketInfo.socketId);

    chrome.test.succeed();
  }

  socket.create(protocol, {}, onCreate);
};

function onDataRead(readInfo) {
  if (readInfo.resultCode > 0 || readInfo.data.byteLength > 0) {
    chrome.test.assertEq(readInfo.resultCode, readInfo.data.byteLength);
  }

  arrayBuffer2String(readInfo.data, function(s) {
      dataAsString = s;  // save this for error reporting
      var match = !!s.match(expectedResponsePattern);
      chrome.test.assertTrue(match, "Received data does not match.");
      succeeded = true;
      chrome.test.succeed();
  });
}

function onWriteOrSendToComplete(writeInfo) {
  bytesWritten += writeInfo.bytesWritten;
  if (bytesWritten == request.length) {
    if (protocol == "tcp")
      socket.read(socketId, onDataRead);
    else
      socket.recvFrom(socketId, onDataRead);
  }
}

function onSetKeepAlive(result) {
  if (protocol == "tcp")
    chrome.test.assertTrue(result, "setKeepAlive failed for TCP.");
  else
    chrome.test.assertFalse(result, "setKeepAlive did not fail for UDP.");

  string2ArrayBuffer(request, function(arrayBuffer) {
      if (protocol == "tcp")
        socket.write(socketId, arrayBuffer, onWriteOrSendToComplete);
      else
        socket.sendTo(socketId, arrayBuffer, address, port,
                      onWriteOrSendToComplete);
    });
}

function onSetNoDelay(result) {
  if (protocol == "tcp")
    chrome.test.assertTrue(result, "setNoDelay failed for TCP.");
  else
    chrome.test.assertFalse(result, "setNoDelay did not fail for UDP.");
  socket.setKeepAlive(socketId, true, 1000, onSetKeepAlive);
}

function onConnectOrBindComplete(result) {
  chrome.test.assertEq(0, result,
                       "Connect or bind failed with error " + result);
  if (result == 0) {
    socket.setNoDelay(socketId, true, onSetNoDelay);
  }
}

function onCreate(socketInfo) {
  socketId = socketInfo.socketId;
  chrome.test.assertTrue(socketId > 0, "failed to create socket");
  if (protocol == "tcp")
    socket.connect(socketId, address, port, onConnectOrBindComplete);
  else
    socket.bind(socketId, "0.0.0.0", 0, onConnectOrBindComplete);
}

function waitForBlockingOperation() {
  if (++waitCount < 10) {
    setTimeout(waitForBlockingOperation, 1000);
  } else {
    // We weren't able to succeed in the given time.
    chrome.test.fail("Operations didn't complete after " + waitCount + " " +
                     "seconds. Response so far was <" + dataAsString + ">.");
  }
}

var testSending = function() {
  dataRead = "";
  succeeded = false;
  waitCount = 0;

  setTimeout(waitForBlockingOperation, 1000);
  socket.create(protocol, {}, onCreate);
};

var onMessageReply = function(message) {
  var parts = message.split(":");
  protocol = parts[0];
  address = parts[1];
  port = parseInt(parts[2]);
  console.log("Running tests, protocol " + protocol + ", echo server " +
              address + ":" + port);
  chrome.test.runTests([ testSocketCreation, testSending ]);
};

// Find out which protocol we're supposed to test, and which echo server we
// should be using, then kick off the tests.
chrome.test.sendMessage("info_please", onMessageReply);
