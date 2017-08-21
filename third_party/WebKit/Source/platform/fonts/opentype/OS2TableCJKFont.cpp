/* ***** BEGIN LICENSE BLOCK *****
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ***** END LICENSE BLOCK ***** */

#include "platform/fonts/opentype/OS2TableCJKFont.h"

#include "SkTypeface.h"
#include "platform/wtf/Vector.h"

namespace blink {

// Ported from https://hg.mozilla.org/mozilla-central/rev/c93381b53df3
// Returns true if an OS/2 table is found and the code page bits 17-21 are set,
// compare https://www.microsoft.com/typography/otspec/os2.htm#cpr
bool OS2TableCJKFont::IsCJKFontFromOS2Table(SkTypeface* typeface) {
  SkFourByteTag os2_table_tag = SkSetFourByteTag('O', 'S', '/', '2');

  size_t os2_table_size = typeface->getTableSize(os2_table_tag);

  if (!os2_table_size)
    return false;

  Vector<char> table_buffer;
  table_buffer.resize(os2_table_size);

  size_t copied_bytes = typeface->getTableData(
      os2_table_tag, 0, table_buffer.size(), table_buffer.data());

  DCHECK(copied_bytes);

  const size_t sx_height_offset = 77;
  const size_t ul_code_page_range_1_offset = 69;
  // Ignore short OS/2 tables which do not contain code page ranges information.
  if (copied_bytes < sx_height_offset)
    return false;

  uint32_t ul_code_page_range_1 =
      static_cast<uint32_t>(table_buffer.data()[ul_code_page_range_1_offset]);
  const uint32_t code_page_bits_required =
      (1 << 17) |  // codepage 932 - JIS/Japan
      (1 << 18) |  // codepage 936 - Chinese (simplified)
      (1 << 19) |  // codepage 949 - Korean Wansung
      (1 << 20) |  // codepage 950 - Chinese (traditional)
      (1 << 21);   // codepage 1361 - Korean Johab

  return ul_code_page_range_1 & code_page_bits_required;
}

}  // namespace blink
