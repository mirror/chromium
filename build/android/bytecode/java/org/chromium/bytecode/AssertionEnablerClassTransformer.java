// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.bytecode;

import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * An application that enables Java ASSERT statements by modifying Java bytecode. It takes in a JAR
 * file, modifies bytecode of classes that use ASSERT, and outputs the bytecode to a new JAR file.
 */
class AssertionEnablerClassTransformer implements ClassVisitorProvider {
    private static class AssertionEnabler extends ClassVisitor {
        static final String ASSERTION_DISABLED_NAME = "$assertionsDisabled";
        static final String STATIC_INITIALIZER_NAME = "<clinit>";

        AssertionEnabler(ClassWriter writer) {
            super(Opcodes.ASM5, writer);
        }

        @Override
        public MethodVisitor visitMethod(final int access, final String name, String desc,
                String signature, String[] exceptions) {
            // Patch static initializer.
            if ((access & Opcodes.ACC_STATIC) != 0 && name.equals(STATIC_INITIALIZER_NAME)) {
                return new MethodVisitor(Opcodes.ASM5,
                        super.visitMethod(access, name, desc, signature, exceptions)) {
                    // The following bytecode is generated for each class with ASSERT statements:
                    // 0: ldc #8 // class CLASSNAME
                    // 2: invokevirtual #9 // Method java/lang/Class.desiredAssertionStatus:()Z
                    // 5: ifne 12
                    // 8: iconst_1
                    // 9: goto 13
                    // 12: iconst_0
                    // 13: putstatic #2 // Field $assertionsDisabled:Z
                    //
                    // This function replaces line #13 to the following:
                    // 13: pop
                    // Consequently, $assertionsDisabled is assigned the default value FALSE.
                    @Override
                    public void visitFieldInsn(int opcode, String owner, String name, String desc) {
                        if (opcode == Opcodes.PUTSTATIC && name.equals(ASSERTION_DISABLED_NAME)) {
                            mv.visitInsn(Opcodes.POP);
                        } else {
                            super.visitFieldInsn(opcode, owner, name, desc);
                        }
                    }
                };
            }
            return super.visitMethod(access, name, desc, signature, exceptions);
        }
    }

    public ClassVisitor get(ClassWriter writer) {
        return new AssertionEnabler(writer);
    }
}
