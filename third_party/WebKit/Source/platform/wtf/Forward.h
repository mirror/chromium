/*
 *  Copyright (C) 2006, 2009, 2011 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef WTF_Forward_h
#define WTF_Forward_h

#include "platform/wtf/Compiler.h"
#include <stddef.h>

namespace WTF {

template <typename T>
class RefPtr;
template <typename T>
class StringBuffer;
class PartitionAllocator;
template <typename T>
struct DefaultHash;
template <typename T>
struct HashTraits;
template <typename ValueArg, typename Allocator>
class LinkedHashSetNode;
template <typename ValueArg, size_t inlineCapacity>
struct ListHashSetAllocator;

template <typename T,
          size_t inlineCapacity = 0,
          typename Allocator = PartitionAllocator>
class Deque;

template <typename Value,
          typename HashFunctions = typename DefaultHash<Value>::Hash,
          typename Traits = HashTraits<Value>,
          typename Allocator = PartitionAllocator>
class HashCountedSet;

template <typename KeyArg,
          typename MappedArg,
          typename HashArg = typename DefaultHash<KeyArg>::Hash,
          typename KeyTraitsArg = HashTraits<KeyArg>,
          typename MappedTraitsArg = HashTraits<MappedArg>,
          typename Allocator = PartitionAllocator>
class HashMap;

template <typename ValueArg,
          typename HashArg = typename DefaultHash<ValueArg>::Hash,
          typename TraitsArg = HashTraits<ValueArg>,
          typename Allocator = PartitionAllocator>
class HashSet;

template <typename Key,
          typename Value,
          typename Extractor,
          typename HashFunctions,
          typename Traits,
          typename KeyTraits,
          typename Allocator>
class HashTable;

template <typename KeyTypeArg, typename ValueTypeArg>
struct KeyValuePair;

template <typename ValueArg,
          typename HashFunctions = typename DefaultHash<ValueArg>::Hash,
          typename TraitsArg = HashTraits<ValueArg>,
          typename Allocator = PartitionAllocator>
class LinkedHashSet;

template <typename ValueArg,
          size_t inlineCapacity = 256,
          typename HashArg = typename DefaultHash<ValueArg>::Hash,
          typename AllocatorArg =
              ListHashSetAllocator<ValueArg, inlineCapacity>>
class ListHashSet;

template <typename T,
          size_t inlineCapacity = 0,
          typename Allocator = PartitionAllocator>
class Vector;

class ArrayBuffer;
class ArrayBufferView;
class ArrayPiece;
class AtomicString;
class CString;
class Float32Array;
class Float64Array;
class Int8Array;
class Int16Array;
class Int32Array;
class OrdinalNumber;
class String;
class StringBuilder;
class StringImpl;
class StringView;
class Uint8Array;
class Uint8ClampedArray;
class Uint16Array;
class Uint32Array;

}  // namespace WTF

using WTF::Deque;
using WTF::HashCountedSet;
using WTF::HashMap;
using WTF::HashSet;
using WTF::HashTable;
using WTF::LinkedHashSet;
using WTF::ListHashSet;
using WTF::RefPtr;
using WTF::Vector;

using WTF::ArrayBuffer;
using WTF::ArrayBufferView;
using WTF::ArrayPiece;
using WTF::AtomicString;
using WTF::CString;
using WTF::Float32Array;
using WTF::Float64Array;
using WTF::Int8Array;
using WTF::Int16Array;
using WTF::Int32Array;
using WTF::String;
using WTF::StringBuffer;
using WTF::StringBuilder;
using WTF::StringImpl;
using WTF::StringView;
using WTF::Uint8Array;
using WTF::Uint8ClampedArray;
using WTF::Uint16Array;
using WTF::Uint32Array;

#endif  // WTF_Forward_h
