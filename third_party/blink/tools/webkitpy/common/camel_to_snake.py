# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
import sys


def convert(s):
    s = re.sub(r'CSS', r'Css', s)
    s = re.sub(r'Context2D', r'Context_2d', s)
    s = re.sub(r'DList', r'Dlist', s)
    s = re.sub(r'ETC1', r'Etc1', s)
    s = re.sub(r'IFrame', r'Iframe', s)
    s = re.sub(r'OList', r'Olist', s)
    s = re.sub(r'OnLine', r'Online', s)
    s = re.sub(r'Path2D', r'Path_2d', s)
    s = re.sub(r'Point2D', r'Point_2d', s)
    s = re.sub(r'RTCDTMF', r'RtcDtmf', s)
    s = re.sub(r'S3TC', r'S3tc', s)
    s = re.sub(r'UList', r'Ulist', s)
    s = re.sub(r'XPath', r'Xpath', s)
    s = re.sub(r'sRGB', r'Srgb', s)

    s = re.sub(r'SVGFE', r'SvgFe', s)
    s = re.sub(r'SVGMPath', r'SvgMpath', s)
    s = re.sub(r'SVGTSpan', r'SvgTspan', s)
    s = re.sub(r'SVG', r'Svg', s)

    s = re.sub(r'XHTML', r'Xhtml', s)
    s = re.sub(r'HTML', r'Html', s)

    s = re.sub(r'WebGL2', r'Webgl2', s)
    s = re.sub(r'WebGL', r'Webgl', s)

    s = re.sub(r'([A-Z][A-Z0-9]*)([A-Z][a-z0-9])', r'\1_\2', s)
    s = re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', s)
    return s.lower()

def main():
    for arg in sys.argv[1:]:
        print "%s\t=>\t%s" % (arg, convert(arg))

if __name__ == "__main__":
    main()
