// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

const fs = require('fs');
const path = require('path');

const utils = require('../utils');
/**
 * Move Helper
 * 1) scanHelpers
 * 2) mapToTestRunnerName
 *
 * Transform Helper
 * - Add a comment to existing helper that this is being deprecated
 * - Cop
 *
 * - Create a map of existing indentifiers
 * - Unwrap the initialize_ function
 * - Append methods to existing file
 */

const HELPERS_ROOT_PATH = path.resolve(
  __dirname, '..', '..', '..', '..', 'LayoutTests', 'http', 'tests', 'inspector');


function main() {
  const helperPaths = scanHelpers();
  const helpers = mapTestFilesToHelperModules(helperPaths);

  appendComment(helpers);
  writeNewHelpers(helpers);
}

function appendComment(helpers) {
  for (const {originalPath, originalContents, helperModule, filename} of helpers.values()) {
      const comment = `// This file is being deprecated and is moving to front_end/${helperModule}/${filename}
// Please see crbug.com/667560 for more details\n\n`;
    fs.writeFileSync(originalPath, comment + originalContents);
  }
}

function writeNewHelpers(helpers) {
  for (const {contents, helperModule, filename} of helpers.values()) {
    const modulePath = path.resolve(__dirname, '..', '..', 'front_end', helperModule);
    if (!utils.isDir(modulePath))
      fs.mkdirSync(modulePath);
    const destPath = path.resolve(modulePath, filename);
    const existingContents = utils.isFile(destPath) ? fs.readFileSync(destPath, 'utf-8') : '';
    fs.writeFileSync(destPath, existingContents + '\n' + contents);
  }
}

function mapTestFilesToHelperModules(helperPaths) {
  const helpers = new Map();
  for (const p of helperPaths) {
    let namespacePrefix = path.basename(p).split('-test')[0].split('-').map(a => a.substring(0, 1).toUpperCase() + a.substring(1)).join('');
    let filename = namespacePrefix;

    // Already mgirated or n/a
    if (namespacePrefix === 'Inspector' || namespacePrefix === 'Console' || namespacePrefix === 'Protocol')
      continue;

    // Needs to be manually migrated
    if (namespacePrefix === 'CspInline' || namespacePrefix === 'Stacktrace')
      continue;
    if (namespacePrefix === 'ExtensionsNetwork')
      namespacePrefix = 'Extensions';
    if (namespacePrefix === 'CspInline')
      namespacePrefix = 'CSPInline';
    if (namespacePrefix === 'Debugger')
      namespacePrefix = 'Sources'
    if (namespacePrefix === 'Resources') {
      namespacePrefix = 'Application'
    }
    if (namespacePrefix === 'Appcache')
      namespacePrefix = 'Application'
    if (namespacePrefix === 'ResourceTree')
      namespacePrefix = 'Application'
    if (namespacePrefix === 'ServiceWorkers')
      namespacePrefix = 'Application'
    if (namespacePrefix === 'CacheStorage')
      namespacePrefix = 'Application'
    if (namespacePrefix === 'Indexeddb') {
      namespacePrefix = 'Application'
      filename = 'IndexedDB'
    }
    if (namespacePrefix === 'Timeline')
      namespacePrefix = 'Performance'
    if (namespacePrefix === 'ProductRegistry')
      namespacePrefix = 'Network'
    if (namespacePrefix === 'Search')
      namespacePrefix = 'Sources'
    if (namespacePrefix === 'LiveEdit')
      namespacePrefix = 'Sources'
    if (namespacePrefix === 'Persistence')
      namespacePrefix = 'Bindings'
    if (namespacePrefix === 'IsolatedFilesystem')
      namespacePrefix = 'Bindings'
    if (namespacePrefix === 'Automapping')
      namespacePrefix = 'Bindings'
    const contents = fs.readFileSync(p, 'utf-8');
    const namespace = namespacePrefix + 'TestRunner'
    helpers.set(p, {
      originalPath: p,
      namespace,
      helperModule: namespace.replace(/([A-Z])/g, '_$1').replace(/^_/, '').toLowerCase(),
      filename: filename + 'TestRunner.js',
      originalContents: contents,
      contents,
    });
  }
  return helpers;
}

main();

function scanHelpers() {
  const paths = [];

  scan(HELPERS_ROOT_PATH);

  function scan(p) {
    const files = fs.readdirSync(p).map(file => path.resolve(p, file));
    for (const file of files) {
      if (utils.isDir(file))
        scan(file);
      else if (file.indexOf("-test.js") !== -1)
        paths.push(file);
    }
  }

  return paths;
}