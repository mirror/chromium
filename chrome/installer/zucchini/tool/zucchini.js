// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Hackable Javascript Implementation of Zucchini. Currently, only "dump patch"
// (-dp) feature is available.
'use strict';
var fs = require('fs');

/******** Constants ********/

const kMagicStr = 'Zuc\x00';
const kMagic = new DataView(new Uint8Array(kMagicStr.split('').map(
    function(x){ return x.charCodeAt(0); })).buffer).getUint32(0, true);

const PATCH_TYPE_RAW = 0;
const PATCH_TYPE_SINGLE = 1;
const PATCH_TYPE_ENSEMBLE = 2;
const PATCH_TYPE_TO_STR = [
  'Raw', 'Single', 'Ensemble'
];

/******** Global Variables ********/

var s_t0 = null;

/******** Utilities ********/

var COLOR = {
  RED: (s) => { return '\x1B[1;31m' + s + '\x1B[0m'}
};

// Hex look up table that includes '0' padding.
var HEX_LUT = (function() {
  let t = '0123456789ABCDEF';
  let ret = Array(256);
  for (let i = 0; i < 256; ++i)
    ret[i] = t[i >> 4] + t[i & 0x0F];
  return ret;
})();

function hex2(n) {
  return HEX_LUT[n & 0xFF];
}
var hex8 = (function() {
  let v8 = new Uint8Array(4);
  let v32 = new Uint32Array(v8.buffer);
  return function(n) {  // Assumes little endian.
    v32[0] = n;
    return HEX_LUT[v8[3]] + HEX_LUT[v8[2]] + HEX_LUT[v8[1]] + HEX_LUT[v8[0]];
  }
})();

function floatNow() {
  var t = process.hrtime();
  return t[0] + t[1] / 1000000000;
}

function runTimeText() {
  return (floatNow() - s_t0).toFixed(3) + ' s';
}

function printWarn(s) {
  console.log(COLOR.RED(s));
}

/******** Stream Helpers ********/

function readFile(fname) {
  let buffer = fs.readFileSync(fname);
  return new Uint8Array(buffer);
}

function singleToMulti(rs) {
  let n = rs.nextVarUInt32();
  let sizes = new Array(n);
  for (let i = 0; i < n; ++i)
    sizes[i] = rs.nextVarUInt32();
  let ret = new Array(n);
  for (let i = 0; i < n; ++i)
    ret[i] = rs.nextSubStream(sizes[i]);
  return ret;
}

class CommandLine {
  constructor(argv) {
    this.command = argv.length >= 3 ? argv[2] : null;
    this.args = [];
    this.switches = {};
    for (let i = 3; i < argv.length; ++i) {
      let tok = argv[i];
      if (tok.startsWith('-')) {
        let sw = tok.replace(/^-+/, '');
        if (sw.length) {
          let m = sw.match(/^(.*)=(.*)$/);
          if (m) {
            this.switches[m[1]] = m[2];
          } else {
            this.switches[sw] = null;
          }
        }
      } else {
        this.args.push(tok);
      }
    }
  }

  hasSwitch(sw) {
    return (sw in this.switches);
  }
}

/******** BinaryReadStream ********/
class BinaryReadStream {
  constructor(buf, opt_offset, opt_size) {
    let offset = opt_offset === undefined ? 0 : opt_offset;
    this.size = (opt_size !== undefined) ? opt_size : (buf.byteLength - offset);
    this.view = new DataView(buf, offset, this.size);
    this.pos = 0;
  }

  empty() {
    return this.pos >= this.size;
  }

  reset() {
    this.pos = 0;
  }

  remaining() {
    return this.size - this.pos;
  }

  peekRemainingData() {
    let head = this.view.byteOffset + this.pos;
    let tail = head + this.remaining();
    return new Uint8Array(this.view.buffer).subarray(head, tail);
  }

  remainingData() {
    let head = this.view.byteOffset + this.pos;
    let tail = head + this.remaining();
    this.pos = this.size;
    return new Uint8Array(this.view.buffer).subarray(head, tail);
  }

  nextUInt8() {
    if (this.pos >= this.size)
      throw new Error('Out of data.');
    return this.view.getUint8(this.pos++);
  }

  nextUInt16() {
    if (this.pos + 1 >= this.size)
      throw new Error('Out of data.');
    let ret = this.view.getUint16(this.pos, true);
    this.pos += 2;
    return ret;
  }

  nextUInt32() {
    if (this.pos + 3 >= this.size)
      throw new Error('Out of data.');
    let ret = this.view.getUint32(this.pos, true);
    this.pos += 4;
    return ret;
  }

  nextBytes(n) {
    if (n === undefined)
      n = this.size - this.pos;
    else if (this.pos + n > this.size)
      throw new Error('Out of data.');
    let ret = new Uint8Array(
        this.view.buffer, this.view.byteOffset + this.pos, n);
    this.pos += n;
    return ret;
  }

  nextVarUInt32() {
    let a = this.nextUInt8();
    if (a < 0x80)
      return a;
    a &= 0x7F;
    let b = this.nextUInt8();
    if (b < 0x80)
      return (b << 7) | a;
    b &= 0x7F;
    let c = this.nextUInt8();
    if (c < 0x80)
      return (c << 14) | (b << 7) | a;
    c &= 0x7F;
    let d = this.nextUInt8();
    if (d < 0x80)
      return (d << 21) | (c << 14) | (b << 7) | a;
    d &= 0x7F;
    let e = this.nextUInt8();
    return (((e & 0x0F) << 28) | (d << 21) | (c << 14) | (b << 7) | a) >>> 0;
  }

  nextVarSInt32() {
    let ret = this.nextVarUInt32();
    let sign = ret & 1;
    ret >>= 1;
    return sign ? ~ret : ret;
  }

  jump(pos) {
    if (pos < 0 || pos > this.size)
      throw new Error('Invalid position.');
    this.pos = pos;
  }

  nextSubStream(n) {
    if (n > this.remaining())
      throw new Error('Out of data.');
    let ret = new BinaryReadStream(
        this.view.buffer, this.view.byteOffset + this.pos, n);
    this.pos += n;
    return ret;
  }

  remainingVarUInt32Vector(signed) {
    let old_pos = this.pos;
    let n = 0;
    while (!this.empty()) {
      this.nextVarUInt32();  // Sign doesn't matter for counting.
      ++n;
    }
    let ret = null;
    this.pos = old_pos;
    if (signed) {
      ret = new Int32Array(n);
      for (let i = 0; i < n; ++i) {
        ret[i] = this.nextVarSInt32();
      }
    } else {
      ret = new Uint32Array(n);
      for (let i = 0; i < n; ++i) {
        ret[i] = this.nextVarUInt32();
      }
    }
      return ret;
  }

  nextVarUInt32Vector() {
    let n = this.nextVarUInt32();
    let ret = new Uint32Array(n);
    for (let i = 0; i < n; ++i) {
      ret[i] = this.nextVarUInt32();
    }
    return ret;
  }

  nextVarUInt32DeltaVector() {
    let n = this.nextVarUInt32();
    let ret = new Uint32Array(n);
    let cur = 0;
    for (let i = 0; i < n; ++i) {
      cur = toUint32(cur + this.nextVarUInt32());
      ret[i] = cur;
    }
    return ret;
  }
}

/******** Crc32 ********/
class Crc32 {
  constructor() {
    this.generateTable();
  }

  generateTable() {
    this.table = new Uint32Array(256);
    for (let i = 0; i < 256; ++i) {
      let r = i;
      for (let j = 0; j < 8; ++j)
        r = (r >>> 1) ^ (Crc32.kCrcPoly & ~((r & 1) - 1));
      this.table[i] = r;
    }
  }

  crcUpdate(v, data) {
    let table = this.table;
    // Actually faster to have |size| variable, and |i| outside for loop!
    let size = data.length;
    let i = 0;
    for (; size > 0; --size, ++i)
      v = table[(v ^ data[i]) & 0xFF] ^ (v >>> 8);
    return v >>> 0;
  }

  static get singleton() {
    if (this._instance === undefined)
      this._instance = new Crc32();
    return this._instance;
  }

  static compute(data) {
    return Crc32.singleton.crcUpdate(0xFFFFFFFF, data) ^ 0xFFFFFFFF;
  }
}
Crc32.kCrcPoly = 0xEDB88320;

/******** Equivalence ********/
class Equivalence {
  constructor(src, dst, length) {
    this.src = src;
    this.dst = dst;
    this.length = length;
  }

  toString() {
    return '[' + hex8(this.src) + ',' + hex8(this.src + this.length) +
        ') -> [' + hex8(this.dst) + ',' + hex8(this.dst + this.length) +
        '): ' + this.length;
  }
};

function ParseBuffer(rs) {
  let size = rs.nextUInt32();
  return rs.nextSubStream(size);
}

/******** GroupedPrinter ********/
class GroupedPrinter {
  constructor() {
    this.cur_val = null;
    this.count = 0;
    this.prev_idx = 0;
    this.idx = 0;
  }
  flush_(v) {
    let size = this.idx - this.prev_idx;
    if (size <= 1) {
      console.log(`${this.prev_idx}: ${this.cur_val}`);
    } else {
      console.log(`${this.prev_idx}: ${this.cur_val} (${size})`);
    }
    this.prev_idx = this.idx;
  }
  next(v) {
    if (this.cur_val !== v) {
      if (this.cur_val !== null) {
        this.flush_();
      }
      this.cur_val = v;
    }
    ++this.idx;
  }
  end() {
    if (this.cur_val !== null) {
      this.flush_();
    }
  }
}

/******** PatchElement ********/
class PatchElement {
  constructor(rs) {
    this.old_offset = rs.nextUInt32();
    this.new_offset = rs.nextUInt32();
    this.old_length = rs.nextUInt32();
    this.new_length = rs.nextUInt32();
    this.exec_type = rs.nextUInt32();
    this.rs_src_skip = ParseBuffer(rs);
    this.rs_dst_skip = ParseBuffer(rs);
    this.rs_copy_count = ParseBuffer(rs);
    this.rs_extra_data = ParseBuffer(rs);
    this.rs_raw_delta_skip = ParseBuffer(rs);
    this.rs_raw_delta_diff = ParseBuffer(rs);
    this.rs_reference_delta = ParseBuffer(rs);
    this.pool_count = rs.nextUInt32();
    this.pool_tags = [];
    this.target_lists = [];
    let rs_extra_target_map = {};
    for (let i = 0; i < this.pool_count; ++i) {
      let pool_tag_value = rs.nextUInt8();
      this.pool_tags.push(pool_tag_value);
      rs_extra_target_map[pool_tag_value] = ParseBuffer(rs);
    }

    // Parse Equivalences.
    this.equiv_list = [];
    let src = 0;
    let dst = 0;
    while (!this.rs_src_skip.empty()) {
      src = (src + this.rs_src_skip.nextVarSInt32()) >> 0;
      dst = (dst + this.rs_dst_skip.nextVarUInt32()) >> 0;
      let length = this.rs_copy_count.nextVarUInt32();
      this.equiv_list.push(new Equivalence(src, dst, length));
      src = (src + length) >> 0;
      dst = (dst + length) >> 0;
    }
    if (!this.rs_dst_skip.empty())
      printWarn('Excessive data in dst_skip stream.');
    if (!this.rs_copy_count.empty())
      printWarn('Excessive data in copy_count stream.');

    // Parse Raw Deltas.
    this.num_delta_skip = this.rs_raw_delta_skip.remaining();
    this.num_delta_diff = this.rs_raw_delta_diff.remaining();

    this.delta_diff_offset = new Uint32Array(this.num_delta_diff);
    let diff_offset = 0;
    for (let i = 0; i < this.num_delta_diff; ++i) {
      if (this.rs_raw_delta_skip.empty()) {
        printWarn('Insufficient data in raw_delta_skip stream.');
        break;
      }
      diff_offset += this.rs_raw_delta_skip.nextVarUInt32();
      this.delta_diff_offset[i] = diff_offset;
      diff_offset += 1;  // Bias.
    }
    if (!this.rs_raw_delta_skip.empty())
      printWarn('Excessive data in raw_delta_skip stream.');
    this.delta_bytes = this.rs_raw_delta_diff.remainingData();

    // Parse Reference Deltas.
    this.reference_delta =
        this.rs_reference_delta.remainingVarUInt32Vector(true);

    // Parse Extra Targets.
    this.extra_targets = {};
    for (let i = 0; i < this.pool_count; ++i) {
      let rs_extra_target = rs_extra_target_map[this.pool_tags[i]];
      let target_list = rs_extra_target.remainingVarUInt32Vector(false);
      let cur_target = 0;
      for (let j = 0; j < target_list.length; ++j) {
        target_list[j] += cur_target;
        cur_target = target_list[j] + 1;  // Bias.
      }
      this.target_lists.push(target_list);
    }
  }

  dump() {
    console.log(`Old offset: ${this.old_offset}`);
    console.log(`New offset: ${this.new_offset}`);
    console.log(`Old length: ${this.old_length}`);
    console.log(`New length: ${this.new_length}`);
    console.log(`Exec type : ${this.exec_type}`);
  }

  dumpDetails(verbose) {
    let show_equiv = 'equiv' in verbose;
    let show_delta = 'delta' in verbose;
    let show_ref_delta = 'ref_delta' in verbose;

    console.log('Number of equivalences: ' + this.equiv_list.length);
    let total_copied = 0;
    for (let eq of this.equiv_list) {
      total_copied += eq.length;
    }
    let copied_frac = total_copied / this.new_length;
    let total_extra = this.new_length - total_copied;
    console.log('Total copied: ' + total_copied + ' = ' +
                (copied_frac * 100).toFixed(2) + '%');
    console.log('Total extra : ' + total_extra);
    if (this.rs_extra_data.remaining() !== total_extra) {
      printWarn('Inconsistent extra data count: Expect ' + total_extra +
                ', have ' + this.rs_extra_data.remaining());
    }
    console.log('Delta skip bytes: ' + this.num_delta_skip);
    console.log('Delta diff bytes: ' + this.num_delta_diff);
    console.log('Reference deltas: ' + this.reference_delta.length);
    console.log('Extra targets in each pool:');
    for (let i = 0; i < this.pool_tags.length; ++i) {
      let pool_tag = this.pool_tags[i];
      console.log(`  ${pool_tag}: ${this.target_lists[pool_tag].length}`);
    }

    if (show_equiv) {
      if (show_delta)
        console.log('*** Equivalences and Raw Deltas ***');
      else
        console.log('*** Equivalences ***');
    } else if (show_delta) {
      console.log('*** Raw Deltas ***');
    }

    // Walk through equivalences and deltas.
    let diff_offset_base = 0;
    let prev_eq = null;
    let delta_idx = 0;
    for (let eq of this.equiv_list) {
      let prev_delta_idx = delta_idx;
      let diff_offset_lim = diff_offset_base + eq.length;
      while (delta_idx < this.num_delta_diff &&
             this.delta_diff_offset[delta_idx] < diff_offset_lim) {
        ++delta_idx;
      }
      let eq_str = '';
      if (show_equiv) {
        let num_delta = delta_idx - prev_delta_idx;
        let delta_str = num_delta > 0 ? ` (${num_delta})` : '';
        eq_str = eq.toString() + delta_str;
        console.log(eq_str);
        if (prev_eq) {
          if (eq.dst < prev_eq.dst) {
            printWarn('Decreasing equivalence entry!');
          } else if (eq.dst < prev_eq.dst + prev_eq.length) {
            printWarn('Overlapping equivalence entry!');
          }
        }
      }
      if (show_delta) {
        for (let i = prev_delta_idx; i < delta_idx; ++i) {
          let diff_offset = this.delta_diff_offset[i];
          let t = show_equiv ? '  ' : '';
          t += hex8(i) + ': ' + hex8(diff_offset) + ': ' +
               hex8(eq.dst + diff_offset - diff_offset_base) + ': ' +
               hex2(this.delta_bytes[i])
          console.log(t);
        }
      }
      diff_offset_base = diff_offset_lim;
      prev_eq = eq;
    }

    if (show_ref_delta) {
      console.log('*** Reference deltas ***');
      let printer = new GroupedPrinter();
      for (let i = 0; i < this.reference_delta.length; ++i) {
        printer.next(this.reference_delta[i]);
      }
      printer.end();
      for (let i = 0; i < this.pool_count; ++i) {
        console.log(`*** Extra targets for Pool ${this.pool_tags[i]} ***`);
        let target_list = this.target_lists[i];
        for (let j = 0; j < target_list.length; ++j) {
          console.log(`${hex8(target_list[j])}`);
        }
      }
    }
  }
}

/******** PatchStream ********/
class PatchStream {
  constructor(rs) {
    let magic = rs.nextUInt32();
    if (magic !== kMagic)
      throw new Error('Bad patch magic.');
    this.old_size = rs.nextUInt32();
    this.old_crc = rs.nextUInt32();
    this.new_size = rs.nextUInt32();
    this.new_crc = rs.nextUInt32();
    this.patch_type = rs.nextUInt32();
    let element_count = rs.nextUInt32();
    this.elements = new Array(element_count);
    for (let i = 0; i < element_count; ++i) {
      this.elements[i] = new PatchElement(rs);
    }
  }

  dump(verbose) {
    if (!verbose)
      verbose = {};

    console.log(`******** Summary ********`);
    console.log(`Old size: ${this.old_size}`);
    console.log(`Old CRC : 0x${hex8(this.old_crc)}`);
    console.log(`New size: ${this.new_size}`);
    console.log(`New CRC : 0x${hex8(this.new_crc)}`);
    console.log(`Patch type: ${this.patch_type} ` +
                `(${PATCH_TYPE_TO_STR[this.patch_type]})`);
    console.log(`Number of elements: ${this.elements.length}`);

    console.log('');
    for (let i = 0; i < this.elements.length; ++i) {
      console.log(`******** Element ${i} ********`);
      this.elements[i].dump();
      this.elements[i].dumpDetails(verbose);
    }
  }
}

/******** Application ********/
class Application {
  constructor(command_line) {
    this.command_line = command_line;
  }

  static set prog_command(v) {
    this._prog_command = v;
  }

  static get prog_command() {
    return this._prog_command;
  }

  static printGeneralHelp() {
    console.log('Must have exactly one of:');
    let app_list = [];
    for (let entry of APP_LIST)
      app_list.push(entry[0]);
    console.log('   ' + app_list.join(', '));
  }

  static printUsage() {
    console.log('Usage:');
    let prefix = '    ' + Application.prog_command;
    for (let entry of APP_LIST) {
      console.log(prefix + ' ' + entry[1].getSpecificHelpString());
    }
  }

  static printHelpAndExit() {
    Application.printGeneralHelp();
    Application.printUsage();
    process.exit(0);
  }

  static getSpecificHelpString() { }

  static getSpecificHelpParams() { }

  printSpecificHelpAndExit() {
    console.log(this.constructor.getSpecificHelpString());
    let help_params = this.constructor.getSpecificHelpParams();
    for (let line of help_params)
      console.log('    ' + line);
    Application.printUsage();
    process.exit(0);
  }

  checkParams(params, n) {
    if (params.length != n || params.indexOf('') >= 0)
      this.printSpecificHelpAndExit();
  }

  hasSwitch(sw) {
    return this.command_line.hasSwitch(sw);
  }
}

/******** DumpPatchApplication ********/
class DumpPatchApplication extends Application {
  constructor(command_line) {
    super(command_line);
  }

  static getSpecificHelpString() {
    return '-dp <patch_file> [-v] [-e] [-d] [-r]';
  }

  static getSpecificHelpParams() {
      return ['-v: Verbose, equal to -e -r',
              '-e: Dump equivalences',
              '-d: Dump raw deltas',
              '-r: Dump reference deltas'];
  }

  dump(patch) {
    let rs = new BinaryReadStream(patch.buffer);
    let ps = new PatchStream(rs);
    let verbose = {};
    if (this.hasSwitch('v') || this.hasSwitch('e'))
      verbose['equiv'] = null;
    if (this.hasSwitch('d'))
      verbose['delta'] = null;
    if (this.hasSwitch('v') || this.hasSwitch('r'))
      verbose['ref_delta'] = null;
    ps.dump(verbose);
  }

  run() {
    this.checkParams(this.command_line.args, 1);
    let patch_fname = this.command_line.args[0];
    let patch = readFile(patch_fname);
    // console.log('Read file (' + runTimeText() + ')');
    this.dump(patch);
  }
}

/******** Global Variables ********/

var APP_LIST = [
  ['-dp', DumpPatchApplication],
];

var APP_MAP = (function() {
  let ret = {};
  for (let entry of APP_LIST)
    ret[entry[0]] = entry[1];
  return ret;
})();

/******** Main ********/

function main() {
  s_t0 = floatNow();
  Application.prog_command = 'node ' + process.argv[1].replace(/.*[\\/]/, '');
  if (process.argv.length < 3)
    Application.printHelpAndExit();
  let command_line = new CommandLine(process.argv);
  let app_name = command_line.command;
  if (app_name === null || !(app_name in APP_MAP))
    Application.printHelpAndExit();
  let app = new APP_MAP[app_name](command_line);
  try {
    app.run();
  } catch (e) {
    console.log(e.stack);
  }
}

main();
