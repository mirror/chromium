/* This test tests the efficiency of using an object as a dictionary.
 * The performance on this benchmark correlates to some extent with
 * the performance of string-unpack-code from SunSpider.
 */

var alphabet = [
 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
 'y', 'z', '0', '1', '2', '3', '4', '5'];

// For an index between 0 and 1023, returns a 1 or 2-character ConsString.
function short_cons(index) {
  return alphabet[index >> 5] + alphabet[index & 31];
}

// For an index between 0 and 1023, returns a 2-character symbol (SeqString).
function short_symbol(index) {
  return eval("'" + short_cons(index) + "'");
}

var all_strings = "";

for (var i = 0; i < 1024; i++) {
  all_strings += short_cons(i);
}

// Flatten the string.
all_strings = eval("'" + all_strings + "'");


// For an index between 0 and 1023, returns a 2-character SlicedString.
function short_slice(index) {
  return all_strings.substring(index << 1, (index << 1) + 2);
}


var symbols =  new Array();
var slices = new Array();
var conses = new Array();

for (i = 0; i < 1000; i++) {
  symbols.push(short_symbol(i));
  slices.push(short_slice(i));
  conses.push(short_cons(i));
}


function work(set, string_array, name, overhead) {
  var i;
  var set_repetitions = set ? 100 : 1;
  var get_repetitions = set ? 1 : 100;
  var elapsed = 0;
  var start = new Date();
  for (var n = 0; elapsed < 2000; n++) {
    for (i = 0; i < set_repetitions; i++) {
      var dict = new Object();
      for (j = 0; j < 1000; j++) {
        if (string_array != null)
          dict[string_array[j]] = j;
      }
    }
    for (i = 0; i < get_repetitions; i++) {
      for (j = 0; j < 1000; j++) {
        if (string_array != null && dict[string_array[j]] != j) {
          throw new Error(name + " put/get");
        }
      }
    }
    elapsed = new Date() - start;
  }
  var ms = Math.floor(elapsed/n);
  var explanation = (overhead == 0) ? "" : " (overhead has been subtracted)";
  print('Time (dictionary-' + name +'): ' + (ms - overhead) + ' ms.' + explanation);
  return ms;
}

var set_overhead = work(true, null, "set-overhead", 0);
var get_overhead = work(false, null, "get-overhead", 0);
work(true, symbols, "set-symbols", set_overhead);
work(false, symbols, "get-symbols", get_overhead);
work(true, slices, "set-slices", set_overhead);
work(false, slices, "get-slices", get_overhead);
work(true, conses, "set-conses", set_overhead);
work(false, conses, "get-conses", get_overhead);
