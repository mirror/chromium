// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Utilities for file operations.
 */
var fileOperationUtils = {};

/**
 * Deduplicates file name inside |rootDirectory| directory.
 * @param {string} fileName The suggested file name.
 * @param {!DirectoryEntry} rootDirectory The root directory where new file with
 *     |fileName| will be created.
 */
fileOperationUtils.deduplicateFileName = function(fileName, rootDirectory) {
  // Split the name into three part. The parenthesized number (if exists) will
  // be replaced by incremented number for retry. For example, suppose
  // |fileName| is "file (10).txt", the second check path will be
  // "file (11).txt".
  var match = /^(.*?)(?: \((\d+)\))?(\.[^.]*?)?$/.exec(fileName);
  var prefix = match[1];
  var parenthesizedNumber = parseInt(match[2] || 0);
  var ext = match[3] || '';

  var getFileName = function(filePrefix, fileNumber, fileExtension) {
    var newName = filePrefix;
    if (fileNumber > 0) {
      newName += ' (' + fileNumber + ')';
    }
    newName += fileExtension;

    return new Promise(rootDirectory.getFile.bind(
                           rootDirectory, newName, {create: false}))
        .then(function() {
          return getFileName(filePrefix, fileNumber + 1, fileExtension);
        })
        .catch(function(error) {
          if (error.code == error.NOT_FOUND_ERR) {
            return Promise.resolve(newName);
          } else {
            return Promise.reject(error);
          }
        });
  };

  return getFileName(prefix, parenthesizedNumber, ext);
};