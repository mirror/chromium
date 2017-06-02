// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_WINDOW_COORDINATE_CONVERSIONS_H_
#define SERVICES_UI_WS_WINDOW_COORDINATE_CONVERSIONS_H_

namespace gfx {
class Point;
}

namespace ui {
namespace ws {

class ServerWindow;

// Converts |point| from the coordinates of |from| to the coordinates of |to|.
// |from| and |to| must be an ancestors or descendants of each other.
gfx::Point ConvertPointFromRoot(const ServerWindow* window,
                                const gfx::Point& point);

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_WINDOW_COORDINATE_CONVERSIONS_H_
