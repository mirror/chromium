// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Implementation of functions that are shared between ReadableStream and
// WritableStream.

(function(global, binding, v8) {
  'use strict';

  const hasOwnProperty = v8.uncurryThis(global.Object.hasOwnProperty);

  function hasOwnPropertyNoThrow(x, property) {
    // The cast of |x| to Boolean will eliminate undefined and null, which would
    // cause hasOwnProperty to throw a TypeError, as well as some other values
    // that can't be objects and so will fail the check anyway.
    return Boolean(x) && hasOwnProperty(x, property);
  }

  binding.streamOperations = { hasOwnPropertyNoThrow };
});
