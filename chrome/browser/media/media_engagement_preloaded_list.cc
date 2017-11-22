// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/media_engagement_preloaded_list.h"

#include "base/files/file_util.h"
#include "base/memory/singleton.h"
#include "base/path_service.h"
#include "chrome/browser/media/media_engagement_preload.pb.h"
#include "url/origin.h"

namespace {

// BitReader is a class that allows a bytestring to be read bit-by-bit.
class BitReader {
 public:
  BitReader(const uint8_t* bytes, size_t num_bits)
      : bytes_(bytes),
        num_bits_(num_bits),
        num_bytes_((num_bits + 7) / 8),
        current_byte_index_(0),
        num_bits_used_(8) {}

  // Next sets |*out| to the next bit from the input. It returns false if no
  // more bits are available or true otherwise.
  bool Next(bool* out) {
    if (num_bits_used_ == 8) {
      if (current_byte_index_ >= num_bytes_) {
        return false;
      }
      current_byte_ = bytes_[current_byte_index_++];
      num_bits_used_ = 0;
    }

    *out = 1 & (current_byte_ >> (7 - num_bits_used_));
    num_bits_used_++;
    return true;
  }

  // Read sets the |num_bits| least-significant bits of |*out| to the value of
  // the next |num_bits| bits from the input. It returns false if there are
  // insufficient bits in the input or true otherwise.
  bool Read(unsigned num_bits, uint32_t* out) {
    DCHECK_LE(num_bits, 32u);

    uint32_t ret = 0;
    for (unsigned i = 0; i < num_bits; ++i) {
      bool bit;
      if (!Next(&bit)) {
        return false;
      }
      ret |= static_cast<uint32_t>(bit) << (num_bits - 1 - i);
    }

    *out = ret;
    return true;
  }

  // Unary sets |*out| to the result of decoding a unary value from the input.
  // It returns false if there were insufficient bits in the input and true
  // otherwise.
  bool Unary(size_t* out) {
    size_t ret = 0;

    for (;;) {
      bool bit;
      if (!Next(&bit)) {
        return false;
      }
      if (!bit) {
        break;
      }
      ret++;
    }

    *out = ret;
    return true;
  }

  // Seek sets the current offest in the input to bit number |offset|. It
  // returns true if |offset| is within the range of the input and false
  // otherwise.
  bool Seek(size_t offset) {
    if (offset >= num_bits_) {
      return false;
    }
    current_byte_index_ = offset / 8;
    current_byte_ = bytes_[current_byte_index_++];
    num_bits_used_ = offset % 8;
    return true;
  }

 private:
  const uint8_t* const bytes_;
  const size_t num_bits_;
  const size_t num_bytes_;
  // current_byte_index_ contains the current byte offset in |bytes_|.
  size_t current_byte_index_;
  // current_byte_ contains the current byte of the input.
  uint8_t current_byte_;
  // num_bits_used_ contains the number of bits of |current_byte_| that have
  // been read.
  unsigned num_bits_used_;
};

// HuffmanDecoder is a very simple Huffman reader. The input Huffman tree is
// simply encoded as a series of two-byte structures. The first byte determines
// the "0" pointer for that node and the second the "1" pointer. Each byte
// either has the MSB set, in which case the bottom 7 bits are the value for
// that position, or else the bottom seven bits contain the index of a node.
//
// The tree is decoded by walking rather than a table-driven approach.
class HuffmanDecoder {
 public:
  HuffmanDecoder(const uint8_t* tree, size_t tree_bytes)
      : tree_(tree), tree_bytes_(tree_bytes) {}

  bool Decode(BitReader* reader, char* out) {
    const uint8_t* current = &tree_[tree_bytes_ - 2];

    for (;;) {
      bool bit;
      if (!reader->Next(&bit)) {
        return false;
      }

      uint8_t b = current[bit];
      if (b & 0x80) {
        *out = static_cast<char>(b & 0x7f);
        return true;
      }

      unsigned offset = static_cast<unsigned>(b) * 2;
      DCHECK_LT(offset, tree_bytes_);
      if (offset >= tree_bytes_) {
        return false;
      }

      current = &tree_[offset];
    }
  }

 private:
  const uint8_t* const tree_;
  const size_t tree_bytes_;
};

static const char kEndOfString = 0;
static const char kEndOfTable = 127;

}  // namespace

// static
MediaEngagementPreloadedList* MediaEngagementPreloadedList::GetInstance() {
  return base::Singleton<MediaEngagementPreloadedList>::get();
}

MediaEngagementPreloadedList::MediaEngagementPreloadedList() {}

MediaEngagementPreloadedList::~MediaEngagementPreloadedList() = default;

bool MediaEngagementPreloadedList::CheckOriginIsPresent(url::Origin origin) {
  bool found = false;
  DCHECK(CheckStringIsPresent(origin.Serialize(), &found));
  return found;
}

bool MediaEngagementPreloadedList::CheckStringIsPresent(std::string input,
                                                        bool* out_found) {
  // Check that we have data to look through.
  if (!IsLoaded())
    return true;

  HuffmanDecoder huffman(huffman_tree_.data(), huffman_tree_size_);
  BitReader reader(trie_data_.data(), trie_bits_);
  size_t bit_offset = root_position_;

  *out_found = false;

  size_t input_size = input.size();
  size_t input_offset = 0;

  for (;;) {
    // Seek to the desired location.
    if (!reader.Seek(bit_offset)) {
      return false;
    }

    // Decode the unary length of the common prefix.
    size_t prefix_length;
    if (!reader.Unary(&prefix_length)) {
      return false;
    }

    // Match each character in the prefix.
    for (size_t i = 0; i < prefix_length; i++) {
      if (input_offset == input_size) {
        // We can't match the terminator with a prefix string.
        return true;
      }

      char c;
      if (!huffman.Decode(&reader, &c)) {
        return false;
      }
      if (input[input_offset] != c) {
        return true;
      }
      input_offset++;
    }

    bool is_first_offset = true;
    size_t current_offset = 0;

    // Go through the dispatch table. This contains the child nodes of the
    // current node in the trie.
    for (;;) {
      // Get the character for this node.
      char c;
      if (!huffman.Decode(&reader, &c)) {
        return false;
      }

      // Reached the end of the dispatch table without finding a valid node.
      if (c == kEndOfTable) {
        return true;
      }

      // If we matched the end of the string then we should either return true
      // if we finished matching the string, or skip this node.
      if (c == kEndOfString) {
        if (input_offset == input_size) {
          *out_found = true;
          return true;
        }

        // There will be no pointer since this is the end of the string.
        continue;
      }

      // The entries in a dispatch table are in order thus we can tell if there
      // will be no match if the current character past the one that we want.
      if (input_offset == input_size || input[input_offset] < c) {
        return true;
      }

      // Read the offset to the next node in the dispatch table.
      if (is_first_offset) {
        // The first offset is backwards from the current position.
        uint32_t jump_delta_bits;
        uint32_t jump_delta;

        if (!reader.Read(5, &jump_delta_bits) ||
            !reader.Read(jump_delta_bits, &jump_delta)) {
          return false;
        }

        if (bit_offset < jump_delta) {
          return false;
        }

        current_offset = bit_offset - jump_delta;
        is_first_offset = false;
      } else {
        // Subsequent offsets are forward from the target of the first offset.
        uint32_t is_long_jump;
        if (!reader.Read(1, &is_long_jump)) {
          return false;
        }
        uint32_t jump_delta;
        if (!is_long_jump) {
          if (!reader.Read(7, &jump_delta)) {
            return false;
          }
        } else {
          uint32_t jump_delta_bits;
          if (!reader.Read(4, &jump_delta_bits) ||
              !reader.Read(jump_delta_bits + 8, &jump_delta)) {
            return false;
          }
        }

        current_offset += jump_delta;
        if (current_offset >= bit_offset) {
          return false;
        }
      }

      // If the current character we are looking at matched this node then
      // we should move to this node and move to the next character.
      DCHECK_LT(input_offset, input_size);
      if (input[input_offset] == c) {
        bit_offset = current_offset;
        input_offset++;
        break;
      }
    }
  }

  return true;
}

bool MediaEngagementPreloadedList::LoadFromFile(base::FilePath path) {
  // Check the file exists.
  if (!base::PathExists(path)) {
    return false;
  }

  // Read the file to a string.
  std::string file_data;
  if (!base::ReadFileToString(path, &file_data)) {
    return false;
  }

  // Load the preloaded list into a proto message.
  chrome_browser_media::PreloadedData message;
  if (!message.ParseFromString(file_data)) {
    return false;
  }

  // If any of the fields are empty we should return false.
  if (!message.huffman_tree_size() || !message.trie_data_size() ||
      !message.has_trie_bits() || !message.has_root_position()) {
    return false;
  }

  // Copy data into huffman tree.
  huffman_tree_.clear();
  huffman_tree_size_ = 0;
  for (int i = 0; i < message.huffman_tree_size(); i++) {
    uint8_t new_num = static_cast<uint8_t>(message.huffman_tree(i));
    huffman_tree_.push_back(new_num);
    huffman_tree_size_ += sizeof(new_num);
  }
  huffman_tree_.shrink_to_fit();

  // Copy data into trie data.
  trie_data_.clear();
  for (int i = 0; i < message.trie_data_size(); i++) {
    uint8_t new_num = static_cast<uint8_t>(message.trie_data(i));
    trie_data_.push_back(new_num);
  }
  trie_data_.shrink_to_fit();

  // Extract the final bits from the proto.
  trie_bits_ = message.trie_bits();
  root_position_ = message.root_position();

  return true;
}
