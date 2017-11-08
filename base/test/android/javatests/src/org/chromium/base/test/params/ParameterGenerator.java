// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.params;

import java.util.List;

/**
 * Generator to use generate arguments for parameterized test methods.
 * @see org.chromium.base.test.params.ParameterAnnotations.UseParameterGenerator
 */
public interface ParameterGenerator { List<ParameterSet> getParams(); }
