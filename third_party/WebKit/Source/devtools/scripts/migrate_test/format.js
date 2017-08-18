// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

const path = require('path');
const fs = require('fs');
const cheerio = require('cheerio');
const prettier = require('prettier');

const utils = require('../utils');

function main() {
  // walkSync('../../../../LayoutTests/http/tests/inspector', formatFile);
  walkSync('../../../../LayoutTests/http/tests/inspector-enabled', formatFile);
  walkSync('../../../../LayoutTests/http/tests/inspector-unit', formatFile);
  walkSync('../../../../LayoutTests/inspector-enabled', formatFile);
  // walkSync('../../../../LayoutTests/inspector', formatFile);
  walkSync('../../../../LayoutTests/inspector/console', formatFile);
}

function walkSync(currentDirPath, process) {
  fs.readdirSync(currentDirPath).forEach(function(name) {
    if (name === 'resources') {
      return;
    }
    let filePath = path.join(currentDirPath, name);
    let stat = fs.statSync(filePath);
    if (stat.isFile() && (filePath.endsWith('.html'))) {
      try {
        process(filePath);
      } catch (err) {
        console.log(filePath, 'has err:', err);
      }
    } else if (stat.isDirectory()) {
      walkSync(filePath, process);
    }
  });
}

function formatFile(filePath) {
  const regex = /<script>([\S\s]*?)<\/script>/;
  let scriptCount = 0;
  const scripts = [];

  let htmlContents = fs.readFileSync(filePath, 'utf-8');

  while (htmlContents.match(regex)) {
    extractScriptContents();
  }

  for (var i = 0; i < scripts.length; i++) {
    prettyPrint(i);
  }

  fs.writeFileSync(filePath, htmlContents);

  function extractScriptContents() {
    scripts.push(htmlContents.match(regex)[1]);
    htmlContents = htmlContents.replace(regex, sentinel(scriptCount));
    scriptCount++;
  }

  function prettyPrint(i) {
    const formatted = prettier
                          .format(scripts[i], {tabWidth: 2, printWidth: 120, singleQuote: true})
                          // Need to escape $ symbol
                          .split('$')
                          .join('$$');

    htmlContents = htmlContents.replace(sentinel(i), '<script>\n' + formatted + '</script>');
  }

  function sentinel(n) {
    return '@@SUPER_SECRET_SCRIPT_' + n + '@@';
  }
}

main();
