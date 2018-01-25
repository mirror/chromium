// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/common/origin_trials/trial_token.h"

#include <memory>

#include "base/macros.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/test/simple_test_clock.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace blink {

namespace {

// This is a sample public key for testing the API. The corresponding private
// key (use this to generate new samples for this test file) is:
//
//  0x83, 0x67, 0xf4, 0xcd, 0x2a, 0x1f, 0x0e, 0x04, 0x0d, 0x43, 0x13,
//  0x4c, 0x67, 0xc4, 0xf4, 0x28, 0xc9, 0x90, 0x15, 0x02, 0xe2, 0xba,
//  0xfd, 0xbb, 0xfa, 0xbc, 0x92, 0x76, 0x8a, 0x2c, 0x4b, 0xc7, 0x75,
//  0x10, 0xac, 0xf9, 0x3a, 0x1c, 0xb8, 0xa9, 0x28, 0x70, 0xd2, 0x9a,
//  0xd0, 0x0b, 0x59, 0xe1, 0xac, 0x2b, 0xb7, 0xd5, 0xca, 0x1f, 0x64,
//  0x90, 0x08, 0x8e, 0xa8, 0xe0, 0x56, 0x3a, 0x04, 0xd0
//
//  This private key can also be found in tools/origin_trials/eftest.key in
//  binary form. Please update that if changing the key.
//
//  To use this with a real browser, use --origin-trial-public-key with the
//  public key, base-64-encoded:
//  --origin-trial-public-key=dRCs+TocuKkocNKa0AtZ4awrt9XKH2SQCI6o4FY6BNA=
const uint8_t kTestPublicKey[] = {
    0x75, 0x10, 0xac, 0xf9, 0x3a, 0x1c, 0xb8, 0xa9, 0x28, 0x70, 0xd2,
    0x9a, 0xd0, 0x0b, 0x59, 0xe1, 0xac, 0x2b, 0xb7, 0xd5, 0xca, 0x1f,
    0x64, 0x90, 0x08, 0x8e, 0xa8, 0xe0, 0x56, 0x3a, 0x04, 0xd0,
};

// This is a valid, but incorrect, public key for testing signatures against.
// The corresponding private key is:
//
//  0x21, 0xee, 0xfa, 0x81, 0x6a, 0xff, 0xdf, 0xb8, 0xc1, 0xdd, 0x75,
//  0x05, 0x04, 0x29, 0x68, 0x67, 0x60, 0x85, 0x91, 0xd0, 0x50, 0x16,
//  0x0a, 0xcf, 0xa2, 0x37, 0xa3, 0x2e, 0x11, 0x7a, 0x17, 0x96, 0x50,
//  0x07, 0x4d, 0x76, 0x55, 0x56, 0x42, 0x17, 0x2d, 0x8a, 0x9c, 0x47,
//  0x96, 0x25, 0xda, 0x70, 0xaa, 0xb9, 0xfd, 0x53, 0x5d, 0x51, 0x3e,
//  0x16, 0xab, 0xb4, 0x86, 0xea, 0xf3, 0x35, 0xc6, 0xca
const uint8_t kTestPublicKey2[] = {
    0x50, 0x07, 0x4d, 0x76, 0x55, 0x56, 0x42, 0x17, 0x2d, 0x8a, 0x9c,
    0x47, 0x96, 0x25, 0xda, 0x70, 0xaa, 0xb9, 0xfd, 0x53, 0x5d, 0x51,
    0x3e, 0x16, 0xab, 0xb4, 0x86, 0xea, 0xf3, 0x35, 0xc6, 0xca,
};

// This is a good trial token, signed with the above test private key.
// Generate this token with the command (in tools/origin_trials):
// generate_token.py valid.example.com Frobulate --expire-timestamp=1458766277
const char* kSampleToken =
    "Ap+Q/Qm0ELadZql+dlEGSwnAVsFZKgCEtUZg8idQC3uekkIeSZIY1tftoYdrwhqj"
    "7FO5L22sNvkZZnacLvmfNwsAAABZeyJvcmlnaW4iOiAiaHR0cHM6Ly92YWxpZC5l"
    "eGFtcGxlLmNvbTo0NDMiLCAiZmVhdHVyZSI6ICJGcm9idWxhdGUiLCAiZXhwaXJ5"
    "IjogMTQ1ODc2NjI3N30=";
const uint8_t kSampleTokenSignature[] = {
    0x9f, 0x90, 0xfd, 0x09, 0xb4, 0x10, 0xb6, 0x9d, 0x66, 0xa9, 0x7e,
    0x76, 0x51, 0x06, 0x4b, 0x09, 0xc0, 0x56, 0xc1, 0x59, 0x2a, 0x00,
    0x84, 0xb5, 0x46, 0x60, 0xf2, 0x27, 0x50, 0x0b, 0x7b, 0x9e, 0x92,
    0x42, 0x1e, 0x49, 0x92, 0x18, 0xd6, 0xd7, 0xed, 0xa1, 0x87, 0x6b,
    0xc2, 0x1a, 0xa3, 0xec, 0x53, 0xb9, 0x2f, 0x6d, 0xac, 0x36, 0xf9,
    0x19, 0x66, 0x76, 0x9c, 0x2e, 0xf9, 0x9f, 0x37, 0x0b};

// This is a good subdomain trial token, signed with the above test private key.
// Generate this token with the command (in tools/origin_trials):
// generate_token.py example.com Frobulate --is-subdomain
//   --expire-timestamp=1458766277
const char* kSampleSubdomainToken =
    "Auu+j9nXAQoy5+t00MiWakZwFExcdNC8ENkRdK1gL4OMFHS0AbZCscslDTcP1fjN"
    "FjpbmQG+VCPk1NrldVXZng4AAABoeyJvcmlnaW4iOiAiaHR0cHM6Ly9leGFtcGxl"
    "LmNvbTo0NDMiLCAiaXNTdWJkb21haW4iOiB0cnVlLCAiZmVhdHVyZSI6ICJGcm9i"
    "dWxhdGUiLCAiZXhwaXJ5IjogMTQ1ODc2NjI3N30=";
const uint8_t kSampleSubdomainTokenSignature[] = {
    0xeb, 0xbe, 0x8f, 0xd9, 0xd7, 0x01, 0x0a, 0x32, 0xe7, 0xeb, 0x74,
    0xd0, 0xc8, 0x96, 0x6a, 0x46, 0x70, 0x14, 0x4c, 0x5c, 0x74, 0xd0,
    0xbc, 0x10, 0xd9, 0x11, 0x74, 0xad, 0x60, 0x2f, 0x83, 0x8c, 0x14,
    0x74, 0xb4, 0x01, 0xb6, 0x42, 0xb1, 0xcb, 0x25, 0x0d, 0x37, 0x0f,
    0xd5, 0xf8, 0xcd, 0x16, 0x3a, 0x5b, 0x99, 0x01, 0xbe, 0x54, 0x23,
    0xe4, 0xd4, 0xda, 0xe5, 0x75, 0x55, 0xd9, 0x9e, 0x0e};

// This is a good trial token, explicitly not a subdomain, signed with the above
// test private key. Generate this token with the command:
// generate_token.py valid.example.com Frobulate --no-subdomain
//   --expire-timestamp=1458766277
const char* kSampleNonSubdomainToken =
    "AreD979D7tO0luSZTr1+/+J6E0SSj/GEUyLK41o1hXFzXw1R7Z1hCDHs0gXWVSu1"
    "lvH52Winvy39tHbsU2gJJQYAAABveyJvcmlnaW4iOiAiaHR0cHM6Ly92YWxpZC5l"
    "eGFtcGxlLmNvbTo0NDMiLCAiaXNTdWJkb21haW4iOiBmYWxzZSwgImZlYXR1cmUi"
    "OiAiRnJvYnVsYXRlIiwgImV4cGlyeSI6IDE0NTg3NjYyNzd9";
const uint8_t kSampleNonSubdomainTokenSignature[] = {
    0xb7, 0x83, 0xf7, 0xbf, 0x43, 0xee, 0xd3, 0xb4, 0x96, 0xe4, 0x99,
    0x4e, 0xbd, 0x7e, 0xff, 0xe2, 0x7a, 0x13, 0x44, 0x92, 0x8f, 0xf1,
    0x84, 0x53, 0x22, 0xca, 0xe3, 0x5a, 0x35, 0x85, 0x71, 0x73, 0x5f,
    0x0d, 0x51, 0xed, 0x9d, 0x61, 0x08, 0x31, 0xec, 0xd2, 0x05, 0xd6,
    0x55, 0x2b, 0xb5, 0x96, 0xf1, 0xf9, 0xd9, 0x68, 0xa7, 0xbf, 0x2d,
    0xfd, 0xb4, 0x76, 0xec, 0x53, 0x68, 0x09, 0x25, 0x06};

const char* kExpectedFeatureName = "Frobulate";
const char* kExpectedOrigin = "https://valid.example.com";
const char* kExpectedSubdomainOrigin = "https://example.com";
const char* kExpectedMultipleSubdomainOrigin =
    "https://part1.part2.part3.example.com";
const uint64_t kExpectedExpiry = 1458766277;

// The token should not be valid for this origin, or for this feature.
const char* kInvalidOrigin = "https://invalid.example.com";
const char* kInsecureOrigin = "http://valid.example.com";
const char* kIncorrectPortOrigin = "https://valid.example.com:444";
const char* kIncorrectDomainOrigin = "https://valid.example2.com";
const char* kInvalidTLDOrigin = "https://com";
const char* kInvalidFeatureName = "Grokalyze";

// The token should be valid if the current time is kValidTimestamp or earlier.
double kValidTimestamp = 1458766276.0;

// The token should be invalid if the current time is kInvalidTimestamp or
// later.
double kInvalidTimestamp = 1458766278.0;

// Well-formed trial token with an invalid signature.
const char* kInvalidSignatureToken =
    "Ap+Q/Qm0ELadZql+dlEGSwnAVsFZKgCEtUZg8idQC3uekkIeSZIY1tftoYdrwhqj"
    "7FO5L22sNvkZZnacLvmfNwsAAABaeyJvcmlnaW4iOiAiaHR0cHM6Ly92YWxpZC5l"
    "eGFtcGxlLmNvbTo0NDMiLCAiZmVhdHVyZSI6ICJGcm9idWxhdGV4IiwgImV4cGly"
    "eSI6IDE0NTg3NjYyNzd9";

// Trial token truncated in the middle of the length field; too short to
// possibly be valid.
const char kTruncatedToken[] =
    "Ap+Q/Qm0ELadZql+dlEGSwnAVsFZKgCEtUZg8idQC3uekkIeSZIY1tftoYdrwhqj"
    "7FO5L22sNvkZZnacLvmfNwsA";

// Trial token with an incorrectly-declared length, but with a valid signature.
const char kIncorrectLengthToken[] =
    "Ao06eNl/CZuM88qurWKX4RfoVEpHcVHWxdOTrEXZkaC1GUHyb/8L4sthADiVWdc9"
    "kXFyF1BW5bbraqp6MBVr3wEAAABaeyJvcmlnaW4iOiAiaHR0cHM6Ly92YWxpZC5l"
    "eGFtcGxlLmNvbTo0NDMiLCAiZmVhdHVyZSI6ICJGcm9idWxhdGUiLCAiZXhwaXJ5"
    "IjogMTQ1ODc2NjI3N30=";

// Trial token with a misidentified version (42).
const char kIncorrectVersionToken[] =
    "KlH8wVLT5o59uDvlJESorMDjzgWnvG1hmIn/GiT9Ng3f45ratVeiXCNTeaJheOaG"
    "A6kX4ir4Amv8aHVC+OJHZQkAAABZeyJvcmlnaW4iOiAiaHR0cHM6Ly92YWxpZC5l"
    "eGFtcGxlLmNvbTo0NDMiLCAiZmVhdHVyZSI6ICJGcm9idWxhdGUiLCAiZXhwaXJ5"
    "IjogMTQ1ODc2NjI3N30=";

const char kSampleTokenJSON[] =
    "{\"origin\": \"https://valid.example.com:443\", \"feature\": "
    "\"Frobulate\", \"expiry\": 1458766277}";

const char kSampleNonSubdomainTokenJSON[] =
    "{\"origin\": \"https://valid.example.com:443\", \"isSubdomain\": false, "
    "\"feature\": \"Frobulate\", \"expiry\": 1458766277}";

const char kSampleSubdomainTokenJSON[] =
    "{\"origin\": \"https://example.com:443\", \"isSubdomain\": true, "
    "\"feature\": \"Frobulate\", \"expiry\": 1458766277}";

// Various ill-formed trial tokens. These should all fail to parse.
const char* kInvalidTokens[] = {
    // Invalid - Not JSON at all
    "abcde",
    // Invalid JSON
    "{",
    // Not an object
    "\"abcde\"", "123.4", "[0, 1, 2]",
    // Missing keys
    "{}", "{\"something\": 1}", "{\"origin\": \"https://a.a\"}",
    "{\"origin\": \"https://a.a\", \"feature\": \"a\"}",
    "{\"origin\": \"https://a.a\", \"expiry\": 1458766277}",
    "{\"feature\": \"FeatureName\", \"expiry\": 1458766277}",
    // Incorrect types
    "{\"origin\": 1, \"feature\": \"a\", \"expiry\": 1458766277}",
    "{\"origin\": \"https://a.a\", \"feature\": 1, \"expiry\": 1458766277}",
    "{\"origin\": \"https://a.a\", \"feature\": \"a\", \"expiry\": \"1\"}",
    "{\"origin\": \"https://a.a\", \"isSubdomain\": \"true\", \"feature\": "
    "\"a\", \"expiry\": 1458766277}",
    "{\"origin\": \"https://a.a\", \"isSubdomain\": 1, \"feature\": \"a\", "
    "\"expiry\": 1458766277}",
    // Negative expiry timestamp
    "{\"origin\": \"https://a.a\", \"feature\": \"a\", \"expiry\": -1}",
    // Origin not a proper origin URL
    "{\"origin\": \"abcdef\", \"feature\": \"a\", \"expiry\": 1458766277}",
    "{\"origin\": \"data:text/plain,abcdef\", \"feature\": \"a\", \"expiry\": "
    "1458766277}",
    "{\"origin\": \"javascript:alert(1)\", \"feature\": \"a\", \"expiry\": "
    "1458766277}",
};

// Large valid token, size = 3052 chars. The feature name is 100 characters, and
// the origin is 2048 chars.
// Generate this token with the command:
// generate_token.py --is-subdomain --expire-timestamp=1458766277 \
//   https://www.<2027 random chars>.com:9999 \
//   ThisTrialNameIs100CharactersLongIncludingPaddingAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
const char kLargeValidToken[] =
    "Aq1wGS1wFPBT/"
    "S0tRbXIhO6fntc3GuDacAPAcfTBMxkdXpgXJMERVcVEVNfZAu1laKHhMUjTp0pOIBi/"
    "KyWXrAQAAAireyJvcmlnaW4iOiAiaHR0cHM6Ly93d3cuYWFhcHB0eXFpbm0wcGFsMWJpZnlpdH"
    "Vkb2h4dThzbWh2Zmx4aTVnbmdsYTVzbWtwbmw2MnFpOWdscWlmanBpZ3V4bDlweHVkYXN6YnVz"
    "Y3Ric3htaTd2MXRyYnhsdWd1YTdpeGp2dXZxM2ZoejVocnVmZmhmN2RocGVxeWN6Zjdzd3Zmcn"
    "hscDBseWhpaHZ0bno3Mnl3a2FucWlqeGZjZ2l4M3d6Z25za3lhODlma2plaGgzZ3h6Y3dmY3Fn"
    "aGI4NnZvNjFxeGhtNG5vc211ZXYyeGN0ajlkNG5hZHR4c3k1bHVld3RjZW5tM2pibWxsd3J1dW"
    "hscGx2ejBmbXdpcm9leGdxc3IxbzJsaTRwYmF5eTJvamt2b3dhaGZ5Z3h1eTJwOWVlYW5zeXNp"
    "YXQ5dzNrdzI1NmVuajRoeHZjZ3A1andleGJnN2liZHk1a2Y1bWlsY2hlZXp4OHhlcnN6Znc2ZW"
    "RweHlzOG15bGl2bzZqcm9ja2JncmhhcWVzZmFod2JycnJoemZzamV4cm13bDF3ZmU1amFub3Rr"
    "cmFidXR2a29tdXJhYXAwMXh0eXJjZThjdW9mZXRtdmFyaGZsYng5dnBhdTB3ZXZvbmV3MG95dz"
    "gya3RlamtwNG9kc3NyM2h2cmh2NGx6ZmZodnBnc2hmbWxoaHl3NWdwejExdzB1ZmhjdzF5N2Fy"
    "MGIwemVnYjZtZ3djYnJ1eGZhNDJmcXJscW50dWdocm9yZzdpNWV0cGhhZ3k0MXNydnUwbmJrdz"
    "k5ZmxybXl4M2JyZTB6a3EyZmtwbGNibzVrZHg3YmQyZWNtd21veXp6c2R1ZnB1dXlucHhlMW13"
    "MGd3NnVoeGljODdsb3VqZXd3Z3RtcXFyOXJlYXkxdjlhZ2dka2dhems0aGJka29yaXVsODhvaH"
    "l1d2h1N2JqbW95aHB5c3Zsb2gxZWZvbm9ybXA1NDFuanRzMXhyZWpvNWo2d21pN2J3bW5obHd5"
    "djJsaWdzdGFkY2k0YXR4MGJpdHJkYmVqYXJ0YnExZGRlNGlmb21wcjBrdHJsdHA5aWNjdm9mYW"
    "ZrYzBjY3RjZWM4NmtmaG5meHNjcjZjcWlqcnV2aWtrdWJva2Zsc2hpcWV1emZ4aTJvbHNjYWUz"
    "a2M5MDJpbjZmOW5qaTNrZGl3bzB2dGN3MzJrbm5qaXhuZm54MjNtOXN2emd6cnFmYmNpYjk2bj"
    "JvZzdzdXhzZTZ2Y2U5a3Q1ZW1qeTVvYXFmcnpjZ3lvaXB3MGdmZWp1b2pvbXlqb3hzb2Nod3l3"
    "eHg1Z2UyMmJndnVrenl2eThqbHk2NmwwaWdwODFvZXI2d2t1bjh5NnljdGR5bnRqZGtsZnF6NH"
    "dqeG14eWduY3RnaGtneDF2eXJmeHlqcTAwNXprdnVkaDBxZnZqc2JyYmo0bng1bTd2Zm1rdnV6"
    "b2k5a2Znb2djMnFhZ294eGFzb3JydXJtZzN2dG5pd3FxbTI4emZoOGp2bTR1MnZhMGhsb2huaW"
    "81dGl0empxenp3dmxiZHpmN3pseXdsZ2JpbHB5eG5tbWlpcnk4Zzdpc3o0dm0zc3ZuanR5YXlj"
    "bm9wOWhzd25ta3Njc2E1NmwwZTlqcjN5eXk5dzd0d2pycTdmcnFkempraHNyMGl1NmFucmFtbW"
    "F5d2dscmdwb29ucTg0OWtvaG8zNWx0aDBzeml1ZzRrb2w4aTdicWtncWNmeG1rNnBoMGZuMjdn"
    "Ymdsdml4dG9ucW14eGhoYzI5bGNodmhraWY4eXZkdmVkYXVybmt4dXB5bHN6N2NucGl2YjR3Ym"
    "dya3JheWhpdzIwaGZiMWtjNGdudGtxaHBpbnR6c21pNDQ3ZnkyOWx5andsamJuc3hhaG1kYWVp"
    "amVnYmM0aGZwbGZhOHdkMHE0cWpieHR1ZnNjaWJiamY2YXNnYjV2dWV6Y2FwenZ4bXhpdGVhc3"
    "R4aXVpenVlaDV4cncxbHk3aHBzYnliaGltbmp6cDB4cDZwb2thc2hlYmhpcTMxZnZnbWVvdG9n"
    "NHY0M2huc3Uzenp0bGRveGxobG9teGttNmZndHh3OHhtOHQ0dTJtemNmMWFwZXNxdXF3ejV5d3"
    "ludXUwb2JyZWkycWlwcW5sdWVzaG01c3AxMndscTZoOW4wdTZ3cmF1cXRkN2hrZnJ1bmFleWVs"
    "NWphNnptZWp6cGJkeXEyNnRncGo5eXY4bThtZXducmZxd29zZ2Y3aG5oNHI3aXZ5OGN4NXgxcG"
    "lpd3NvZHh2ODVjeWltczU3MTE2Zm56YWcya3h0dG16cTc1dXJ6MWEydDBhazR5empxejRkNjZi"
    "ZGJ5dmU0bmJoc2VsN2NwN2h3bnQ4NWIya3RiOGlwZG95Mmc1dmx0b29ydXB5NDN5aWhxdWd0cG"
    "dhcGo2eXIzOWd5eGNlaW9vZ284cGM2c2lqZ2ZoYml1NmZ1aGIwcW9jeGprcHJvOHlycXphaXR3"
    "d3l3b2xnZ2Fzbnl0bnhlazV3aHhwd3ZnMHA1aWRrbHBnNWZjaHlpbXJmYWx1Yzl2N3U0dWJvbW"
    "hkaWlyajF1YjBnaTRlc2dxazhjdHJiajByOXBpNGM3eGoyMTR2bWQ2Mzhub3JxcDNtaGpob2Yw"
    "NHVxbnJjaDRmbnN5emhyZmJlczZhY2VodW9qc2hucjd0OTFvam8xdno1ZHF2am83NzY5amJ5bn"
    "p6dW02amkuY29tOjk5OTkiLCAiaXNTdWJkb21haW4iOiB0cnVlLCAiZmVhdHVyZSI6ICJUaGlz"
    "VHJpYWxOYW1lSXMxMDBDaGFyYWN0ZXJzTG9uZ0luY2x1ZGluZ1BhZGRpbmdBQUFBQUFBQUFBQU"
    "FBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUEiLCAiZXhwaXJ5IjogMTQ1"
    "ODc2NjI3N30=";
const uint8_t kLargeValidTokenSignature[] = {
    0xad, 0x70, 0x19, 0x2d, 0x70, 0x14, 0xf0, 0x53, 0xfd, 0x2d, 0x2d,
    0x45, 0xb5, 0xc8, 0x84, 0xee, 0x9f, 0x9e, 0xd7, 0x37, 0x1a, 0xe0,
    0xda, 0x70, 0x03, 0xc0, 0x71, 0xf4, 0xc1, 0x33, 0x19, 0x1d, 0x5e,
    0x98, 0x17, 0x24, 0xc1, 0x11, 0x55, 0xc5, 0x44, 0x54, 0xd7, 0xd9,
    0x02, 0xed, 0x65, 0x68, 0xa1, 0xe1, 0x31, 0x48, 0xd3, 0xa7, 0x4a,
    0x4e, 0x20, 0x18, 0xbf, 0x2b, 0x25, 0x97, 0xac, 0x04};

// Valid token that is too large, size = 4100 chars. The feature name is 100
// characters, and the origin is 2833 chars.
// Generate this token with the command:
// generate_token.py --is-subdomain --expire-timestamp=1458766277 \
//   https://www.<2812 random chars>.com:9999 \
//   ThisTrialNameIs100CharactersLongIncludingPaddingAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
const char kTooLargeValidToken[] =
    "AlhEg6tINOds/"
    "zM5lcaQj4af+u3RengcVVquJJ9sVDK2Y+J6GVnTqOzYNR3kar+1nAEkLU5FGK+"
    "l2lM6tFZppwkAAAu8eyJvcmlnaW4iOiAiaHR0cHM6Ly93d3cuYWFzc2t5bXp6azhsMW5pamp4c"
    "2ptZmtoc3lxZHdpOWI4bXh5a3Fxdm56cHBzZ3Izb2o4enJuNTJoM3dudTdjeXJ0N2t4ZDZqdXZ"
    "tOWthcDM0M3JwMzF5ZmU4anIwcjhqcDZyZmJnc2t6eTdpeWV2dnhydzNtdXY2ZXljZ2xwbzB2Y"
    "jJtcnhnaWtzZXkyenhnc2Z2YnpvZzZnYmJuYnBkdWczenY1OXhnejM5d3JtY254cmtqeW13czc"
    "yb251cGZraTRoZmJ0dnhueWFrMW5obGsyNHJ2bXl5b3hwNm44aHhrYW5uanJiMjIxbHlkaWpzd"
    "TZkcGF5c2FvZW1vOXg2NGZ1YWx6Z3M1Ym9hYXB0cWNnN3FhMmgwdW5qZDNpZ2Jzb3htbXh5dDV"
    "icjg0dnRhZWs4bDFpdnJyMGJkaWcza3dtd2V3c2p6eXNmbGxvd3E3cXBndGtoOGIycGxscnVya"
    "GZnMmVtNGo2aWJmdm5tamd5aWdraXY2b2VnanpleXVpbWliam5kd2NwcG9xcTJseHAxZjBjcHl"
    "raXo5azE3d25pMWdtMzd0czd0bWt3bmJnZXEwanlnaGFrZzZkMGtjbHdqZ3RuamRwbnl3ajA3c"
    "ThpcWdnc3Z5cmZyZjI1cWlhaTNyazdmdml0eHpsZW1pYTRuNGRnZzlxa2p6dDNhaTh2ZzlycHJ"
    "2eWthdGZva3dqZWxheW0wa254N28xbHh5bWd4bm96bGhveHJwcnFtanF6MGV0Zm50eHZlcDZxc"
    "2o2bTBpYWxhMXVuaGlwNjN5Z2Fzc3praHJoYTRvb2dndmUzZXFxMXB3enliMjcyZ2ZhaTRtMXB"
    "2OHI4MWdpY3Z6YWZqYjY2dWgzZWNjdGZzc3B4bzZrY2JqYmliMnhlcmptYXdwcW54dnZvNXpxY"
    "m54aTZoYzlyanQ3OG12bWM2OXljN2xoYWNpbzBzanBqYWd4cmFscDM5aXl5MjNtMmd3amlrZXR"
    "zc3picHV2cmlnbXN2M3U4YnV3aXVmaWd5OHp5amxrMnZibDQycHcxZXFmMDgyZWp3cHVtcXVvM"
    "2YyZHBrbGJtNm1ub3diZHZ0cG54Zm1zdW9zeDFscXl3dGZ5cjR2b3F4eG54bXVrOXRzc3diOXV"
    "pczY2NTh2bmtiaGU3OW9kOWt4MGJ2N21rbzVlMm93emg3am15N3BzMWhyeHUzcmd1a245MWhwc"
    "3R6eXdyeGZnenlpbXltY2VsZG1lcmk1d3VzeDBtcTVwcmVnZmlpeWJ4end6Z2p3Z2h4ZGhiaDF"
    "hZzQ4cnpraXZ3Y3k1ZG9uanlnczJlM2phamR6NW5xanlrdTJ3dnFvaWs4YXN1ZmppYW1iZnlqb"
    "HVmdWl1OTByajFvenBta2dpZmt3ZjJzbHh6M29xMHNwZjY0ZjRtd3h2MDlvdnZvYmJ0MDhoamJ"
    "zNHF4a3dmZ3B2eXFrM29mbWhlZndlOTEzcGhxYm41NWsxbDZ0bmZmbDV3MGtlMzhiczFpY2t1b"
    "TN5dGpqY29wY2ZsZHB5dm9vaWhpbzY0dWtuaG0xanNwcmFlemdubjl3MHk4Zmc5YnRubTlrdXF"
    "3dWJ1dmFnc3Q2ZTR1bXNpd2FicDU3bnVobGx3bW14OTltdmFhcGcyYmxwc2NvenZ3dXljdmlod"
    "nlzeDZpdnJhbWRoNGdiYWluYmk2eHBhdWswbWFxd2ZhcHFyaWxqdmVheThxenFtZmdiYWVobHV"
    "mcGp2aXBmZGlvcmswY2lxZmF0dTd4dWQ2ZnY4bDlqMmkzNDhidnZvNTV4ZXh2YXVmYnU5bW5wZ"
    "WpqYXEyMHF4OHYxNXJjZmxyZjZmbHptamZhcmI1ZnR2dnV2d3JmdXZkcTZ3d2VmenlxNXFsYTZ"
    "scG1jMG05cGl2OXh3Zno3dTIxZ3R4eXhuZnlpbTdja3N2Nm1kMHQ5aHJxa3p5Z2w2Y2s3Mmt2Z"
    "XRqb2YxZXRsZjNzdG44dnVpM3BqbmNtcXlzNHVkdGplZTRiZmYxZTMwbWt6aHk1bDJjcHBuMTF"
    "maDNiNmt5czFranI0cjUzd2hiNXhhbG91YTd2aWs4ZTNvcG56dGptZXhxZXVueng2ZmVraGVzb"
    "2dheDNqd3J6cGdmbGIxcWZtdW9hYmJjeGpnY3lkbndib3BldWgzYnhpOXpzeDB5NXNvOHRpZjF"
    "qZ3JwbWphNHB2bm9vZ3B3dnk4a3E0bHJ3Y2lncTJqaTE1YnB5Nm9veGU0cHh3cXA1b2lvaGFic"
    "3ZpdWZ2cGFqbjBobHl5amxjZjY4b2FvNm9ycG9wdDF6NXpvamp3ZzNwa2NkdXNzemttbW51Zjh"
    "yYWVzbXZlc3BjbmppcW8weHVneHV1ZXZzZmdoMmZteGU5b3B1bGpsc285dzhmNXdsOG9wdXgza"
    "HRhcGhqMW9rZXI2ajE5aGdtN2JocGc2aXZ1Y2N3eGluN3E3dHZzdXZiMTU4aWxxdWd3YmpzZ2F"
    "4b25ydHpldzlvbzZjanRoaWxseGtiemxyeGl3cW5td3hmbzFuazNlY3dobWh4anZiam5hc2Z0a"
    "jU5dm1zajQwZXhidnQ1bmJtaXcwdHRmZTBqd2d6Y3d0b25wNTlrMG1sdWFvd2U5bWdwbXJ3ZHc"
    "2cmxmeWV3dXhnd3ZsbmowczJwZ2JhYnljMWVtaWRyNmkydzY5cjdnMGtjZXE3ZW9xMmE1Z2lyN"
    "jN2dGl0eGl1Z3R2eXp6aGlvY2pianh4NjMyY21uczZyYmpzOWVvamtoYXV3YnJ5NDh2aHZ6YnU"
    "1b3d5eHNoc2VtY3ZwOGZ4bmJpbGtzc3Vvb2xjcXpmcHl1cWExZmkxc291bHIweXR5aDVwdHR1Z"
    "3hoM29hc3J5MGxvYjVibG1kdmZrMndqcmF3d3k3aWJzZmMxbm5xYmd3Z2c1bGh3YnYzbW1zc2l"
    "xa3R0dmRoeDFrdGIzMzV3ZW12dzFuYmN4eW1la3p1ZmZ5a3l1anpsdm4wdDNjcG42bnVjbWpic"
    "3Y3cGNiZG5tZTZiamRybndhdzRsemdrYTNicHhkb2IyYnJuenpoYXJneHA3eHdhM2FnaW93bzF"
    "5aWF3aG1wM3ltanRsZ24xNXdjcXJ1MDNvb2w5c3J4dHJobnZwcnJoY29yZmU2Nmh5MXp0Y2w5Z"
    "XQ1eXRoMjZyNm5zbGJmamV2cmtuNWZwZmhqamFuN2hxbnJmaHFtcnlsazVrY2lyM3RrZDNscHc"
    "wZ3M2Znl0NHFmamNxYjF1MXNjZnN2YnZybHJrbHpjajhkbmdiaGFhcXpjMDJidWlubHB3ZTZiN"
    "zA3Y2pqcGt6Z3BqMmx2b3pjM2hrbzV0YTNra2dxdGpyc2tocGFmcTMyb3BhbWZpcXBjZjlmY2t"
    "jOWdrZTFwdGR2Z3l5eDdpaXNneGxwbmp5MWJ6a3Zyc2VoNnpnbXBrajZ1bHM2dWJ6ZThhY2lwZ"
    "mN5djB3Y3g5cXFxZmowYXRobmJjM29lZDB4eWlhYXlsY2F2bWNteWY2aXNxdHptZHM2ZmcxN3l"
    "wZ2xtbmx4dmV4Mmt0MWIyZjY3NXZrdmdidmN4dzB1cXBhdDBnYnU4eXd3d2l3aHZ1bGhzM2k0d"
    "GxtdGVyZWVzeXN3M2dncnpva2w0dWlndDFrbmt4bWYyaHhsMzhnZ3NpdHoyYTg5b3NpdDBkbWV"
    "6ajRoZmJwcGwwMnJ3NmFjZ3pjMHJscmw0M2xncGVqam1qYXNic3QweThydDJidXYyaG9mZnJyc"
    "HVzcmUxYWs3dXZ3aW5wNnRlYy5jb206OTk5OSIsICJpc1N1YmRvbWFpbiI6IHRydWUsICJmZWF"
    "0dXJlIjogIlRoaXNUcmlhbE5hbWVJczEwMENoYXJhY3RlcnNMb25nSW5jbHVkaW5nUGFkZGluZ"
    "0FBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQSIsICJ"
    "leHBpcnkiOiAxNDU4NzY2Mjc3fQ==";

}  // namespace

class TrialTokenTest : public testing::TestWithParam<const char*> {
 public:
  TrialTokenTest()
      : expected_origin_(url::Origin::Create(GURL(kExpectedOrigin))),
        expected_subdomain_origin_(
            url::Origin::Create(GURL(kExpectedSubdomainOrigin))),
        expected_multiple_subdomain_origin_(
            url::Origin::Create(GURL(kExpectedMultipleSubdomainOrigin))),
        invalid_origin_(url::Origin::Create(GURL(kInvalidOrigin))),
        insecure_origin_(url::Origin::Create(GURL(kInsecureOrigin))),
        incorrect_port_origin_(url::Origin::Create(GURL(kIncorrectPortOrigin))),
        incorrect_domain_origin_(
            url::Origin::Create(GURL(kIncorrectDomainOrigin))),
        invalid_tld_origin_(url::Origin::Create(GURL(kInvalidTLDOrigin))),
        expected_expiry_(base::Time::FromDoubleT(kExpectedExpiry)),
        valid_timestamp_(base::Time::FromDoubleT(kValidTimestamp)),
        invalid_timestamp_(base::Time::FromDoubleT(kInvalidTimestamp)),
        expected_signature_(
            std::string(reinterpret_cast<const char*>(kSampleTokenSignature),
                        arraysize(kSampleTokenSignature))),
        expected_subdomain_signature_(std::string(
            reinterpret_cast<const char*>(kSampleSubdomainTokenSignature),
            arraysize(kSampleSubdomainTokenSignature))),
        expected_nonsubdomain_signature_(std::string(
            reinterpret_cast<const char*>(kSampleNonSubdomainTokenSignature),
            arraysize(kSampleNonSubdomainTokenSignature))),
        correct_public_key_(
            base::StringPiece(reinterpret_cast<const char*>(kTestPublicKey),
                              arraysize(kTestPublicKey))),
        incorrect_public_key_(
            base::StringPiece(reinterpret_cast<const char*>(kTestPublicKey2),
                              arraysize(kTestPublicKey2))) {}

 protected:
  OriginTrialTokenStatus Extract(const std::string& token_text,
                                 base::StringPiece public_key,
                                 std::string* token_payload,
                                 std::string* token_signature) {
    return TrialToken::Extract(token_text, public_key, token_payload,
                               token_signature);
  }

  OriginTrialTokenStatus ExtractIgnorePayload(const std::string& token_text,
                                              base::StringPiece public_key) {
    std::string token_payload;
    std::string token_signature;
    return Extract(token_text, public_key, &token_payload, &token_signature);
  }

  std::unique_ptr<TrialToken> Parse(const std::string& token_payload) {
    return TrialToken::Parse(token_payload);
  }

  bool ValidateOrigin(TrialToken* token, const url::Origin origin) {
    return token->ValidateOrigin(origin);
  }

  bool ValidateFeatureName(TrialToken* token, const char* feature_name) {
    return token->ValidateFeatureName(feature_name);
  }

  bool ValidateDate(TrialToken* token, const base::Time& now) {
    return token->ValidateDate(now);
  }

  base::StringPiece correct_public_key() { return correct_public_key_; }
  base::StringPiece incorrect_public_key() { return incorrect_public_key_; }

  const url::Origin expected_origin_;
  const url::Origin expected_subdomain_origin_;
  const url::Origin expected_multiple_subdomain_origin_;
  const url::Origin invalid_origin_;
  const url::Origin insecure_origin_;
  const url::Origin incorrect_port_origin_;
  const url::Origin incorrect_domain_origin_;
  const url::Origin invalid_tld_origin_;

  const base::Time expected_expiry_;
  const base::Time valid_timestamp_;
  const base::Time invalid_timestamp_;

  std::string expected_signature_;
  std::string expected_subdomain_signature_;
  std::string expected_nonsubdomain_signature_;

 private:
  base::StringPiece correct_public_key_;
  base::StringPiece incorrect_public_key_;
};

// Test the extraction of the signed payload from token strings. This includes
// checking the included version identifier, payload length, and cryptographic
// signature.

TEST_F(TrialTokenTest, ExtractValidSignature) {
  std::string token_payload;
  std::string token_signature;
  OriginTrialTokenStatus status = Extract(kSampleToken, correct_public_key(),
                                          &token_payload, &token_signature);
  ASSERT_EQ(OriginTrialTokenStatus::kSuccess, status);
  EXPECT_STREQ(kSampleTokenJSON, token_payload.c_str());
  EXPECT_EQ(expected_signature_, token_signature);
}

TEST_F(TrialTokenTest, ExtractSubdomainValidSignature) {
  std::string token_payload;
  std::string token_signature;
  OriginTrialTokenStatus status =
      Extract(kSampleSubdomainToken, correct_public_key(), &token_payload,
              &token_signature);
  ASSERT_EQ(OriginTrialTokenStatus::kSuccess, status);
  EXPECT_STREQ(kSampleSubdomainTokenJSON, token_payload.c_str());
  EXPECT_EQ(expected_subdomain_signature_, token_signature);
}

TEST_F(TrialTokenTest, ExtractNonSubdomainValidSignature) {
  std::string token_payload;
  std::string token_signature;
  OriginTrialTokenStatus status =
      Extract(kSampleNonSubdomainToken, correct_public_key(), &token_payload,
              &token_signature);
  ASSERT_EQ(OriginTrialTokenStatus::kSuccess, status);
  EXPECT_STREQ(kSampleNonSubdomainTokenJSON, token_payload.c_str());
  EXPECT_EQ(expected_nonsubdomain_signature_, token_signature);
}

TEST_F(TrialTokenTest, ExtractInvalidSignature) {
  OriginTrialTokenStatus status =
      ExtractIgnorePayload(kInvalidSignatureToken, correct_public_key());
  EXPECT_EQ(OriginTrialTokenStatus::kInvalidSignature, status);
}

TEST_F(TrialTokenTest, ExtractSignatureWithIncorrectKey) {
  OriginTrialTokenStatus status =
      ExtractIgnorePayload(kSampleToken, incorrect_public_key());
  EXPECT_EQ(OriginTrialTokenStatus::kInvalidSignature, status);
}

TEST_F(TrialTokenTest, ExtractEmptyToken) {
  OriginTrialTokenStatus status =
      ExtractIgnorePayload("", correct_public_key());
  EXPECT_EQ(OriginTrialTokenStatus::kMalformed, status);
}

TEST_F(TrialTokenTest, ExtractShortToken) {
  OriginTrialTokenStatus status =
      ExtractIgnorePayload(kTruncatedToken, correct_public_key());
  EXPECT_EQ(OriginTrialTokenStatus::kMalformed, status);
}

TEST_F(TrialTokenTest, ExtractUnsupportedVersion) {
  OriginTrialTokenStatus status =
      ExtractIgnorePayload(kIncorrectVersionToken, correct_public_key());
  EXPECT_EQ(OriginTrialTokenStatus::kWrongVersion, status);
}

TEST_F(TrialTokenTest, ExtractSignatureWithIncorrectLength) {
  OriginTrialTokenStatus status =
      ExtractIgnorePayload(kIncorrectLengthToken, correct_public_key());
  EXPECT_EQ(OriginTrialTokenStatus::kMalformed, status);
}

TEST_F(TrialTokenTest, ExtractLargeToken) {
  std::string token_payload;
  std::string token_signature;
  OriginTrialTokenStatus status = Extract(
      kLargeValidToken, correct_public_key(), &token_payload, &token_signature);
  ASSERT_EQ(OriginTrialTokenStatus::kSuccess, status);
  std::string expected_signature(
      std::string(reinterpret_cast<const char*>(kLargeValidTokenSignature),
                  arraysize(kLargeValidTokenSignature)));
  EXPECT_EQ(expected_signature, token_signature);
}

TEST_F(TrialTokenTest, ExtractTooLargeToken) {
  OriginTrialTokenStatus status =
      ExtractIgnorePayload(kTooLargeValidToken, correct_public_key());
  EXPECT_EQ(OriginTrialTokenStatus::kMalformed, status);
}

// Test parsing of fields from JSON token.

TEST_F(TrialTokenTest, ParseEmptyString) {
  std::unique_ptr<TrialToken> empty_token = Parse("");
  EXPECT_FALSE(empty_token);
}

TEST_P(TrialTokenTest, ParseInvalidString) {
  std::unique_ptr<TrialToken> empty_token = Parse(GetParam());
  EXPECT_FALSE(empty_token) << "Invalid trial token should not parse.";
}

INSTANTIATE_TEST_CASE_P(, TrialTokenTest, ::testing::ValuesIn(kInvalidTokens));

TEST_F(TrialTokenTest, ParseValidToken) {
  std::unique_ptr<TrialToken> token = Parse(kSampleTokenJSON);
  ASSERT_TRUE(token);
  EXPECT_EQ(kExpectedFeatureName, token->feature_name());
  EXPECT_FALSE(token->match_subdomains());
  EXPECT_EQ(expected_origin_, token->origin());
  EXPECT_EQ(expected_expiry_, token->expiry_time());
}

TEST_F(TrialTokenTest, ParseValidNonSubdomainToken) {
  std::unique_ptr<TrialToken> token = Parse(kSampleNonSubdomainTokenJSON);
  ASSERT_TRUE(token);
  EXPECT_EQ(kExpectedFeatureName, token->feature_name());
  EXPECT_FALSE(token->match_subdomains());
  EXPECT_EQ(expected_origin_, token->origin());
  EXPECT_EQ(expected_expiry_, token->expiry_time());
}

TEST_F(TrialTokenTest, ParseValidSubdomainToken) {
  std::unique_ptr<TrialToken> token = Parse(kSampleSubdomainTokenJSON);
  ASSERT_TRUE(token);
  EXPECT_EQ(kExpectedFeatureName, token->feature_name());
  EXPECT_TRUE(token->match_subdomains());
  EXPECT_EQ(kExpectedSubdomainOrigin, token->origin().Serialize());
  EXPECT_EQ(expected_subdomain_origin_, token->origin());
  EXPECT_EQ(expected_expiry_, token->expiry_time());
}

TEST_F(TrialTokenTest, ValidateValidToken) {
  std::unique_ptr<TrialToken> token = Parse(kSampleTokenJSON);
  ASSERT_TRUE(token);
  EXPECT_TRUE(ValidateOrigin(token.get(), expected_origin_));
  EXPECT_FALSE(ValidateOrigin(token.get(), invalid_origin_));
  EXPECT_FALSE(ValidateOrigin(token.get(), insecure_origin_));
  EXPECT_FALSE(ValidateOrigin(token.get(), incorrect_port_origin_));
  EXPECT_FALSE(ValidateOrigin(token.get(), incorrect_domain_origin_));
  EXPECT_FALSE(ValidateOrigin(token.get(), invalid_tld_origin_));
  EXPECT_TRUE(ValidateFeatureName(token.get(), kExpectedFeatureName));
  EXPECT_FALSE(ValidateFeatureName(token.get(), kInvalidFeatureName));
  EXPECT_FALSE(ValidateFeatureName(
      token.get(), base::ToUpperASCII(kExpectedFeatureName).c_str()));
  EXPECT_FALSE(ValidateFeatureName(
      token.get(), base::ToLowerASCII(kExpectedFeatureName).c_str()));
  EXPECT_TRUE(ValidateDate(token.get(), valid_timestamp_));
  EXPECT_FALSE(ValidateDate(token.get(), invalid_timestamp_));
}

TEST_F(TrialTokenTest, ValidateValidSubdomainToken) {
  std::unique_ptr<TrialToken> token = Parse(kSampleSubdomainTokenJSON);
  ASSERT_TRUE(token);
  EXPECT_TRUE(ValidateOrigin(token.get(), expected_origin_));
  EXPECT_TRUE(ValidateOrigin(token.get(), expected_subdomain_origin_));
  EXPECT_TRUE(ValidateOrigin(token.get(), expected_multiple_subdomain_origin_));
  EXPECT_FALSE(ValidateOrigin(token.get(), insecure_origin_));
  EXPECT_FALSE(ValidateOrigin(token.get(), incorrect_port_origin_));
  EXPECT_FALSE(ValidateOrigin(token.get(), incorrect_domain_origin_));
  EXPECT_FALSE(ValidateOrigin(token.get(), invalid_tld_origin_));
}

TEST_F(TrialTokenTest, TokenIsValid) {
  std::unique_ptr<TrialToken> token = Parse(kSampleTokenJSON);
  ASSERT_TRUE(token);
  EXPECT_EQ(OriginTrialTokenStatus::kSuccess,
            token->IsValid(expected_origin_, valid_timestamp_));
  EXPECT_EQ(OriginTrialTokenStatus::kWrongOrigin,
            token->IsValid(invalid_origin_, valid_timestamp_));
  EXPECT_EQ(OriginTrialTokenStatus::kWrongOrigin,
            token->IsValid(insecure_origin_, valid_timestamp_));
  EXPECT_EQ(OriginTrialTokenStatus::kWrongOrigin,
            token->IsValid(incorrect_port_origin_, valid_timestamp_));
  EXPECT_EQ(OriginTrialTokenStatus::kExpired,
            token->IsValid(expected_origin_, invalid_timestamp_));
}

TEST_F(TrialTokenTest, SubdomainTokenIsValid) {
  std::unique_ptr<TrialToken> token = Parse(kSampleSubdomainTokenJSON);
  ASSERT_TRUE(token);
  EXPECT_EQ(OriginTrialTokenStatus::kSuccess,
            token->IsValid(expected_origin_, valid_timestamp_));
  EXPECT_EQ(OriginTrialTokenStatus::kSuccess,
            token->IsValid(expected_subdomain_origin_, valid_timestamp_));
  EXPECT_EQ(
      OriginTrialTokenStatus::kSuccess,
      token->IsValid(expected_multiple_subdomain_origin_, valid_timestamp_));
  EXPECT_EQ(OriginTrialTokenStatus::kWrongOrigin,
            token->IsValid(incorrect_domain_origin_, valid_timestamp_));
  EXPECT_EQ(OriginTrialTokenStatus::kWrongOrigin,
            token->IsValid(insecure_origin_, valid_timestamp_));
  EXPECT_EQ(OriginTrialTokenStatus::kWrongOrigin,
            token->IsValid(incorrect_port_origin_, valid_timestamp_));
  EXPECT_EQ(OriginTrialTokenStatus::kExpired,
            token->IsValid(expected_origin_, invalid_timestamp_));
}

// Test overall extraction and parsing, to ensure output status matches returned
// token, and signature is provided.
TEST_F(TrialTokenTest, FromValidToken) {
  OriginTrialTokenStatus status;
  std::unique_ptr<TrialToken> token =
      TrialToken::From(kSampleToken, correct_public_key(), &status);
  EXPECT_TRUE(token);
  EXPECT_EQ(OriginTrialTokenStatus::kSuccess, status);
  EXPECT_EQ(expected_signature_, token->signature());
}

TEST_F(TrialTokenTest, FromInvalidSignature) {
  OriginTrialTokenStatus status;
  std::unique_ptr<TrialToken> token =
      TrialToken::From(kSampleToken, incorrect_public_key(), &status);
  EXPECT_FALSE(token);
  EXPECT_EQ(OriginTrialTokenStatus::kInvalidSignature, status);
}

TEST_F(TrialTokenTest, FromMalformedToken) {
  OriginTrialTokenStatus status;
  std::unique_ptr<TrialToken> token =
      TrialToken::From(kIncorrectLengthToken, correct_public_key(), &status);
  EXPECT_FALSE(token);
  EXPECT_EQ(OriginTrialTokenStatus::kMalformed, status);
}

}  // namespace blink
