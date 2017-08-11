package org.chromium.bytecode;

import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.Label;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

class AssertionEnablerClassAdapter extends ClassVisitor {
    AssertionEnablerClassAdapter(ClassVisitor visitor) {
        super(Opcodes.ASM5, visitor);
    }

    @Override
    public MethodVisitor visitMethod(final int access, final String name, String desc,
            String signature, String[] exceptions) {
        return new RewriteAssertMethodVisitorWriter(
                Opcodes.ASM5, super.visitMethod(access, name, desc, signature, exceptions));
    }

    static class RewriteAssertMethodVisitorWriter extends MethodVisitor {
        static final String ASSERTION_DISABLED_NAME = "$assertionsDisabled";
        static final String INSERT_INSTRUCTION_OWNER = "org/chromium/build/BuildHooks";
        static final String INSERT_INSTRUCTION_NAME = "assertFailureHandler";
        static final String INSERT_INSTRUCTION_DESC = "(Ljava/lang/AssertionError;)V";
        static final boolean INSERT_INSTRUCTION_ITF = false;

        boolean mStartLoadingAssert;
        Label mGotoLabel;

        public RewriteAssertMethodVisitorWriter(int api, MethodVisitor mv) {
            super(api, mv);
        }

        @Override
        public void visitFieldInsn(int opcode, String owner, String name, String desc) {
            if (opcode == Opcodes.PUTSTATIC && name.equals(ASSERTION_DISABLED_NAME)) {
                super.visitInsn(Opcodes.POP); // enable assert
            } else if (opcode == Opcodes.GETSTATIC && name.equals(ASSERTION_DISABLED_NAME)) {
                mStartLoadingAssert = true;
                super.visitFieldInsn(opcode, owner, name, desc);
            } else {
                super.visitFieldInsn(opcode, owner, name, desc);
            }
        }

        @Override
        public void visitJumpInsn(int opcode, Label label) {
            if (mStartLoadingAssert && opcode == Opcodes.IFNE && mGotoLabel == null) {
                mGotoLabel = label;
            }
            super.visitJumpInsn(opcode, label);
        }

        @Override
        public void visitInsn(int opcode) {
            if (!mStartLoadingAssert || opcode != Opcodes.ATHROW) {
                super.visitInsn(opcode);
            } else {
                super.visitMethodInsn(Opcodes.INVOKESTATIC, INSERT_INSTRUCTION_OWNER,
                        INSERT_INSTRUCTION_NAME, INSERT_INSTRUCTION_DESC, INSERT_INSTRUCTION_ITF);
                super.visitJumpInsn(Opcodes.GOTO, mGotoLabel);
                mStartLoadingAssert = false;
                mGotoLabel = null;
            }
        }
    }
}