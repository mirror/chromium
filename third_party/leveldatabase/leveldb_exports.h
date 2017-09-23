// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_LEVELDATABASE_LEVELDB_EXPORTS_H_
#define THIRD_PARTY_LEVELDATABASE_LEVELDB_EXPORTS_H_

#if defined(COMPONENT_BUILD) && !defined(COMPILE_LEVELDB_STATICALLY)
#if defined(WIN32)

#if defined(LEVELDB_IMPLEMENTATION)
#define LEVELDB_EXPORT __declspec(dllexport)
#else
#define LEVELDB_EXPORT __declspec(dllimport)
#endif  // defined(LEVELDB_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(LEVELDB_IMPLEMENTATION)
#define LEVELDB_EXPORT __attribute__((visibility("default")))
#else
#define LEVELDB_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define LEVELDB_EXPORT
#endif

#endif  // THIRD_PARTY_LEVELDATABASE_LEVELDB_EXPORTS_H_
