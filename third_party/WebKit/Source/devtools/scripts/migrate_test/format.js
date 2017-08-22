// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var childProcess = require('child_process');
const path = require('path');
const fs = require('fs');

const recast = require('recast');
var Linter = require('eslint').Linter;

const utils = require('../utils');

const TEMP_JS_FILE = path.resolve(__dirname, 'temp.js');
const TEST_FUNCTION_SENTINEL = 'TEST_FUNCTION_SENTINEL();';

function main() {
  // formatFile(`/usr/local/google/home/chenwilliam/chromium/src/third_party/WebKit/LayoutTests/http/tests/inspector-enabled/shadow-dom-rules-restart.html`);
  // return;
  // walkSync('../../../../LayoutTests/http/tests/inspector');
  walkSync('../../../../LayoutTests/http/tests/inspector-enabled');
  walkSync('../../../../LayoutTests/http/tests/inspector-unit');
  walkSync('../../../../LayoutTests/inspector-enabled');
  // walkSync('../../../../LayoutTests/inspector');
  // walkSync('../../../../LayoutTests/inspector/console');
}

function walkSync(currentDirPath) {
  fs.readdirSync(currentDirPath).forEach(function(name) {
    if (name === 'resources') {
      return;
    }
    let filePath = path.join(currentDirPath, name);
    let stat = fs.statSync(filePath);
    if (stat.isFile() && (filePath.endsWith('.html'))) {
      try {
        formatFile(filePath);
      } catch (err) {
        console.log(filePath, 'has err:', err);
      }
    } else if (stat.isDirectory()) {
      walkSync(filePath);
    }
  });
}

function formatFile(filePath) {
  let htmlContents = fs.readFileSync(filePath, 'utf-8');
  const scriptContents = htmlContents.match(/<script.*?>([\S\s]*?)<\/script>/g);

  let functionNode;
  for (const script of scriptContents) {
    const regex = /<script.*?>([\S\s]*?)<\/script>/
    const innerScript = script.match(regex)[1];
    if (!innerScript)
      continue;
    const ast = recast.parse(innerScript);
    const sentinelNode = createExpressionNode(TEST_FUNCTION_SENTINEL);

    let index =
        ast.program.body.findIndex(n => n.type === 'VariableDeclaration' && n.declarations[0].id.name === 'test');
    if (index > -1) {
      functionNode = ast.program.body[index];
      ast.program.body[index] = sentinelNode;
    }

    index = ast.program.body.findIndex(n => n.type === 'FunctionDeclaration' && n.id.name === 'test');
    if (index > -1) {
      functionNode = ast.program.body[index];
      ast.program.body[index] = sentinelNode;
    }
    htmlContents = htmlContents.replace(innerScript, recast.print(ast).code);
  }

  if (!functionNode) {
    console.log('ERROR with: ', filePath);
    return;
  }

  let linter = new Linter();
  let result = linter.verifyAndFix(recast.print(functionNode).code, {
    envs: ['browser'],
    useEslintrc: false,
    parserOptions: {
      ecmaVersion: 8,
    },
    rules: {
      semi: 2,
    }
  });
  if (result.messages.length) {
    console.log('Issue with eslint', result.messages, 'for file: ', filePath);
    process.exit(1);
  }
  fs.writeFileSync(TEMP_JS_FILE, result.output);
  const formattedTestcode = shellOutput(`clang-format ${TEMP_JS_FILE} --style=FILE`);
  fs.unlinkSync(TEMP_JS_FILE);
  const newContents = htmlContents.replace(TEST_FUNCTION_SENTINEL, formattedTestcode);

  fs.writeFileSync(filePath, newContents);
}

main();

function shellOutput(command) {
  return childProcess.execSync(command).toString();
}

function createExpressionNode(code) {
  return recast.parse(code).program.body[0];
}
