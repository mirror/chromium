// When the fuzzer runs it writes "PRNG state set to xxx".  You can restart the
// test at that point by changing this seed.
var SEED = 156;

var VERBOSE_REGEXP_FUZZ = false;

//*****************************************************************************
//  Section 1/2:  The Regexp fuzzer
//  This section Copyright (c) 2008 Google
//*****************************************************************************


var illegal_error = /internal error/;


// This function generates a random regexp by doing a recursive
// descent generation based on the grammar in the spec.  Not all
// regexps generated will be valid since the grammar is interpreted
// sloppily.  This is by design in order to test the error reporting
// and because it's easier.
//
// At the start of the generate the probablilities are set to
// some initial values that determine the nature of the regexp.  Some
// regexps will have lots of backreferences, others lots of noncapturing
// brackets etc.  As time goes, the probabilities are adjusted towards
// zero.  This ensures that generation terminates because the individual
// generating functions are designed to return shorter (nonrecursing)
// answers when their probability test returns false.

function generateRegexp() {
  // Names of the probability variables (in probs) that determine the
  // character of the regexp being generated.  All variables must be listed
  // here.
  var probabilityNames = new Array(
    "disjunction", "alternative", "term", "term2", "term3", "assertion",
    "assertion2", "assertion3", "quantifier", "quantifierPrefix",
    "quantifierPrefix2", "quantifierPrefix3", "quantifierPrefix4",
    "decimalDigits", "simple_atom", "atom", "atom2", "atom3", "atom4",
    "atom5", "atom6", "atom7", "patternCharacters", "patternCharacter",
    "atomEscape", "atomEscape2", "characterEscape", "characterEscape2",
    "characterEscape3", "characterEscape4", "hexDigit", "hexDigit2",
    "controlEscape", "controlEscape2", "controlEscape3", "controlEscape4",
    "controlLetter", "identityEscape", "CharacterClassEscape",
    "CharacterClassEscape2", "CharacterClassEscape3",
    "CharacterClassEscape4", "CharacterClassEscape5",
    "characterClass", "classRanges", "nonemptyClassRanges",
    "nonemptyClassRanges2", "nonemptyClassRangesNoDash",
    "nonemptyClassRangesNoDash2", "classAtom", "classEscape", "classEscape2",
   
    "classEscape3");

  var probs = new Object();

  for (var i = 0; i < probabilityNames.length; i++) {
    probs[probabilityNames[i]] = prngZeroToOne();
  }

  return disjunction();

  // Inner functions.
  function prob(name) {
    probs[name] *= 0.99;
    return prngZeroToOne() > probs[name];
  }


  // Section 15.10 of ECMA spec (regexp grammar).


  function disjunction() {
    if (prob("disjunction"))
      return alternative();
    else
      return alternative() + disjunction();
  }


  function alternative() {
    if (prob("alternative"))
      return "";
    else return alternative() + term();
  }


  function term() {
    if (prob("term"))
      return atom();
    else if (prob("term2" && prob("term3")))
      return assertion();
    else
      return atom() + quantifier();
  }


  function assertion() {
    if (prob("assertion")) {
      if (prob("assertion2"))
        return "^";
      else
        return "$";
    } else {
      if (prob("assertion3"))
        return "\\b";
      else
        return "\\B";
    }
  }


  function quantifier() {
    if (prob("quantifier"))
      return quantifierPrefix();
    else
      return quantifierPrefix() + "?";
  }


  function quantifierPrefix() {
    if (prob("quantifierPrefix")) {
      if (prob("quantifierPrefix2"))
        return "*";
      else
        return "+";
    } else {
      if (prob("quantifierPrefix3")) {
        return "{" + decimalDigits() + "}";
      } else {
        if (prob("quantifierPrefix4"))
          return "{" + decimalDigits() + ",}";
        else
          return "{" + decimalDigits() + "," + decimalDigits() + "}";
      }
    }
  }


  function decimalDigits() {
    if (prob("decimalDigits"))
      return decimalDigit();
    else
      return decimalDigit() + decimalDigits();
  }


  function decimalDigit() {
    return String.fromCharCode(48 + prng(10));
  }


  function atom() {
    if (prob("simple_atom"))
      return patternCharacters();
    if (prob("atom")) {
      if (prob("atom2")) {
        if (prob("atom3"))
          return patternCharacter();
        else
          return ".";
      } else {
        if (prob("atom4"))
          return "\\" + atomEscape();
        else
          return characterClass();
      }
    } else {
      if (prob("atom5")) {
        if (prob("atom6"))
          return "(" + disjunction() + ")";
        else
          return "(?:" + disjunction() + ")";
      } else {
        if (prob("atom7"))
          return "(?=" + disjunction() + ")";
        else
          return "(?!" + disjunction() + ")";
      }
    }
  }


  function patternCharacters() {
    if (prob("patternCharacters"))
      return patternCharacter();
    else
      return patternCharacter() + patternCharacters();
  }


  function patternCharacter() {
    if (prob("patternCharacter")) {
      return String.fromCharCode(prng(65536));
    } else {
      return String.fromCharCode(prng(95) + 32);
    }
  }


  function atomEscape() {
    if (prob("atomEscape")) {
      if (prob("atomEscape2"))
        return decimalDigits();
      else
        return characterEscape();
    } else {
      return CharacterClassEscape();
    }
  }


  function characterEscape() {
    if (prob("characterEscape")) {
      if (prob("characterEscape2")) {
        if (prob("characterEscape3"))
          return controlEscape();
        else
          return "c" + controlLetter();
      } else {
        return hexEscapeSequence();
      }
    } else {
      if (prob("characterEscape4"))
        return unicodeEscapeSequence();
      else
        return identityEscape();
    }
  }


  function unicodeEscapeSequence() {
    return "u" + hexDigit() + hexDigit() + hexDigit() + hexDigit();
  }


  function hexEscapeSequence() {
    return "x" + hexDigit() + hexDigit();
  }


  function hexDigit() {
    if (prob("hexDigit"))
      return decimalDigit();
    if (prob("hexDigit2"))
      return String.fromCharCode(65 + prng(6));
    else
      return String.fromCharCode(97 + prng(6));
  }


  function controlEscape() {
    if (prob("controlEscape")) {
      if (prob("controlEscape2"))
        return "f";
      else
        return "n";
    } else {
      if (prob("controlEscape3")) {
        return "r";
      } else {
        if (prob("controlEscape4"))
          return "t";
        else
          return "v";
      }
    }
  }


  function controlLetter() {
    if (prob("controlLetter"))
      return String.fromCharCode(prng(26) + 65);
    else
      return String.fromCharCode(prng(26) + 97);
  }


  function identityEscape() {
    if (prob("identityEscape")) {
      return String.fromCharCode(prng(65536));
    } else {
      return String.fromCharCode(prng(95) + 32);
    }
  }


  function CharacterClassEscape() {
    if (prob("CharacterClassEscape")) {
      if (prob("CharacterClassEscape2")) {
        if (prob("CharacterClassEscape3"))
          return "d"
        else
          return "D";
      } else {
        if (prob("CharacterClassEscape4"))
          return "s";
        else
          return "S";
      }
    } else {
      if (prob("CharacterClassEscape5")) {
        return "w";
      } else {
        return "W";
      }
    }
  }


  function characterClass() {
    if (prob("characterClass"))
      return "[" + classRanges() + "]";
    else
      return "[^" + classRanges() + "]";
  }


  function classRanges() {
    if (prob("classRanges"))
      return "";
    else
      return nonemptyClassRanges();
  }


  function nonemptyClassRanges() {
    if (prob("nonemptyClassRanges")) {
      if (prob("nonemptyClassRanges2"))
        return classAtom();
      else
        return classAtom() + nonemptyClassRangesNoDash();
    } else {
      return classAtom() + "-" + classAtom() + classRanges();
    }
  }


  function nonemptyClassRangesNoDash() {
    if (prob("nonemptyClassRangesNoDash")) {
      if (prob("nonemptyClassRangesNoDash2"))
        return classAtom();
      else
        return classAtom() + nonemptyClassRangesNoDash();
    } else {
      return classAtom() + "-" + classAtom() + classRanges();
    }
  }


  function classAtom() {
    if (prob("classAtom"))
      return "-";
    else
      return "\\" + classEscape();
  }


  function classEscape() {
    if (prob("classEscape")) {
      if (prob("classEscape2"))
        return decimalDigits();
      else
        return "b"
    } else {
      if (prob("classEscape3"))
        return characterEscape();
      else
        return CharacterClassEscape();
    }
  }
}


var prngState = SEED - 1;
var twister;
initializePrng(prngState, true);


function initializePrng(s, silent) {
  twister = new MersenneTwister19937;
  twister.init_genrand(prngState);
  if (!silent)
    print("PRNG state set to " + prngState);
}


function prng(modulus) {
  return Math.floor(twister.genrand_res53() * modulus);
}


function prngZeroToOne() {
  return twister.genrand_res53();
}


var lastOperation;

function startOver() {
  lastOperation = "start over";
  while (true) {
    var answer = generateRegexp();
    try {
      var compiled = new RegExp(answer);
      answer.match(compiled);
      return answer;
    } catch (e) {
      if (e.toString().match(illegal_error)) {
        print(e);
        throw e;
      }
    }
  }
}


function chop(re) {
  switch (prng(4)) {
    case 0:
      lastOperation = "chop 0";
      return re.substring(prng(re.length));
    case 1:
      lastOperation = "chop 1";
      return re.substring(0, prng(re.length));
    case 2:
      lastOperation = "chop 2";
      return re.substring(prng(re.length)) + re.substring(0, prng(re.length));
    case 3:
      lastOperation = "chop 3";
      var start = prng(re.length);
      return re.substring(start, prng(re.length - start));
  }
}


function add(re) {
  switch (prng(3)) {
    case 0:
      lastOperation = "add 0";
      return re + generateRegexp();
    case 1:
      lastOperation = "add 1";
      return generateRegexp() + re;
    case 2:
      lastOperation = "add 2";
      var at = prng(re.length);
      return re.substring(0, at) + generateRegexp() + re.substring(at);
  }
}


function upAnte(re) {
  var meta_chars = "~@#$%^&*()-_=+{[}]\\|;:'\",<.>/?";
  var brackets = "()[]{}";
  var pos = prng(re.length);
  switch (prng(6)) {
    case 0:
      lastOperation = "add meta";
      return re.substring(0, pos) + meta_chars[prng(meta_chars.length)] + re.substring(pos);
    case 1:
      lastOperation = "add brackets";
      var pos2 = prng(re.length - pos);
      var bracket_type = prng(brackets.length >> 1);
      return re.substring(0, pos) + brackets[bracket_type * 2] + re.substring(pos, pos2) + brackets[bracket_type * 2 + 1] + re.substring(pos + pos2);
    case 2:
      lastOperation = "add number";
      return re.substring(0, pos) + prng(10) + re.substring(pos);
    case 3:
      lastOperation = "add backreference";
      return re.substring(0, pos) + "\\" + prng(10) + re.substring(pos);
    case 4:
      lastOperation = "add {}";
      if (prng(2)) {
        return re.substring(0, pos) + "{" + prng(1000) + "}" + re.substring(pos);
      } else {
        return re.substring(0, pos) + "{" + prng(1000) + "," + prng(1000) + "}" + re.substring(pos);
      }
    case 5:
      lastOperation = "add unicode";
      return re.substring(0, pos) + String.fromCharCode(prng(65536)) + re.substring(pos);
  }
}


var re = startOver();
var compileable = 0;


while (true) {
  while (!prng(1000)) {
    prngState++;
    initializePrng(prngState, false);
    re = startOver();
    if (compileable) {
      print("compileable regexps: " + compileable);
    }
    compileable = 0;
  }
  var new_re;
  lastOperation = "";
  switch (prng(7)) {
    case 0:
      new_re = chop(re);
      break;
    case 1:
    case 2:
    case 3:
      new_re = add(re);
      break;
    case 4:
    case 5:
    case 6:
      new_re = upAnte(re);
      break;
  }
  try {
    if (VERBOSE_REGEXP_FUZZ)
      print(new_re);
    var compiled = new RegExp(new_re);
    new_re.match(compiled);
    compileable++;
    re = new_re;
  } catch (e) {
    if (e.toString().match(illegal_error)) {
      print(e);
      throw e;
    }
  }
}


//*****************************************************************************
//  Section 2/2: The random number generator with associated license.
//*****************************************************************************

// this program is a JavaScript version of Mersenne Twister, with concealment and encapsulation in class,
// an almost straight conversion from the original program, mt19937ar.c,
// translated by y. okada on July 17, 2006.
// and modified a little at july 20, 2006, but there are not any substantial differences.
// in this program, procedure descriptions and comments of original source code were not removed.
// lines commented with //c// were originally descriptions of c procedure. and a few following lines are appropriate JavaScript descriptions.
// lines commented with /* and */ are original comments.
// lines commented with // are additional comments in this JavaScript version.
// before using this version, create at least one instance of MersenneTwister19937 class, and initialize the each state, given below in c comments, of all the instances.
/*
   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.

   Before using, initialize the state by using init_genrand(seed)
   or init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote
        products derived from this software without specific prior written
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


   Any feedback is very welcome.
   http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
*/

function MersenneTwister19937()
{
	/* Period parameters */
	//c//#define N 624
	//c//#define M 397
	//c//#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
	//c//#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
	//c//#define LOWER_MASK 0x7fffffffUL /* least significant r bits */
	N = 624;
	M = 397;
	MATRIX_A = 0x9908b0df;   /* constant vector a */
	UPPER_MASK = 0x80000000; /* most significant w-r bits */
	LOWER_MASK = 0x7fffffff; /* least significant r bits */
	//c//static unsigned long mt[N]; /* the array for the state vector  */
	//c//static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */
	var mt = new Array(N);   /* the array for the state vector  */
	var mti = N+1;           /* mti==N+1 means mt[N] is not initialized */

	function unsigned32 (n1) // returns a 32-bits unsiged integer from an operand to which applied a bit operator.
	{
		return n1 < 0 ? (n1 ^ UPPER_MASK) + UPPER_MASK : n1;
	}

	function subtraction32 (n1, n2) // emulates lowerflow of a c 32-bits unsiged integer variable, instead of the operator -. these both arguments must be non-negative integers expressible using unsigned 32 bits.
	{
		return n1 < n2 ? unsigned32((0x100000000 - (n2 - n1)) & 0xffffffff) : n1 - n2;
	}

	function addition32 (n1, n2) // emulates overflow of a c 32-bits unsiged integer variable, instead of the operator +. these both arguments must be non-negative integers expressible using unsigned 32 bits.
	{
		return unsigned32((n1 + n2) & 0xffffffff)
	}

	function multiplication32 (n1, n2) // emulates overflow of a c 32-bits unsiged integer variable, instead of the operator *. these both arguments must be non-negative integers expressible using unsigned 32 bits.
	{
		var sum = 0;
		for (var i = 0; i < 32; ++i){
			if ((n1 >>> i) & 0x1){
				sum = addition32(sum, unsigned32(n2 << i));
			}
		}
		return sum;
	}

	/* initializes mt[N] with a seed */
	//c//void init_genrand(unsigned long s)
	this.init_genrand = function (s)
	{
		//c//mt[0]= s & 0xffffffff;
		mt[0]= unsigned32(s & 0xffffffff);
		for (mti=1; mti<N; mti++) {
			mt[mti] = 
			//c//(1812433253 * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti);
			addition32(multiplication32(1812433253, unsigned32(mt[mti-1] ^ (mt[mti-1] >>> 30))), mti);
			/* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
			/* In the previous versions, MSBs of the seed affect   */
			/* only MSBs of the array mt[].                        */
			/* 2002/01/09 modified by Makoto Matsumoto             */
			//c//mt[mti] &= 0xffffffff;
			mt[mti] = unsigned32(mt[mti] & 0xffffffff);
			/* for >32 bit machines */
		}
	}

	/* initialize by an array with array-length */
	/* init_key is the array for initializing keys */
	/* key_length is its length */
	/* slight change for C++, 2004/2/26 */
	//c//void init_by_array(unsigned long init_key[], int key_length)
	this.init_by_array = function (init_key, key_length)
	{
		//c//int i, j, k;
		var i, j, k;
		//c//init_genrand(19650218);
		this.init_genrand(19650218);
		i=1; j=0;
		k = (N>key_length ? N : key_length);
		for (; k; k--) {
			//c//mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525))
			//c//	+ init_key[j] + j; /* non linear */
			mt[i] = addition32(addition32(unsigned32(mt[i] ^ multiplication32(unsigned32(mt[i-1] ^ (mt[i-1] >>> 30)), 1664525)), init_key[j]), j);
			mt[i] = 
			//c//mt[i] &= 0xffffffff; /* for WORDSIZE > 32 machines */
			unsigned32(mt[i] & 0xffffffff);
			i++; j++;
			if (i>=N) { mt[0] = mt[N-1]; i=1; }
			if (j>=key_length) j=0;
		}
		for (k=N-1; k; k--) {
			//c//mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941))
			//c//- i; /* non linear */
			mt[i] = subtraction32(unsigned32((dbg=mt[i]) ^ multiplication32(unsigned32(mt[i-1] ^ (mt[i-1] >>> 30)), 1566083941)), i);
			//c//mt[i] &= 0xffffffff; /* for WORDSIZE > 32 machines */
			mt[i] = unsigned32(mt[i] & 0xffffffff);
			i++;
			if (i>=N) { mt[0] = mt[N-1]; i=1; }
		}
		mt[0] = 0x80000000; /* MSB is 1; assuring non-zero initial array */
	}

	/* generates a random number on [0,0xffffffff]-interval */
	//c//unsigned long genrand_int32(void)
	this.genrand_int32 = function ()
	{
		//c//unsigned long y;
		//c//static unsigned long mag01[2]={0x0UL, MATRIX_A};
		var y;
		var mag01 = new Array(0x0, MATRIX_A);
		/* mag01[x] = x * MATRIX_A  for x=0,1 */

		if (mti >= N) { /* generate N words at one time */
			//c//int kk;
			var kk;

			if (mti == N+1)   /* if
			init_genrand() has not been
			called, */
				//c//init_genrand(5489);
				/* a default initial
				seed is used */
				this.init_genrand(5489);
				/* a default initial seed
				is used */

			for (kk=0;kk<N-M;kk++) {
				//c//y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
				//c//mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
				y = unsigned32((mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK));
				mt[kk] = unsigned32(mt[kk+M] ^ (y >>> 1) ^ mag01[y & 0x1]);
			}
			for (;kk<N-1;kk++) {
				//c//y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
				//c//mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
				y = unsigned32((mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK));
				mt[kk] = unsigned32(mt[kk+(M-N)] ^ (y >>> 1) ^ mag01[y & 0x1]);
			}
			//c//y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
			//c//mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];
			y = unsigned32((mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK));
			mt[N-1] = unsigned32(mt[M-1] ^ (y >>> 1) ^ mag01[y & 0x1]);
			mti = 0;
		}

		y = mt[mti++];

		/* Tempering */
		//c//y ^= (y >> 11);
		//c//y ^= (y << 7) & 0x9d2c5680;
		//c//y ^= (y << 15) & 0xefc60000;
		//c//y ^= (y >> 18);
		y = unsigned32(y ^ (y >>> 11));
		y = unsigned32(y ^ ((y << 7) & 0x9d2c5680));
		y = unsigned32(y ^ ((y << 15) & 0xefc60000));
		y = unsigned32(y ^ (y >>> 18));

		return y;
	}

	/* generates a random number on [0,0x7fffffff]-interval */
	//c//long genrand_int31(void)
	this.genrand_int31 = function ()
	{
		//c//return (genrand_int32()>>1);
		return (this.genrand_int32()>>>1);
	}

	/* generates a random number on [0,1]-real-interval */
	//c//double genrand_real1(void)
	this.genrand_real1 = function ()
	{
		//c//return genrand_int32()*(1.0/4294967295.0);
		return this.genrand_int32()*(1.0/4294967295.0);
		/* divided by 2^32-1 */
	}

	/* generates a random number on [0,1)-real-interval */
	//c//double genrand_real2(void)
	this.genrand_real2 = function ()
	{
		//c//return genrand_int32()*(1.0/4294967296.0);
		return this.genrand_int32()*(1.0/4294967296.0);
		/* divided by 2^32 */
	}

	/* generates a random number on (0,1)-real-interval */
	//c//double genrand_real3(void)
	this.genrand_real3 = function ()
	{
		//c//return ((genrand_int32()) + 0.5)*(1.0/4294967296.0);
		return ((this.genrand_int32()) + 0.5)*(1.0/4294967296.0);
		/* divided by 2^32 */
	}

          /* generates a random number on [0,1) with 53-bit resolution*/
          //c//double genrand_res53(void)
          this.genrand_res53 = function ()
          {
                  //c//unsigned long a=genrand_int32()>>5, b=genrand_int32()>>6;
                  var a=this.genrand_int32()>>>5, b=this.genrand_int32()>>>6;
                  return(a*67108864.0+b)*(1.0/9007199254740992.0);
          }
          /* These real versions are due to Isaku Wada, 2002/01/09 added */
}

