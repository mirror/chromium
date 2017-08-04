// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

const fs = require('fs');
const path = require('path');

const recast = require('recast');
const types = recast.types;
const b = recast.types.builders;

const utils = require('../utils');
/**
 * - Append methods to existing file
 *
 * propertyToNamespace {[testProperty:string]: {namespace: string, done: boolean}}
 * Moved file
 * {ETR.something: ETR}
 * Existing helper module
 * {ConsoleTestRunner.something: CTR}
 *
 * Create the test module
 * * Update BUILD.gn
 * * Update module.json
 * * Update integration_test_runner
 */
const BUILD_GN_PATH = path.resolve(__dirname, '..', '..', 'BUILD.gn');
const DRY_RUN = process.env.DRY_RUN || false;
const HELPERS_ROOT_PATH = path.resolve(__dirname, '..', '..', '..', '..', 'LayoutTests', 'http', 'tests', 'inspector');
const FRONT_END_PATH = path.resolve(__dirname, '..', '..', 'front_end');


function main() {
  const helperPaths = scanHelpers();
  const helpers = mapTestFilesToHelperModules(helperPaths);
  transformHelperCode(helpers);
  if (DRY_RUN) {
    return;
  }
  updateModuleDescriptors(helpers);
  const newModules = new Set();
  const helperFiles = [];
  for (const h of helpers) {
    newModules.add(h.helperModule);
    helperFiles.push(h.newPath);
  }
  updateApplicationDescriptor('integration_test_runner.json', newModules);
  updateBuildGN(newModules, helperFiles);
  appendComment(helpers);
  writeNewHelpers(helpers);
}

function updateBuildGN(newModuleSet, helperFiles) {
  let content = fs.readFileSync(BUILD_GN_PATH).toString();
  helperFiles =
      helperFiles.map(p => p.split('/').slice(-3).join('/')).filter(p => content.indexOf(p) === -1).map(p => `"${p}",`);

  let newContent = addContentToLinesInSortedOrder({
    content,
    startLine: 'all_devtools_files = [',
    endLine: ']',
    linesToInsert:
        [...newModuleSet].map(module => `"front_end/${module}/module.json",`).filter(p => content.indexOf(p) === -1),
  });

  newContent = addContentToLinesInSortedOrder({
    content: newContent,
    startLine: 'all_devtools_files = [',
    endLine: ']',
    linesToInsert: helperFiles,
  });

  fs.writeFileSync(BUILD_GN_PATH, newContent);

  function top(array) {
    return array[array.length - 1];
  }

  function addContentToLinesInSortedOrder({content, startLine, endLine, linesToInsert}) {
    if (linesToInsert.length === 0)
      return content;
    let lines = content.split('\n');
    let seenStartLine = false;
    let contentStack = linesToInsert.sort((a, b) => a.toLowerCase().localeCompare(b.toLowerCase())).reverse();
    for (var i = 0; i < lines.length; i++) {
      let line = lines[i].trim();
      let nextLine = lines[i + 1].trim();
      if (line === startLine)
        seenStartLine = true;

      if (line === endLine && seenStartLine)
        break;

      if (!seenStartLine)
        continue;

      const nextContent = top(contentStack) ? top(contentStack).toLowerCase() : '';
      if ((line === startLine || nextContent >= line.toLowerCase()) &&
          (nextLine === endLine || nextContent <= nextLine.toLowerCase()))
        lines.splice(i + 1, 0, contentStack.pop());
    }
    if (contentStack.length)
      lines.splice(i, 0, ...contentStack);
    return lines.join('\n');
  }
}

function updateApplicationDescriptor(descriptorFileName, newModuleSet) {
  let descriptorPath = path.join(FRONT_END_PATH, descriptorFileName);
  let newModules = [...newModuleSet];
  if (newModules.length === 0)
    return;
  let includeNewModules = (acc, line) => {
    if (line === '  "modules" : [') {
      acc.push(line);
      return acc.concat(newModules.map((m, i) => {
        // Need spacing to preserve indentation
        let string;
        string = `    { "name": "${m}" },`;
        return string;
      }));
    }
    return acc.concat([line]);
  };
  let lines = fs.readFileSync(descriptorPath).toString().split('\n').reduce(includeNewModules, []);
  fs.writeFileSync(descriptorPath, lines.join('\n'));
}

function updateModuleDescriptors(helpers) {
  const dependenciesByModule = {
    elements_test_runner: [],
  };

  for (const helper of helpers) {
    const parentPath = path.dirname(helper.newPath);
    if (!utils.isDir(parentPath))
      fs.mkdirSync(parentPath);
    const modulePath = path.resolve(parentPath, 'module.json');
    const additionalDependencies = dependenciesByModule[helper.helperModule] || [];
    let contents = {
      dependencies: ['test_runner', 'integration_test_runner'].concat(additionalDependencies),
      scripts: [],
    };
    if (utils.isFile(helper.newPath)) {
      contents = JSON.parse(fs.readFileSync(modulePath, 'utf-8'));
    }
    const filename = path.basename(helper.newPath);
    if (contents.scripts.indexOf(filename) === -1)
      contents.scripts.push(filename);
    contents.skip_compilation = contents.skip_compilation || [];
    contents.skip_compilation.push(filename);
    fs.writeFileSync(modulePath, stringifyJSON(contents));
  }

  function stringifyJSON(obj) {
    return unicodeEscape(JSON.stringify(obj, null, 2) + '\n');
  }

  // http://stackoverflow.com/questions/7499473/need-to-escape-non-ascii-characters-in-javascript
  function unicodeEscape(string) {
    function padWithLeadingZeros(string) {
      return new Array(5 - string.length).join('0') + string;
    }

    function unicodeCharEscape(charCode) {
      return '\\u' + padWithLeadingZeros(charCode.toString(16));
    }

    return string.split('')
        .map(function(char) {
          var charCode = char.charCodeAt(0);
          return charCode > 127 ? unicodeCharEscape(charCode) : char;
        })
        .join('');
  }
}

function transformHelperCode(helpers) {
  const boilerplate = `// Copyright 2017 The Chromium Authors. All
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview using private properties isn't a Closure violation in tests.
 * @suppress {accessControls}
 */
`;

  const propertyToNamespace = scrapeTestProperties(helpers);

  for (const helper of helpers) {
    // TESTING
    // if (helper.newPath.indexOf('ElementsTestRunner') === -1)
    //   continue;
    const ast = recast.parse(helper.originalContents);
    unwrapInitializeFunction(ast);

    recast.visit(ast, {
      /**
     * Remove all deprecated API calls
     */
      visitIdentifier: function(path) {
        if (path.parentPath && path.parentPath.value && path.parentPath.value.object &&
            path.parentPath.value.object.name === 'InspectorTest' && path.value.name !== 'InspectorTest') {
          const identifier = path.value.name;
          if (identifier === 'preloadPanel' || identifier === 'preloadModule') {
            //  Removes these types of calls: InspectorTest.preloadPanel("elements");
            path.parentPath.parentPath.prune();
          }
        }
        return false;
      },
      /**
     * Skip already ported API calls
     */
      visitAssignmentExpression: function(path) {
        if (!path.value.left.object) {
          return false;
        }
        const namespace = path.value.left.object.name;
        const propertyName = path.value.left.property.name;
        if (namespace === 'InspectorTest' && propertyToNamespace.get(propertyName).done) {
          console.log('pruning', propertyName);
          path.prune();
        }
        this.traverse(path);
      },
    });

    /**
   * Migrate all the call sites from InspectorTest to .*TestRunner
   */
    recast.visit(ast, {
      visitIdentifier: function(path) {
        if (path.parentPath && path.parentPath.value && path.parentPath.value.object &&
            path.parentPath.value.object.name === 'InspectorTest' && path.value.name !== 'InspectorTest') {
          if (!propertyToNamespace.get(path.value.name)) {
            debugger;
            throw new Error('Could not find identifier for: ' + path.value.name);
          }
          const newParentIdentifier = propertyToNamespace.get(path.value.name).namespace;
          path.parentPath.value.object.name = newParentIdentifier;
        }
        return false;
      }
    });

    const existingContents = utils.isFile(helper.newPath) ? fs.readFileSync(helper.newPath) : '';
    if (existingContents)
      helper.contents = existingContents + '\n' + print(ast);
    else
      helper.contents = boilerplate + print(ast);
  }

  function unwrapInitializeFunction(ast) {
    // Handle function expression
    let index = ast.program.body.findIndex(
        n => n.type === 'VariableDeclaration' && n.declarations[0].id.name.indexOf('initialize_') !== -1);
    if (index > -1) {
      const otherNodes = ast.program.body.filter((_, i) => i !== index);
      const testFunctionNode = ast.program.body[index];
      ast.program.body = testFunctionNode.declarations[0].init.body.body;
      inspectedPageNodes(otherNodes);
      return;
    }

    // Handle function declaration
    index =
        ast.program.body.findIndex(n => n.type === 'FunctionDeclaration' && n.id.name.indexOf('initialize_') !== -1);
    if (index > -1) {
      const otherNodes = ast.program.body.filter((_, i) => i !== index);
      const testFunctionNode = ast.program.body[index];
      ast.program.body.splice(index, 1);
      ast.program.body = testFunctionNode.body.body;
      inspectedPageNodes(otherNodes);
    }

    function inspectedPageNodes(otherNodes) {
      if (!otherNodes.length)
        return;
      const code = otherNodes.map(node => print(node)).join('\n').slice(0, -1);
      ast.program.body.push(createAwaitExpressionNode(`await TestRunner.evaluateInPagePromise(\`
${code.split('\n').map(x => x.length ? '    ' + x : x).join('\n')}
  \`)`));
    }
  }
}

function print(ast) {
  /**
   * Not using clang-format because certain tests look bad when formatted by it.
   * Recast pretty print is smarter about preserving existing spacing.
   */
  let code = recast.prettyPrint(ast, {tabWidth: 2, wrapColumn: 120, quote: 'single'}).code;
  code = code.replace(/(\/\/\#\s*sourceURL=[\w-]+)\.html/, '$1.js');
  code = code.replace(/\s*\$\$SECRET_IDENTIFIER_FOR_LINE_BREAK\$\$\(\);/g, '\n');
  return code + '\n';
}

function scrapeTestProperties(helpers) {
  const propertyToNamespace = new Map();
  scrapeOriginalHelpers(propertyToNamespace, helpers);
  scrapeMovedHelpers(propertyToNamespace);

  // Manual overrides
  propertyToNamespace.set('consoleModel', {
    namespace: 'ConsoleModel',
    done: true,
  });
  propertyToNamespace.set('networkLog', {
    namespace: 'NetworkLog',
    done: true,
  });
  return propertyToNamespace;
}

function scrapeMovedHelpers(propertyToNamespace) {
  const testRunnerPaths = fs.readdirSync(FRONT_END_PATH)
                              .filter(folder => folder.indexOf('test_runner') !== -1)
                              .map(folder => path.resolve(FRONT_END_PATH, folder))
                              .filter(file => utils.isDir(file));

  testRunnerPaths.forEach((helperPath) => {
    const files = fs.readdirSync(helperPath)
                      .filter(file => file.indexOf('TestRunner') !== -1)
                      .map(file => path.resolve(helperPath, file));
    files.forEach(file => scrapeTestHelperIdentifiers(file));
  });

  function scrapeTestHelperIdentifiers(filePath) {
    var content = fs.readFileSync(filePath).toString();
    var lines = content.split('\n');
    for (var line of lines) {
      var line = line.trim();
      if (line.indexOf('TestRunner.') === -1)
        continue;
      var match = line.match(/^\s*(\b\w*TestRunner.[a-z_A-Z0-9]+)\s*(\=[^,}]|[;])/) ||
          line.match(/^(TestRunner.[a-z_A-Z0-9]+)\s*\=$/);
      if (!match)
        continue;
      var name = match[1];
      var components = name.split('.');
      if (components.length !== 2)
        continue;
      propertyToNamespace.set(components[1], {
        namespace: components[0],
        done: true,
      });
    }
  }
}

function scrapeOriginalHelpers(propertyToNamespace, helpers) {
  const testRunnerPaths = fs.readdirSync(FRONT_END_PATH)
                              .filter(folder => folder.indexOf('test_runner') !== -1)
                              .map(folder => path.resolve(FRONT_END_PATH, folder));

  for (const helper of helpers) {
    scrapeTestHelperIdentifiers(helper.originalContents, helper.namespace);
  }

  function scrapeTestHelperIdentifiers(content, namespace) {
    var lines = content.split('\n');
    for (var line of lines) {
      var line = line.trim();
      if (line.indexOf('InspectorTest.') === -1)
        continue;
      var match = line.match(/^\s*(\b\w*InspectorTest.[a-z_A-Z0-9]+)\s*(\=[^,}]|[;])/) ||
          line.match(/^(InspectorTest.[a-z_A-Z0-9]+)\s*\=$/);
      if (!match)
        continue;
      var name = match[1];
      var components = name.split('.');
      if (components.length !== 2)
        continue;
      propertyToNamespace.set(components[1], {
        namespace: namespace,
        done: false,
      });
    }
  }
}

function appendComment(helpers) {
  for (const {originalPath, originalContents, helperModule, filename} of helpers) {
    const comment = `// This file is being deprecated and is moving to front_end/${helperModule}/${filename}
// Please see crbug.com/667560 for more details\n\n`;
    fs.writeFileSync(originalPath, comment + originalContents);
  }
}

function writeNewHelpers(helpers) {
  for (const {contents, helperModule, filename} of helpers) {
    const modulePath = path.resolve(FRONT_END_PATH, helperModule);
    if (!utils.isDir(modulePath))
      fs.mkdirSync(modulePath);
    const destPath = path.resolve(modulePath, filename);
    fs.writeFileSync(destPath, contents);
  }
}

function mapTestFilesToHelperModules(helperPaths) {
  const helpers = new Set();
  for (const p of helperPaths) {
    let namespacePrefix = path.basename(p)
                              .split('-test')[0]
                              .split('-')
                              .map(a => a.substring(0, 1).toUpperCase() + a.substring(1))
                              .join('');
    let filenamePrefix = namespacePrefix;

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
      namespacePrefix = 'Sources';
    if (namespacePrefix === 'Resources') {
      namespacePrefix = 'Application';
    }
    if (namespacePrefix === 'Appcache')
      namespacePrefix = 'Application';
    if (namespacePrefix === 'ResourceTree')
      namespacePrefix = 'Application';
    if (namespacePrefix === 'ServiceWorkers')
      namespacePrefix = 'Application';
    if (namespacePrefix === 'CacheStorage')
      namespacePrefix = 'Application';
    if (namespacePrefix === 'Indexeddb') {
      namespacePrefix = 'Application';
      filenamePrefix = 'IndexedDB';
    }
    if (namespacePrefix === 'Timeline')
      namespacePrefix = 'Performance';
    if (namespacePrefix === 'ProductRegistry')
      namespacePrefix = 'Network';
    if (namespacePrefix === 'Search')
      namespacePrefix = 'Sources';
    if (namespacePrefix === 'LiveEdit')
      namespacePrefix = 'Sources';
    if (namespacePrefix === 'Persistence')
      namespacePrefix = 'Bindings';
    if (namespacePrefix === 'IsolatedFilesystem')
      namespacePrefix = 'Bindings';
    if (namespacePrefix === 'Automapping')
      namespacePrefix = 'Bindings';
    const contents = fs.readFileSync(p, 'utf-8');
    const namespace = namespacePrefix + 'TestRunner';
    const helperModule = namespace.replace(/([A-Z])/g, '_$1').replace(/^_/, '').toLowerCase();
    const filename = filenamePrefix + 'TestRunner.js';
    helpers.add({
      originalPath: p,
      newPath: path.resolve(FRONT_END_PATH, helperModule, filename),
      namespace,
      helperModule,
      filename,
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
      else if (file.indexOf('-test.js') !== -1)
        paths.push(file);
    }
  }

  return paths;
}

/**
 * Hack to quickly create an AST node
 */
function createAwaitExpressionNode(code) {
  return recast
      .parse(`(async function(){
  ${code}
  })()`)
      .program.body[0];
}