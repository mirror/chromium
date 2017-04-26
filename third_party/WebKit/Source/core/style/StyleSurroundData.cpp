/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "core/style/StyleSurroundData.h"

namespace blink {

StyleSurroundData::StyleSurroundData()
    : margin_left_(kFixed),
      margin_right_(kFixed),
      margin_top_(kFixed),
      margin_bottom_(kFixed),
      padding_left_(kFixed),
      padding_right_(kFixed),
      padding_top_(kFixed),
      padding_bottom_(kFixed) {}

StyleSurroundData::StyleSurroundData(const StyleSurroundData& o)
    : RefCounted<StyleSurroundData>(),
      left_(o.left_),
      right_(o.right_),
      top_(o.top_),
      bottom_(o.bottom_),
      margin_left_(o.margin_left_),
      margin_right_(o.margin_right_),
      margin_top_(o.margin_top_),
      margin_bottom_(o.margin_bottom_),
      padding_left_(o.padding_left_),
      padding_right_(o.padding_right_),
      padding_top_(o.padding_top_),
      padding_bottom_(o.padding_bottom_),
      border_(o.border_) {}

bool StyleSurroundData::operator==(const StyleSurroundData& o) const {
  return (left_ == o.left_ && right_ == o.right_ && top_ == o.top_ &&
          bottom_ == o.bottom_) &&
         (margin_left_ == o.margin_left_ && margin_right_ == o.margin_right_ &&
          margin_top_ == o.margin_top_ && margin_bottom_ == o.margin_bottom_) &&
         (padding_left_ == o.padding_left_ &&
          padding_right_ == o.padding_right_ &&
          padding_top_ == o.padding_top_ &&
          padding_bottom_ == o.padding_bottom_) &&
         border_ == o.border_;
}

}  // namespace blink
