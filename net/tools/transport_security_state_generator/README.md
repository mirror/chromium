# Transport Security State Generator

This directory contains the code for the transport security state generator, a
tool that generates a C++ file based on data in
[transport_security_state_static.json](/net/http/transport_security_state_static.json).
This JSON file contains the domain security policies for all preloaded domains.

[TOC]

## Domain Security Policies

Website owners can set a number of security policies for their domains, usually
by sending the configuration in a HTTP header. Website owners can also preload
some of these security policies to better protect their visitors. Chrome
supports preloading for the following domain security policies:

* [HTTP Strict Transport Security (HSTS)](https://tools.ietf.org/html/rfc6797)
* [Public Key Pinning Extension for HTTP](https://tools.ietf.org/html/rfc7469)
* [Expect-CT Extension for HTTP](http://httpwg.org/http-extensions/expect-ct.html)
* [OCSP Expect-Staple](https://docs.google.com/document/d/1aISglJIIwglcOAhqNfK-2vtQl-_dWAapc-VLDh-9-BE/preview)

Chromium and most other browsers ship the preloaded configurations inside their
binary. Chromium uses a custom data structure for this.

### I want to preload a website

Please follow the instructions at [hstspreload.org](https://hstspreload.org/).

## I want to use the preload list for another project

Please contact hstspreload@chromium.org before you do.

## The Preload Generator

The transport security state generator is executed during the build process (it
may execute multiple times depending on the targets you're building) and
generates the data structures that are stored inside the binary. You can find
the generated output in
``[build-folder]/gen/net/http/transport_security_state_static*.h``.

### Usage

Make sure you have build the ``transport_security_state_generator`` target.

``transport_security_state_generator <json-file> <pins-file> <template-file> <output-file> [--v=1]
``

* **json-file**: file containing a JSON structure with all preload configurations (e.g. ``net/http/transport_security_state_static.json``)
* **pins-file**: file containg the public key information for the pinsets referenced in **json-file** (e.g. ``net/http/transport_security_state_static.pins``)
* **template-file**: file that contains the structure for the generate header file with placeholder for the generated data (e.g. ``net/http/transport_security_state_static.template``)
* **output-file**: file to write the output to
* **--v**: verbosity level

## The Preload Format

The preload data is stored in the Chromium binary as a trie encoded in a byte
array (``net::TransportSecurityStateSource::preloaded_data``). The hostnames are
stored in their canonicalized form and compressed using a Huffman coding.

### Huffman Coding

A Huffman coding is calculated for all characters used in the trie (hostnames and the ``end of table`` value and the ``terminal`` value). The Huffman tree can be rebuild from the ``net::TransportSecurityStateSource::huffman_tree`` array.

The (internal) nodes of the tree are encoded as a pairs of uint8s. The last node in the array is the root of the tree. Each pair is two uint8_t values, the first is "left" and the second is "right". If a uint8_t value has the MSB set then it represents a literal leaf value. Otherwise it's a pointer to the n'th element of the array.

For example, the following uint_8 array

```
0xE1, 0xE2, 0xE3, 0x0, 0xE4, 0xE5, 0x1, 0x2
```

contains 8 elements which means the tree contains 4 (internal) nodes (and some leaf values). When decoded this results in the following Huffman tree:

```
             root (node 3)
            /             \
     node 1                 node 2
   /       \               /      \
0xE3 (c)    node 0     0xE4 (d)    0xE5 (e)
           /      \
       0xE1 (a)    0xE2 (b)
```


### The Trie Encoding

The byte array containing the trie is made up of a set of nodes represented as dispatch tables. Each dispatch table contains a (possibly empty) shared prefix, a value, and zero or more pointers to its child dispatch tables. The node value is a preloaded hostname and its domain security configuration.

The trie contains the hostnames in reverse, the hostnames are terminated by the ``terminal value``.

The dispatch table for the root node starts at bit position ``net::TransportSecurityStateSource::root_position``.

```abnf
trie               = 1*dispatch-table

dispatch-table     = prefix-part        ; a common prefix for the node and its children
                     value-part         ; any number of values are pointers to children
                     end-of-table-value ; signals the end of the table

prefix-part        = *%b1               ; series of 1 bits indicating the prefix length
                     %b0                ; 0 bit to indicate the end of the length encoding
                     prefix-characters  ; the actual prefix characters
value-part         = huffman-character  ; node label and prefix
                     1*(node-value / node-pointer)

terminal-value     = huffman-character  ; the ASCII value 0x0 encoded using Huffman
end-of-table-value = huffman-character  ; the ASCII value 0xF encoded using Huffman

prefix-characters  = *huffman-character
huffman-character  = 1*BIT

node-value         = preloaded-entry    ; encoded preload configuration for one hostname
node-pointer       = long-bit-offset / short-bit-offset

long-bit-offset    = %x1                ; 1 bit indicates long form will follow
                     4BIT               ; 4 bit number indicating bit length of the offset
                     8*22BIT            ; offset encoded as n bit number (see above)
short-bit-offset   = %x0                ; 0 bit indicates short form will follow
                     7BIT               ; offset as a 7 bit number

preloaded-entry    = 1*BIT              ; see below
```

### The Preloaded Entry Encoding

The entries are encoded using a variable length encoding. The global structure of an entry is as follows. The numbers between indicate the number of bits for each policy.

```abnf
preloaded-entry         = hsts-part hpkp-part expect-ct-part expect-staple-part

hsts-part               = include-subdomains    ; HSTS IncludeSubdomains flag
                          BIT                   ; whether to force HTTPS

hpkp-part               = BIT                   ; whether to enable pinning
                         [pinset-id]            ; conditional, only present when pinning is enabled
                         [include-subdomains]   ; HPKP IncludeSubdomains flag, conditional, only present
                                                ; when pinning is enabled and HSTS IncludeSubdomains is
                                                ; not used.
hpkp-pinset-id          = array-index

expect-ct-part          = BIT                   ; whether to enable Expect-CT
                          [report-uri-id]

expect-staple-part      = BIT                   ; whether to enable Expect-Staple
                          [include-subdomains report-uri-id]
                                                ; conditional, only present when Expect-Staple is enabled

report-uri-id           = array-index
include-subdomains      = BIT
array-index             = %x0-%xF
```

### Tests

The generator code has its own unittests in the ``net/tools/transport_security_state_generator`` folder.

The encoder and decoder for the preload format life in different places and are
tested by end-to-end tests (``TransportSecurityStateTest.DecodePreload*``) in
``net/http/transport_security_state_unittest.cc``. The tests use their own
preload lists, the data structures for these lists are generated in the same as
for the official Chromium list.

All these tests are part of the ``net_unittests`` target.

## See also

* https://hstspreload.org/
* https://www.chromium.org/hsts
