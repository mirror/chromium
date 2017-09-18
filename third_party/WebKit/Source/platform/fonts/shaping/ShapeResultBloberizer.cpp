// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/fonts/shaping/ShapeResultBloberizer.h"

#include <hb.h>
#include "platform/fonts/Font.h"
#include "platform/fonts/shaping/CachingWordShaper.h"
#include "platform/fonts/shaping/ShapeResult.h"
#include "platform/fonts/shaping/ShapeResultInlineHeaders.h"
#include "platform/text/TextBreakIterator.h"
#include "platform/text/TextRun.h"
#include "platform/wtf/HashMap.h"
#include "platform/wtf/text/UTF8.h"

namespace blink {

ShapeResultBloberizer::ShapeResultBloberizer(const Font& font,
                                             float device_scale_factor,
                                             unsigned flags,
                                             Type type)
    : font_(font), device_scale_factor_(device_scale_factor), flags_(flags), type_(type) {}

bool ShapeResultBloberizer::HasPendingVerticalOffsets() const {
  // We exclusively store either horizontal/x-only ofssets -- in which case
  // m_offsets.size == size, or vertical/xy offsets -- in which case
  // m_offsets.size == size * 2.
  DCHECK(pending_glyphs_.size() == pending_offsets_.size() ||
         pending_glyphs_.size() * 2 == pending_offsets_.size());
  return pending_glyphs_.size() != pending_offsets_.size();
}

bool ShapeResultBloberizer::HasPendingUtf8Text() const {
  // UTF8 conversion might have failed, resulting in offsets mismatch.
  return flags_ & kIncludeUTF8Text
    && pending_glyphs_.size() == pending_character_offsets_.size();
}

void ShapeResultBloberizer::CommitPendingRun() {
  if (pending_glyphs_.IsEmpty())
    return;

  const auto pending_rotation = GetBlobRotation(pending_font_data_);
  if (pending_rotation != builder_rotation_) {
    // The pending run rotation doesn't match the current blob; start a new
    // blob.
    CommitPendingBlob();
    builder_rotation_ = pending_rotation;
  }

  SkPaint run_paint;
  run_paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
  pending_font_data_->PlatformData().SetupPaint(&run_paint,
                                                device_scale_factor_, &font_);

  const auto run_size = pending_glyphs_.size();
  const auto text_size = HasPendingUtf8Text() ? pending_utf8_text_.size() : 0ul;
  SkString lang; // currently unused

  const auto& buffer = HasPendingVerticalOffsets()
      ? builder_.allocRunTextPos(run_paint, run_size, text_size, std::move(lang))
      : builder_.allocRunTextPosH(run_paint, run_size, 0, text_size, std::move(lang));

  std::copy(pending_glyphs_.begin(), pending_glyphs_.end(), buffer.glyphs);
  std::copy(pending_offsets_.begin(), pending_offsets_.end(), buffer.pos);

  if (text_size) {
    printf("*** commit run w/ utf8 text - clusters: %lu, utf8 size: %lu, utf8: >%s<\n",
           pending_character_offsets_.size(), pending_utf8_text_.size(), pending_utf8_text_.begin());
    printf("** clusters: [ ");
    for (auto offset : pending_character_offsets_) printf("%u ", offset);
    printf("]\n");

    std::copy(pending_character_offsets_.begin(),
              pending_character_offsets_.end(),
              buffer.clusters);
    std::copy(pending_utf8_text_.begin(),
              pending_utf8_text_.end(),
              buffer.utf8text);
    pending_character_offsets_.Shrink(0);
    pending_utf8_text_.Shrink(0);
  }

  builder_run_count_ += 1;
  pending_glyphs_.Shrink(0);
  pending_offsets_.Shrink(0);
}

void ShapeResultBloberizer::CommitPendingBlob() {
  if (!builder_run_count_)
    return;

  blobs_.emplace_back(builder_.make(), builder_rotation_);
  builder_run_count_ = 0;
}

const ShapeResultBloberizer::BlobBuffer& ShapeResultBloberizer::Blobs() {
  CommitPendingRun();
  CommitPendingBlob();
  DCHECK(pending_glyphs_.IsEmpty());
  DCHECK_EQ(builder_run_count_, 0u);

  return blobs_;
}

ShapeResultBloberizer::BlobRotation ShapeResultBloberizer::GetBlobRotation(
    const SimpleFontData* font_data) {
  // For vertical upright text we need to compensate the inherited 90deg CW
  // rotation (using a 90deg CCW rotation).
  return (font_data->PlatformData().IsVerticalAnyUpright() &&
          font_data->VerticalData())
             ? BlobRotation::kCCWRotation
             : BlobRotation::kNoRotation;
}

float ShapeResultBloberizer::FillGlyphs(
    const TextRunPaintInfo& run_info,
    const ShapeResultBuffer& result_buffer) {
  if (CanUseFastPath(run_info.from, run_info.to, run_info.run.length(),
                     result_buffer.HasVerticalOffsets())) {
    return FillFastHorizontalGlyphs(result_buffer, run_info.run.Direction());
  }

  float advance = 0;
  auto results = result_buffer.results_;

  if (run_info.run.Rtl()) {
    unsigned word_offset = run_info.run.length();
    for (unsigned j = 0; j < results.size(); j++) {
      unsigned resolved_index = results.size() - 1 - j;
      const RefPtr<const ShapeResult>& word_result = results[resolved_index];
      word_offset -= word_result->NumCharacters();
      advance =
          FillGlyphsForResult(word_result.Get(), run_info.run, run_info.from,
                              run_info.to, advance, word_offset);
    }
  } else {
    unsigned word_offset = 0;
    for (const auto& word_result : results) {
      advance =
          FillGlyphsForResult(word_result.Get(), run_info.run, run_info.from,
                              run_info.to, advance, word_offset);
      word_offset += word_result->NumCharacters();
    }
  }

  return advance;
}

float ShapeResultBloberizer::FillGlyphs(const StringView& text,
                                        unsigned from,
                                        unsigned to,
                                        const ShapeResult* result) {
  DCHECK(result);
  DCHECK(to <= text.length());
  if (CanUseFastPath(from, to, text.length(), result->HasVerticalOffsets()))
    return FillFastHorizontalGlyphs(result);

  float advance = 0;
  float word_offset = 0;
  return FillGlyphsForResult(result, text, from, to, advance, word_offset);
}

void ShapeResultBloberizer::FillTextEmphasisGlyphs(
    const TextRunPaintInfo& run_info,
    const GlyphData& emphasis_data,
    const ShapeResultBuffer& result_buffer) {
  float advance = 0;
  unsigned word_offset = run_info.run.Rtl() ? run_info.run.length() : 0;
  auto results = result_buffer.results_;

  for (unsigned j = 0; j < results.size(); j++) {
    unsigned resolved_index = run_info.run.Rtl() ? results.size() - 1 - j : j;
    const RefPtr<const ShapeResult>& word_result = results[resolved_index];
    for (unsigned i = 0; i < word_result->runs_.size(); i++) {
      unsigned resolved_offset =
          word_offset - (run_info.run.Rtl() ? word_result->NumCharacters() : 0);
      advance += FillTextEmphasisGlyphsForRun(
          word_result->runs_[i].get(), run_info.run,
          run_info.run.CharactersLength(), run_info.run.Direction(),
          run_info.from, run_info.to, emphasis_data, advance, resolved_offset);
    }
    word_offset += word_result->NumCharacters() * (run_info.run.Rtl() ? -1 : 1);
  }
}

void ShapeResultBloberizer::FillTextEmphasisGlyphs(const StringView& text,
                                                   TextDirection direction,
                                                   unsigned from,
                                                   unsigned to,
                                                   const GlyphData& emphasis,
                                                   const ShapeResult* result) {
  float advance = 0;
  unsigned offset = 0;

  for (unsigned i = 0; i < result->runs_.size(); i++) {
    advance += FillTextEmphasisGlyphsForRun(result->runs_[i].get(), text,
                                            text.length(), direction, from, to,
                                            emphasis, advance, offset);
  }
}

namespace {

template <typename TextContainerType>
inline bool IsSkipInkException(const ShapeResultBloberizer& bloberizer,
                               const TextContainerType& text,
                               unsigned character_index) {
  // We want to skip descenders in general, but it is undesirable renderings for
  // CJK characters.
  return bloberizer.GetType() == ShapeResultBloberizer::Type::kTextIntercepts &&
         !text.Is8Bit() &&
         Character::IsCJKIdeographOrSymbol(text.CodepointAt(character_index));
}

inline void AddEmphasisMark(ShapeResultBloberizer& bloberizer,
                            const GlyphData& emphasis_data,
                            FloatPoint glyph_center,
                            float mid_glyph_offset) {
  const SimpleFontData* emphasis_font_data = emphasis_data.font_data;
  DCHECK(emphasis_font_data);

  bool is_vertical =
      emphasis_font_data->PlatformData().IsVerticalAnyUpright() &&
      emphasis_font_data->VerticalData();

  if (!is_vertical) {
    bloberizer.Add(emphasis_data.glyph, emphasis_font_data,
                   mid_glyph_offset - glyph_center.X());
  } else {
    bloberizer.Add(
        emphasis_data.glyph, emphasis_font_data,
        FloatPoint(-glyph_center.X(), mid_glyph_offset - glyph_center.Y()));
  }
}

inline unsigned CountGraphemesInCluster(const UChar* str,
                                        unsigned str_length,
                                        uint16_t start_index,
                                        uint16_t end_index) {
  if (start_index > end_index) {
    uint16_t temp_index = start_index;
    start_index = end_index;
    end_index = temp_index;
  }
  uint16_t length = end_index - start_index;
  DCHECK_LE(static_cast<unsigned>(start_index + length), str_length);
  TextBreakIterator* cursor_pos_iterator =
      CursorMovementIterator(&str[start_index], length);

  int cursor_pos = cursor_pos_iterator->current();
  int num_graphemes = -1;
  while (0 <= cursor_pos) {
    cursor_pos = cursor_pos_iterator->next();
    num_graphemes++;
  }
  return std::max(0, num_graphemes);
}

using Utf8IndexMap = Vector<unsigned, 256>;

template <typename T, typename Func>
size_t ConvertToUtf8AndMapOffsets(const T* input_begin, const T* input_end,
                                  char* output_begin, char* output_end,
                                  Utf8IndexMap& index_map,
                                  Func&& convert) {
  index_map.resize(input_end - input_begin);
  char* output = output_begin;

  for (const T* input = input_begin; input < input_end; ) {
    const unsigned src_index = input - input_begin;
    const unsigned utf8_index = output - output_begin;

    auto result = convert(&input, input + 1, &output, output_end);
    if (result == WTF::Unicode::kSourceExhausted && input + 1 < input_end) {
      DCHECK_EQ(sizeof(T), sizeof(UChar));
      result = convert(&input, input + 2, &output, output_end);
    }

    // Bail if the conversion fails for any reason.
    if (result != WTF::Unicode::kConversionOK) {
      printf("!! conversion failed: %d\n", result);
      return 0ul;
    }

    printf("** mapping l1/u16 index %d -> %d\n", src_index, utf8_index);
    index_map[src_index] = utf8_index;
  }

  return output - output_begin;
}

}  // namespace

template <typename TextContainerType>
void ShapeResultBloberizer::AddUtf8Data(const ShapeResult::RunInfo& run,
                                        const TextContainerType& text,
                                        const Vector<unsigned>& char_offsets,
                                        unsigned run_offset) {
  DCHECK(flags_ & kIncludeUTF8Text);

  const unsigned text_start = run.start_index_ + run_offset;
  const unsigned text_end = text_start + run.num_characters_;
  DCHECK_LE(text_end, text.length());

  printf("*** AddUtf8TextAndBuildIndexMap - text len: %u, start: %u, len: %u\n",
         text.length(), text_start, text_end - text_start);

  Utf8IndexMap index_map;

  const auto initial_utf8_len = pending_utf8_text_.size();
  pending_utf8_text_.Grow(initial_utf8_len + (text_end - text_start) * 4);

  const size_t appended_utf8_len = text.Is8Bit()
      ? ConvertToUtf8AndMapOffsets(text.Characters8() + text_start,
                                   text.Characters8() + text_end,
                                   pending_utf8_text_.begin() + initial_utf8_len,
                                   pending_utf8_text_.end(),
                                   index_map,
                                   [](const LChar** input_begin,
                                   const LChar* input_end,
                                   char** output_begin,
                                   char* output_end) -> WTF::Unicode::ConversionResult {
                                     return WTF::Unicode::ConvertLatin1ToUTF8(input_begin, input_end, output_begin, output_end);
                                   })
      : ConvertToUtf8AndMapOffsets(text.Characters16() + text_start,
                                   text.Characters16() + text_end,
                                   pending_utf8_text_.begin() + initial_utf8_len,
                                   pending_utf8_text_.end(),
                                   index_map,
                                   [](const UChar** input_begin,
                                      const UChar* input_end,
                                      char** output_begin,
                                      char* output_end) -> WTF::Unicode::ConversionResult {
                                        return WTF::Unicode::ConvertUTF16ToUTF8(input_begin, input_end, output_begin, output_end);
                                   });

  pending_utf8_text_.Shrink(initial_utf8_len + appended_utf8_len);

  if (!appended_utf8_len)
    return;

  for (const auto offset : char_offsets) {
    const auto adjusted_utf8_index = index_map[offset] + initial_utf8_len;
    DCHECK_LT(adjusted_utf8_index, pending_utf8_text_.size());
    pending_character_offsets_.push_back(adjusted_utf8_index);
  }
}

template <typename TextContainerType>
float ShapeResultBloberizer::FillGlyphsForResult(const ShapeResult* result,
                                                 const TextContainerType& text,
                                                 unsigned from,
                                                 unsigned to,
                                                 float initial_advance,
                                                 unsigned run_offset) {
  auto total_advance = initial_advance;

  for (const auto& run : result->runs_) {
    Vector<unsigned> char_offsets;

    total_advance = run->ForEachGlyphInRange(
        total_advance, from, to, run_offset,
        [&](const HarfBuzzRunGlyphData& glyph_data, float total_advance,
            uint16_t character_index) -> bool {

          if (!IsSkipInkException(*this, text, character_index)) {
            FloatPoint start_offset = HB_DIRECTION_IS_HORIZONTAL(run->direction_)
                                            ? FloatPoint(total_advance, 0)
                                            : FloatPoint(0, total_advance);
            Add(glyph_data.glyph,
                run->font_data_.Get(),
                start_offset + glyph_data.offset);

            if (UNLIKELY(flags_ & kIncludeUTF8Text))
              char_offsets.push_back(glyph_data.character_index);
          }
          return true;
        });

    if (flags_ & kIncludeUTF8Text)
      AddUtf8Data(*run, text, char_offsets, run_offset);
  }

  return total_advance;
}

bool ShapeResultBloberizer::CanUseFastPath(unsigned from,
                                           unsigned to,
                                           unsigned length,
                                           bool has_vertical_offsets) {
  return !from && to == length && !has_vertical_offsets &&
         GetType() != ShapeResultBloberizer::Type::kTextIntercepts &&
         !(flags_ & kIncludeUTF8Text);
}

float ShapeResultBloberizer::FillFastHorizontalGlyphs(
    const ShapeResultBuffer& result_buffer,
    TextDirection text_direction) {
  DCHECK(!result_buffer.HasVerticalOffsets());
  DCHECK_NE(GetType(), ShapeResultBloberizer::Type::kTextIntercepts);

  float advance = 0;
  auto results = result_buffer.results_;

  for (unsigned i = 0; i < results.size(); ++i) {
    const auto& word_result =
        IsLtr(text_direction) ? results[i] : results[results.size() - 1 - i];
    advance = FillFastHorizontalGlyphs(word_result.Get(), advance);
  }

  return advance;
}

float ShapeResultBloberizer::FillFastHorizontalGlyphs(
    const ShapeResult* shape_result,
    float advance) {
  DCHECK(!shape_result->HasVerticalOffsets());
  DCHECK_NE(GetType(), ShapeResultBloberizer::Type::kTextIntercepts);

  for (const auto& run : shape_result->runs_) {
    DCHECK(run);
    DCHECK(HB_DIRECTION_IS_HORIZONTAL(run->direction_));

    advance =
        run->ForEachGlyph(advance,
                          [&](const HarfBuzzRunGlyphData& glyph_data,
                              float total_advance) -> bool {
                            DCHECK(!glyph_data.offset.Height());
                            Add(glyph_data.glyph, run->font_data_.Get(),
                                total_advance + glyph_data.offset.Width());
                            return true;
                          });
  }

  return advance;
}

template <typename TextContainerType>
float ShapeResultBloberizer::FillTextEmphasisGlyphsForRun(
    const ShapeResult::RunInfo* run,
    const TextContainerType& text,
    unsigned text_length,
    TextDirection direction,
    unsigned from,
    unsigned to,
    const GlyphData& emphasis_data,
    float initial_advance,
    unsigned run_offset) {
  if (!run)
    return 0;

  unsigned graphemes_in_cluster = 1;
  float cluster_advance = 0;

  FloatPoint glyph_center =
      emphasis_data.font_data->BoundsForGlyph(emphasis_data.glyph).Center();

  // A "cluster" in this context means a cluster as it is used by HarfBuzz:
  // The minimal group of characters and corresponding glyphs, that cannot be
  // broken down further from a text shaping point of view.  A cluster can
  // contain multiple glyphs and grapheme clusters, with mutually overlapping
  // boundaries. Below we count grapheme clusters per HarfBuzz clusters, then
  // linearly split the sum of corresponding glyph advances by the number of
  // grapheme clusters in order to find positions for emphasis mark drawing.
  uint16_t cluster_start = static_cast<uint16_t>(
      direction == TextDirection::kRtl
          ? run->start_index_ + run->num_characters_ + run_offset
          : run->GlyphToCharacterIndex(0) + run_offset);

  float advance_so_far = initial_advance;
  const unsigned num_glyphs = run->glyph_data_.size();
  for (unsigned i = 0; i < num_glyphs; ++i) {
    const HarfBuzzRunGlyphData& glyph_data = run->glyph_data_[i];
    uint16_t current_character_index =
        run->start_index_ + glyph_data.character_index + run_offset;
    bool is_run_end = (i + 1 == num_glyphs);
    bool is_cluster_end =
        is_run_end || (run->GlyphToCharacterIndex(i + 1) + run_offset !=
                       current_character_index);

    if ((direction == TextDirection::kRtl && current_character_index >= to) ||
        (direction != TextDirection::kRtl && current_character_index < from)) {
      advance_so_far += glyph_data.advance;
      direction == TextDirection::kRtl ? --cluster_start : ++cluster_start;
      continue;
    }

    cluster_advance += glyph_data.advance;

    if (text.Is8Bit()) {
      float glyph_advance_x = glyph_data.advance;
      if (Character::CanReceiveTextEmphasis(text[current_character_index])) {
        AddEmphasisMark(*this, emphasis_data, glyph_center,
                        advance_so_far + glyph_advance_x / 2);
      }
      advance_so_far += glyph_advance_x;
    } else if (is_cluster_end) {
      uint16_t cluster_end;
      if (direction == TextDirection::kRtl) {
        cluster_end = current_character_index;
      } else {
        cluster_end = static_cast<uint16_t>(
            is_run_end ? run->start_index_ + run->num_characters_ + run_offset
                       : run->GlyphToCharacterIndex(i + 1) + run_offset);
      }
      graphemes_in_cluster = CountGraphemesInCluster(
          text.Characters16(), text_length, cluster_start, cluster_end);
      if (!graphemes_in_cluster || !cluster_advance)
        continue;

      float glyph_advance_x = cluster_advance / graphemes_in_cluster;
      for (unsigned j = 0; j < graphemes_in_cluster; ++j) {
        // Do not put emphasis marks on space, separator, and control
        // characters.
        if (Character::CanReceiveTextEmphasis(text[current_character_index])) {
          AddEmphasisMark(*this, emphasis_data, glyph_center,
                          advance_so_far + glyph_advance_x / 2);
        }
        advance_so_far += glyph_advance_x;
      }
      cluster_start = cluster_end;
      cluster_advance = 0;
    }
  }
  return advance_so_far - initial_advance;
}

}  // namespace blink
