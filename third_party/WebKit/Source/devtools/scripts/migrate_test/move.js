// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

const fs = require('fs');
const path = require('path');

const mkdirp = require('mkdirp');

const migrateUtils = require('./migrate_utils');
const utils = require('../utils');

const ORIGINAL_TESTS_PATH = path.resolve(__dirname, 'original_tests.txt');
const MOVED_TESTS_PATH = path.resolve(__dirname, 'moved_tests.txt');
const TEST_EXPECTATIONS_PATH = path.resolve(__dirname, '..', '..', '..', '..', 'LayoutTests', 'TestExpectations');
const FLAG_EXPECTATIONS_PATH = path.resolve(__dirname, '..', '..', '..', '..', 'LayoutTests', 'FlagExpectations');

function main() {
  const originalTests = fs.readFileSync(ORIGINAL_TESTS_PATH, 'utf-8').split('\n');
  const oldToNewResourcesPath = new Map();
  const oldToNewTestPath = new Map();

  for (const inputRelativePath of originalTests) {
    if (!inputRelativePath) {
      continue;
    }
    const inputPath = path.resolve(__dirname, '..', '..', '..', '..', 'LayoutTests', inputRelativePath);
    const inputResourcesPath = path.resolve(path.dirname(inputPath), 'resources');
    const outPath = migrateUtils.getOutPath(inputPath);
    const outResourcesPath = path.resolve(path.dirname(outPath), 'resources');
    if (utils.isDir(inputResourcesPath))
      oldToNewResourcesPath.set(inputResourcesPath, outResourcesPath);
    mkdirp.sync(path.dirname(outPath));

    // Move .html -> .js
    fs.writeFileSync(outPath, fs.readFileSync(inputPath, 'utf-8'));
    fs.unlinkSync(inputPath);

    const outRelativePath = outPath.substring(outPath.indexOf('http/'));
    oldToNewTestPath.set(inputRelativePath, outRelativePath);

    // Move expectation file
    const inputExpectationsPath = inputPath.replace('.html', '-expected.txt');
    const outExpectationsPath = outPath.replace('.js', '-expected.txt');
    fs.writeFileSync(outExpectationsPath, fs.readFileSync(inputExpectationsPath, 'utf-8'));
    fs.unlinkSync(inputExpectationsPath);
  }

  fs.writeFileSync(MOVED_TESTS_PATH, Array.from(oldToNewTestPath.values()).join('\n'));

  // Update TestExpectations
  const testExpectations = fs.readFileSync(TEST_EXPECTATIONS_PATH, 'utf-8');
  const updatedTestExpecations = testExpectations.split('\n').map(line => {
    for (const testPath of oldToNewTestPath.keys()) {
      if (line.indexOf(testPath) !== -1) {
        return line.replace(testPath, oldToNewTestPath.get(testPath));
      }
      if (line === '# See crbug.com/667560 for details') {
        return line + '\n' +
            Array.from(oldToNewTestPath.values()).map(x => `crbug.com/667560 ${x} [ Skip ]`).join('\n');
      }
      if (line === '### virtual/mojo-loading/http/tests/devtools') {
        return line + '\n' +
            Array.from(oldToNewTestPath.values())
                .map(x => `crbug.com/667560 virtual/mojo-loading/${x} [ Skip ]`)
                .join('\n');
      }
    }
    return line;
  });
  fs.writeFileSync(TEST_EXPECTATIONS_PATH, updatedTestExpecations.join('\n'));

  // Update FlagExpectations
  for (const folder of fs.readdirSync(FLAG_EXPECTATIONS_PATH)) {
    const flagFilePath = path.resolve(FLAG_EXPECTATIONS_PATH, folder);
    const expectations = fs.readFileSync(flagFilePath, 'utf-8');
    const updatedExpectations = expectations.split('\n').map(line => {
      for (const testPath of oldToNewTestPath.keys()) {
        if (line.indexOf(testPath) !== -1) {
          return line.replace(testPath, oldToNewTestPath.get(testPath));
        }
      }
      return line;
    });
    fs.writeFileSync(flagFilePath, updatedExpectations.join('\n'));
  }

  for (const [oldResourcesPath, newResourcesPath] of oldToNewResourcesPath)
    utils.copyRecursive(oldResourcesPath, path.dirname(newResourcesPath));
}

main();
