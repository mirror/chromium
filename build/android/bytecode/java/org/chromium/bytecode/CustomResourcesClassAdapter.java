package org.chromium.bytecode;

import static org.objectweb.asm.Opcodes.ALOAD;
import static org.objectweb.asm.Opcodes.ARETURN;
import static org.objectweb.asm.Opcodes.ASM5;

import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.Label;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

import java.util.HashMap;

class CustomResourcesClassAdapter extends ClassVisitor {
    private HashMap<String, ClassReader> classes;
    private ByteCodeRewritableClass clazz;
    private boolean shouldTransform;
    private boolean hasGetResources;

    CustomResourcesClassAdapter(ClassVisitor visitor, ByteCodeRewritableClass clazz) {
        super(ASM5, visitor);
        this.clazz = clazz;
    }

    @Override
    public void visit(int i, int i1, String s, String s1, String s2, String[] strings) {
        super.visit(i, i1, s, s1, s2, strings);
        shouldTransform = shouldTransform();
    }

    @Override
    public MethodVisitor visitMethod(final int access, final String name, String desc,
            String signature, String[] exceptions) {
        if (shouldTransform && name.equals("getResources")) {
            hasGetResources = true;
            MethodVisitor mv = super.visitMethod(access, name, desc, signature, exceptions);
            return new CustomResourceMethodAdaptor(ASM5, mv);
        }
        return super.visitMethod(access, name, desc, signature, exceptions);
    }

    @Override
    public void visitEnd() {
        if (shouldTransform && !hasGetResources) {
            int access = Opcodes.ACC_PUBLIC;
            String name = "getResources";
            String desc = "()Landroid/content/res/Resources;";
            String signature = null;
            String[] exceptions = null;
            MethodVisitor mv = super.visitMethod(access, name, desc, signature, exceptions);
            Label l0 = new Label();
            mv.visitLabel(l0);
            mv.visitVarInsn(ALOAD, 0);
            mv.visitMethodInsn(Opcodes.INVOKESPECIAL, clazz.getSuperName(), "getResources",
                    "()Landroid/content/res/Resources;", false);
            mv.visitMethodInsn(Opcodes.INVOKESTATIC, "org/chromium/build/BuildHooks",
                    "getResources",
                    "(Landroid/content/res/Resources;)Landroid/content/res/Resources;", false);
            mv.visitInsn(ARETURN);
            Label l1 = new Label();
            mv.visitLabel(l1);
            mv.visitLocalVariable("this", "L" + clazz.getName() + ";", null, l0, l1, 0);
            mv.visitMaxs(1, 1);
            mv.visitEnd();
        }
        super.visitEnd();
    }

    // TODO: find return statement and inject -> BuildHooks.getResources(resources) before it.
    static class CustomResourceMethodAdaptor extends MethodVisitor {
        public CustomResourceMethodAdaptor(int api, MethodVisitor mv) {
            super(api, mv);
        }

        @Override
        public void visitCode() {
            //            super.visitCode();
            //            super.visitMethodInsn(Opcodes.INVOKESTATIC,
            //            "org/chromium/build/BuildHooks",
            //                    "getResources", "()V", false);
            //            super.visitEnd();
        }
    }

    private boolean shouldTransform() {
        String parentClazzName = "android/content/Context";
        return clazz.subClassOf(parentClazzName);
    }
}
