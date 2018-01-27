// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

const execFile = require('child_process').execFile;

class Git {
  constructor(notify) {
    this._notify = notify;
  }

  show(params) {
    const path = params.path;
    const slash = path.lastIndexOf('/');
    const folder = path.substr(0, slash);
    const file = '.' + path.substr(slash);
    let resolve;
    let reject;
    const result = new Promise((f, r) => { resolve = f; reject = r; });
    execFile('git', ['show', `HEAD:${file}`], {
      cwd: folder, maxBuffer: 1024 * 1024
    }, (error, stdout, stderr) => {
      if (error)
        return reject(stderr);
      resolve(stdout);
    });
    return result;
  }

  dispose() {
    return Promise.resolve({});
  }
};

exports.Git = Git;
