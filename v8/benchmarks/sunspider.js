// Suite script for running all the SunSpider benchmarks
// in one go.

const kBenchmarks = [
    '3d-cube.js',
    '3d-morph.js',
    '3d-raytrace.js',
    'access-binary-trees.js',
    'access-fannkuch.js',
    'access-nbody.js',
    'access-nsieve.js',
    'bitops-3bit-bits-in-byte.js',
    'bitops-bits-in-byte.js',
    'bitops-bitwise-and.js',
    'bitops-nsieve-bits.js',
    'controlflow-recursive.js',
    'crypto-aes.js',
    'crypto-md5.js',
    'crypto-sha1.js',
    'date-format-tofte.js',
    'date-format-xparb.js',
    'math-cordic.js',
    'math-partial-sums.js',
    'math-spectral-norm.js',
    'regexp-dna.js',
    'regexp-unicode.js',
    'string-base64.js',
    'string-fasta.js',
    'string-k-nucleotide.js',
    'string-tagcloud.js',
    'string-unpack-code.js',
    'string-validate-input.js'
];

function RunAll() {
  for (var i = 0; i < kBenchmarks.length; i++) {
    gc();
    load(kBenchmarks[i]);
  }
}

RunAll();
