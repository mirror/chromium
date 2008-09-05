function Thirtytwo (hi, lo) {
  this.l = lo;
  this.h = hi;
}


(function() {
  function plus_equals (other) {
    var seventeen_bits = this.l + other.l;
    this.l = (seventeen_bits & 0xffff);
    this.h = ((this.h + other.h + (seventeen_bits >>> 16)) & 0xffff);
  }
  Thirtytwo.prototype.plus_equals = plus_equals;


  function to_int() {
    return (this.h << 16) + this.l;
  }
  Thirtytwo.prototype.to_int = to_int;


  function rotate_equals(s) {
    var sl = (s & 15);
    var ol = ((this.h >>> (16 - sl)) | (this.l << sl));
    var oh = ((this.l >>> (16 - sl)) | (this.h << sl));
    if (s < 16) {
      this.l = ol & 0xffff;
      this.h = oh & 0xffff;
    } else {
      this.l = oh & 0xffff;
      this.h = ol & 0xffff;
    }
  }
  Thirtytwo.prototype.rotate_equals = rotate_equals;


  function copy(other) {
    this.l = other.l;
    this.h = other.h;
  }
  Thirtytwo.prototype.copy = copy;


  function xor_equals(other) {
    this.l ^= other.l;
    this.h ^= other.h;
  }
  Thirtytwo.prototype.xor_equals = xor_equals;


  function and_equals(other) {
    this.l &= other.l;
    this.h &= other.h;
  }
  Thirtytwo.prototype.and_equals = and_equals;


  function or_equals(other) {
    this.l |= other.l;
    this.h |= other.h;
  }
  Thirtytwo.prototype.or_equals = or_equals;


  function set_words(hi, lo) {
    this.l = lo;
    this.h = hi;
  }
  Thirtytwo.prototype.set_words = set_words;

  function not() {
    this.l ^= 0xffff;
    this.h ^= 0xffff;
  }
  Thirtytwo.prototype.not = not;


  var hex_chars = ['0', '1', '2', '3', '4', '5', '6', '7',
                   '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'];


  function to_hex() {
    return hex_chars[(this.l >> 4) & 0xf] +
           hex_chars[(this.l     ) & 0xf] +
           hex_chars[(this.l >> 12) & 0xf] +
           hex_chars[(this.l >> 8) & 0xf] +
           hex_chars[(this.h >> 4) & 0xf] +
           hex_chars[(this.h     ) & 0xf] +
           hex_chars[(this.h >> 12) & 0xf] +
           hex_chars[(this.h >> 8) & 0xf];
  }
  Thirtytwo.prototype.to_hex = to_hex;
})();


function MD5Accumulator() {
  this.h0 = new Thirtytwo(0x6745, 0x2301);
  this.h1 = new Thirtytwo(0xEFCD, 0xAB89);
  this.h2 = new Thirtytwo(0x98BA, 0xDCFE);
  this.h3 = new Thirtytwo(0x1032, 0x5476);
  this.word_array = new Array(32);
  this.word_array_fullness = 0;
  this.length = 0;
}


(function() {
  var r =
          [7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
           5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
           4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
           6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21];

  var k = [
    new Thirtytwo(0xd76a, 0xa478), new Thirtytwo(0xe8c7, 0xb756),
    new Thirtytwo(0x2420, 0x70db), new Thirtytwo(0xc1bd, 0xceee),
    new Thirtytwo(0xf57c, 0x0faf), new Thirtytwo(0x4787, 0xc62a),
    new Thirtytwo(0xa830, 0x4613), new Thirtytwo(0xfd46, 0x9501),
    new Thirtytwo(0x6980, 0x98d8), new Thirtytwo(0x8b44, 0xf7af),
    new Thirtytwo(0xffff, 0x5bb1), new Thirtytwo(0x895c, 0xd7be),
    new Thirtytwo(0x6b90, 0x1122), new Thirtytwo(0xfd98, 0x7193),
    new Thirtytwo(0xa679, 0x438e), new Thirtytwo(0x49b4, 0x0821),
    new Thirtytwo(0xf61e, 0x2562), new Thirtytwo(0xc040, 0xb340),
    new Thirtytwo(0x265e, 0x5a51), new Thirtytwo(0xe9b6, 0xc7aa),
    new Thirtytwo(0xd62f, 0x105d), new Thirtytwo(0x244, 0x1453),
    new Thirtytwo(0xd8a1, 0xe681), new Thirtytwo(0xe7d3, 0xfbc8),
    new Thirtytwo(0x21e1, 0xcde6), new Thirtytwo(0xc337, 0x07d6),
    new Thirtytwo(0xf4d5, 0x0d87), new Thirtytwo(0x455a, 0x14ed),
    new Thirtytwo(0xa9e3, 0xe905), new Thirtytwo(0xfcef, 0xa3f8),
    new Thirtytwo(0x676f, 0x02d9), new Thirtytwo(0x8d2a, 0x4c8a),
    new Thirtytwo(0xfffa, 0x3942), new Thirtytwo(0x8771, 0xf681),
    new Thirtytwo(0x6d9d, 0x6122), new Thirtytwo(0xfde5, 0x380c),
    new Thirtytwo(0xa4be, 0xea44), new Thirtytwo(0x4bde, 0xcfa9),
    new Thirtytwo(0xf6bb, 0x4b60), new Thirtytwo(0xbebf, 0xbc70),
    new Thirtytwo(0x289b, 0x7ec6), new Thirtytwo(0xeaa1, 0x27fa),
    new Thirtytwo(0xd4ef, 0x3085), new Thirtytwo(0x488, 0x1d05),
    new Thirtytwo(0xd9d4, 0xd039), new Thirtytwo(0xe6db, 0x99e5),
    new Thirtytwo(0x1fa2, 0x7cf8), new Thirtytwo(0xc4ac, 0x5665),
    new Thirtytwo(0xf429, 0x2244), new Thirtytwo(0x432a, 0xff97),
    new Thirtytwo(0xab94, 0x23a7), new Thirtytwo(0xfc93, 0xa039),
    new Thirtytwo(0x655b, 0x59c3), new Thirtytwo(0x8f0c, 0xcc92),
    new Thirtytwo(0xffef, 0xf47d), new Thirtytwo(0x8584, 0x5dd1),
    new Thirtytwo(0x6fa8, 0x7e4f), new Thirtytwo(0xfe2c, 0xe6e0),
    new Thirtytwo(0xa301, 0x4314), new Thirtytwo(0x4e08, 0x11a1),
    new Thirtytwo(0xf753, 0x7e82), new Thirtytwo(0xbd3a, 0xf235),
    new Thirtytwo(0x2ad7, 0xd2bb), new Thirtytwo(0xeb86, 0xd391)];


  MD5Accumulator.prototype.a = new Thirtytwo(0,0);
  MD5Accumulator.prototype.b = new Thirtytwo(0,0);
  MD5Accumulator.prototype.c = new Thirtytwo(0,0);
  MD5Accumulator.prototype.d = new Thirtytwo(0,0);
  MD5Accumulator.prototype.f = new Thirtytwo(0,0);
  MD5Accumulator.prototype.temp = new Thirtytwo(0,0);

  var w = new Array(16);
  for (i = 0; i < 16; i++) {
    w[i] = new Thirtytwo(0, 0);
  }
  MD5Accumulator.prototype.w = w;

  function handle_512_bits() {
    this.a.copy(this.h0);
    this.b.copy(this.h1);
    this.c.copy(this.h2);
    this.d.copy(this.h3);
    var g;
    var i;
    for (i = 0; i < 32; i += 2) {
      w[i >> 1].set_words(this.word_array[i + 1], this.word_array[i]);
    }

    for (i = 0; i < 64; i++) {
      if (i < 16) {
        // f = d xor (b and (c xor d))
        this.f.copy(this.c);
        this.f.xor_equals(this.d);
        this.f.and_equals(this.b);
        this.f.xor_equals(this.d);
        g = i;
      } else if (i < 32) {
        // f = c xor (d and (b xor c))
        this.f.copy(this.b);
        this.f.xor_equals(this.c);
        this.f.and_equals(this.d);
        this.f.xor_equals(this.c);
        g = (5 * i + 1) & 15;
      } else if (i < 48) {
        // f = b xor c xor d
        this.f.copy(this.b);
        this.f.xor_equals(this.c);
        this.f.xor_equals(this.d);
        g = (3 * i + 5) & 15;
      } else {
        // f = c xor (b or (not d))
        this.f.copy(this.d);
        this.f.not();
        this.f.or_equals(this.b);
        this.f.xor_equals(this.c);
        g = (7 * i) & 15;
      }
      this.temp.copy(this.d);
      this.d.copy(this.c);
      this.c.copy(this.b);
      // b += rotate(a + f + k[i] + w[g], r[i])
      this.f.plus_equals(this.a);
      this.f.plus_equals(k[i]);
      this.f.plus_equals(w[g]);
      this.f.rotate_equals(r[i]);
      this.b.plus_equals(this.f);
      this.a.copy(this.temp);
    }
    this.h0.plus_equals(this.a);
    this.h1.plus_equals(this.b);
    this.h2.plus_equals(this.c);
    this.h3.plus_equals(this.d);
  }
  MD5Accumulator.prototype.handle_512_bits = handle_512_bits;


  function add_ascii(str) {
    var i;
    for (i = 0; i < str.length; i++) {
      this.length++;
      var c = str.charCodeAt(i);
      if (this.word_array_fullness & 1) {
        this.word_array[this.word_array_fullness >> 1] |= c << 8;
      } else {
        this.word_array[this.word_array_fullness >> 1] = c;
      }
      this.word_array_fullness++;
      if (this.word_array_fullness == 64) {
        this.handle_512_bits();
        this.word_array_fullness = 0;
      }
    }
  }
  MD5Accumulator.prototype.add_ascii = add_ascii;


  function add_word(word) {
    this.word_array[this.word_array_fullness >> 1] = word;
    this.word_array_fullness += 2;
  }
  MD5Accumulator.prototype.add_word = add_word;


  function get_sum() {
    var saved_length = this.length;
    this.add_ascii("\u0080");
    if ((this.length & 63) > 56)
      this.add_ascii("\0\0\0\0\0\0\0\0");
    while ((this.length & 63) < 56)
      this.add_ascii("\0");
    // Saved length is in bytes.
    this.add_word((saved_length <<  3) & 0xffff);
    this.add_word((saved_length >>> 13) & 0xffff)
    var high_word = Math.floor(saved_length / 0x20000000);
    this.add_word(high_word & 0xffff);
    this.add_word((high_word >>> 16) & 0xffff)
    this.handle_512_bits();
    return this.h0.to_hex() + this.h1.to_hex() +
           this.h2.to_hex() + this.h3.to_hex();
  }
  MD5Accumulator.prototype.get_sum = get_sum;
})();


var plainText = "Rebellious subjects, enemies to peace,\n\
Profaners of this neighbour-stained steel,--\n\
Will they not hear? What, ho! you men, you beasts,\n\
That quench the fire of your pernicious rage\n\
With purple fountains issuing from your veins,\n\
On pain of torture, from those bloody hands\n\
Throw your mistemper'd weapons to the ground,\n\
And hear the sentence of your moved prince.\n\
Three civil brawls, bred of an airy word,\n\
By thee, old Capulet, and Montague,\n\
Have thrice disturb'd the quiet of our streets,\n\
And made Verona's ancient citizens\n\
Cast by their grave beseeming ornaments,\n\
To wield old partisans, in hands as old,\n\
Canker'd with peace, to part your canker'd hate:\n\
If ever you disturb our streets again,\n\
Your lives shall pay the forfeit of the peace.\n\
For this time, all the rest depart away:\n\
You Capulet; shall go along with me:\n\
And, Montague, come you this afternoon,\n\
To know our further pleasure in this case,\n\
To old Free-town, our common judgment-place.\n\
Once more, on pain of death, all men depart.\n"

for (var i = 0; i <4; i++) {
    plainText += plainText;
}

function benchmark() {
  var md5 = new MD5Accumulator();
  md5.add_ascii(plainText);
  var md5Output = md5.get_sum();

  if (md5Output != "1b8719c72d5d8bfd06e096ef6c6288c5")
    throw new Error("md5 calculation failed");
}

var elapsed = 0;
var start = new Date();
for (var n = 0; elapsed < 2000; n++) {
  benchmark();
  elapsed = new Date() - start;
}
print('Time (crypto-md5-optimized): ' + Math.floor(1000 * elapsed/n) + ' us.');
