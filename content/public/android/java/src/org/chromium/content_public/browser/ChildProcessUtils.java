// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import org.chromium.base.CollectionUtil;
import org.chromium.content.browser.ChildProcessLauncherHelper;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 *
 */
public class ChildProcessUtils {
    private ChildProcessUtils() {}

    /**
     *
     * @return
     */
    public static Map<String, List<Integer>> getProcessTypePids() {
        Map<String, List<Integer>> map = new HashMap<>();

        Map<Integer, String> processTypes = ChildProcessLauncherHelper.getAllProcessTypes();
        CollectionUtil.forEach(processTypes, entry -> {
            String processType = entry.getValue();
            List<Integer> pids = map.get(processType);
            if (pids == null) {
                pids = new ArrayList<Integer>();
                map.put(processType, pids);
            }
            pids.add(entry.getKey());
        });

        return map;
    }
}