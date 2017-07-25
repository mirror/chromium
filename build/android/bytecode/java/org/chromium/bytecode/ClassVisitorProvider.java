// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.bytecode;

import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;

interface ClassVisitorProvider {
    /**
     * Return a ClassVisitor capable of transforming the bytecode of a Java class.
     *
     * @param writer used to generate the modified bytecode.
     * @return a ClassVisitor that will perform the bytecode transformation.
     */
    ClassVisitor get(ClassWriter writer);
}
