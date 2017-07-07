/*
 * Copyright (c) 2012, 2017 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "platform/fonts/shaping/ShapeCache.h"
#include "platform/wtf/StringHasher.h"

namespace blink {

SmallStringKey::SmallStringKey(const LChar* characters,
                               unsigned short length,
                               TextDirection direction)
    : length_(length), direction_(static_cast<unsigned>(direction)) {
  DCHECK(length <= kCapacity);
  // We got convert from LChar to UChar.
  for (unsigned short i = 0; i < length; ++i) {
    characters_[i] = characters[i];
  }

  hashString();
}

SmallStringKey::SmallStringKey(const UChar* characters,
                               unsigned short length,
                               TextDirection direction)
    : length_(length), direction_(static_cast<unsigned>(direction)) {
  DCHECK(length <= kCapacity);
  memcpy(characters_, characters, length * sizeof(UChar));
  hashString();
}

void SmallStringKey::hashString() {
  hash_ = StringHasher::ComputeHash(characters_, length_);
}

ShapeCacheEntry* ShapeCache::AddSlowCase(const TextRun& run,
                                         ShapeCacheEntry entry) {
  unsigned length = run.length();
  bool is_new_entry;
  ShapeCacheEntry* value;
  if (length == 1) {
    uint32_t key = run[0];
    // All current codepoints in UTF-32 are bewteen 0x0 and 0x10FFFF,
    // as such use bit 31 (zero-based) to indicate direction.
    if (run.Direction() == TextDirection::kRtl)
      key |= (1u << 31);
    SingleCharMap::AddResult add_result = single_char_map_.insert(key, entry);
    is_new_entry = add_result.is_new_entry;
    value = &add_result.stored_value->value;
  } else {
    SmallStringKey small_string_key;
    if (run.Is8Bit()) {
      small_string_key =
          SmallStringKey(run.Characters8(), length, run.Direction());
    } else {
      small_string_key =
          SmallStringKey(run.Characters16(), length, run.Direction());
    }

    SmallStringMap::AddResult add_result =
        short_string_map_.insert(small_string_key, entry);
    is_new_entry = add_result.is_new_entry;
    value = &add_result.stored_value->value;
  }

  if ((!is_new_entry) || (size() < kMaxSize)) {
    return value;
  }

  // No need to be fancy: we're just trying to avoid pathological growth.
  single_char_map_.clear();
  short_string_map_.clear();

  return nullptr;
}

}  // namespace blink
