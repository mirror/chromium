// META: script=resources/wasm-constants.js
// META: script=resources/wasm-module-builder.js

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains tests to exercise Wasm's in- and out- of bounds behavior.

const module_bytes = (function createModule() {
  const builder = new WasmModuleBuilder;

  builder.addImportedMemory("external", "memory");

  const peek = builder.addFunction("peek", kSig_i_i).exportFunc();
  peek.body
      .get_local(0)
      .i32_load()
      .end();

  const poke = builder.addFunction("poke", kSig_v_ii).exportFunc();
  poke.body
      .get_local(0)
      .get_local(1)
      .i32_store()
      .end();

  const grow = builder.addFunction("grow", kSig_i_i).exportFunc();
  grow.body
      .get_local(0)
      .grow_memory()
      .end();

  return builder.toBuffer();
})();

function instantiate(memory) {
  assert_true(module_bytes instanceof ArrayBuffer,
              "module bytes should be an ArrayBuffer");
  assert_true(memory instanceof WebAssembly.Memory,
              "memory must be a WebAssembly.Memory");
  return WebAssembly.instantiate(module_bytes, { external: { memory: memory } })
      .then(result => result.instance);
}

function instantiatePages(num_pages) {
  return instantiate(new WebAssembly.Memory({ initial: num_pages }));
}

function assert_oob(func, description) {
  return assert_throws(new WebAssembly.RuntimeError, func, description);
}

promise_test(function() {
  return instantiatePages(1).then(function(instance) {
    const peek = instance.exports.peek;

    assert_equals(peek(0), 0, "Peeked nonzero value");
    assert_equals(peek(10000), 0, "Peeked nonzero value");
    assert_equals(peek(65532), 0, "Peeked nonzero value");
  });
}, "WebAssembly#PeekInBounds");

promise_test(function() {
  return instantiatePages(1).then(function(instance) {
    const peek = instance.exports.peek;

    assert_oob(_ => peek(65536));
    assert_oob(_ => peek(65535));
    assert_oob(_ => peek(65534));
    assert_oob(_ => peek(65533));

    assert_oob(_ => peek(1 << 30));
    assert_oob(_ => peek(3 << 30));
  });
}, "WebAssembly#PeekOutOfBounds");

promise_test(function() {
  const memory = new WebAssembly.Memory({initial: 0});
  return instantiate(memory).then(function(instance) {
    const peek = instance.exports.peek;

    assert_oob(_ => peek(0));
    memory.grow(1);
    assert_equals(peek(0), 0);
  });
}, "WebAssembly#PeekOutOfBoundsGrowMemoryFromZeroJS");

promise_test(function() {
  const memory = new WebAssembly.Memory({initial: 1});
  return instantiate(memory).then(function(instance) {
    const peek = instance.exports.peek;

    assert_oob(_ => peek(70000));
    memory.grow(1);
    assert_equals(peek(70000), 0);
  });
}, "WebAssembly#PeekOutOfBoundsGrowMemoryJS");

promise_test(function() {
  const memory = new WebAssembly.Memory({initial: 0});
  return instantiate(memory).then(function(instance) {
    const peek = instance.exports.peek;
    const grow = instance.exports.grow;

    assert_oob(_ => peek(0));
    grow(1);
    assert_equals(peek(0), 0);
  });
}, "WebAssembly#PeekOutOfBoundsGrowMemoryFromZeroWasm");

promise_test(function() {
  const memory = new WebAssembly.Memory({initial: 1});
  return instantiate(memory).then(function(instance) {
    const peek = instance.exports.peek;
    const grow = instance.exports.grow;

    assert_oob(_ => peek(70000));
    grow(1);
    assert_equals(peek(70000), 0);
  });
}, "WebAssembly#PeekOutOfBoundsGrowMemoryWasm");

promise_test(function() {
  return instantiatePages(1).then(function(instance) {
    const peek = instance.exports.peek;
    const poke = instance.exports.poke;

    poke(0, 41);
    poke(10000, 42);
    poke(65532, 43);

    assert_equals(peek(0), 41);
    assert_equals(peek(10000), 42);
    assert_equals(peek(65532), 43);
  });
}, "WebAssembly#PokeInBounds");

promise_test(function() {
  return instantiatePages(1).then(function(instance) {
    const poke = instance.exports.poke;

    assert_oob(_ => poke(65536, 0));
    assert_oob(_ => poke(65535, 0));
    assert_oob(_ => poke(65534, 0));
    assert_oob(_ => poke(65533, 0));

    assert_oob(_ => poke(1 << 30, 0));
    assert_oob(_ => poke(3 << 30, 0));
  });
}, "WebAssembly#PokeOutOfBounds");

promise_test(function() {
  const memory = new WebAssembly.Memory({initial: 0});
  return instantiate(memory).then(function(instance) {
    const peek = instance.exports.peek;
    const poke = instance.exports.poke;

    function check_poke(index, value) {
      poke(index, value);
      assert_equals(peek(index), value);
    }

    assert_oob(_ => poke(0, 42));
    memory.grow(1);
    check_poke(0, 42);
  });
}, "WebAssembly#PokeOutOfBoundsGrowMemoryFromZeroJS");

promise_test(function() {
  const memory = new WebAssembly.Memory({initial: 1});
  return instantiate(memory).then(function(instance) {
    const peek = instance.exports.peek;
    const poke = instance.exports.poke;

    function check_poke(index, value) {
      poke(index, value);
      assert_equals(peek(index), value);
    }

    assert_oob(_ => poke(70000, 42));
    memory.grow(1);
    check_poke(70000, 42);
  });
}, "WebAssembly#PokeOutOfBoundsGrowMemoryJS");

promise_test(function() {
  const memory = new WebAssembly.Memory({initial: 0});
  return instantiate(memory).then(function(instance) {
    const peek = instance.exports.peek;
    const poke = instance.exports.poke;
    const grow = instance.exports.grow;

    function check_poke(index, value) {
      poke(index, value);
      assert_equals(peek(index), value);
    }

    assert_oob(_ => poke(0, 42));
    grow(1);
    check_poke(0, 42);
  });
}, "WebAssembly#PokeOutOfBoundsGrowMemoryFromZeroWasm");

promise_test(function() {
  const memory = new WebAssembly.Memory({initial: 1});
  return instantiate(memory).then(function(instance) {
    const peek = instance.exports.peek;
    const poke = instance.exports.poke;
    const grow = instance.exports.grow;

    function check_poke(index, value) {
      poke(index, value);
      assert_equals(peek(index), value);
    }

    assert_oob(_ => poke(70000, 42));
    grow(1);
    check_poke(70000, 42);
  });
}, "WebAssembly#PokeOutOfBoundsGrowMemoryWasm");
