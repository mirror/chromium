// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package testing;

import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;
import org.objectweb.asm.commons.AdviceAdapter;

import java.io.ByteArrayOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Map;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import java.util.jar.JarOutputStream;
import java.util.jar.Manifest;
import java.util.regex.Pattern;

/**
 * Allows instrumentation of all of a Java JAR's methods using tracing.
 */
public class InstrumentJar {
    private static class InstrumentationMethodVisitor extends AdviceAdapter {
        private String mMethodName;

        public InstrumentationMethodVisitor(
                MethodVisitor mv, int access, String name, String desc) {
            super(Opcodes.ASM5, mv, access, name, desc);
            this.mMethodName = name;
        }

        @Override
        protected void onMethodEnter() {
            visitTraceBegin(this.mMethodName);
        }

        @Override
        public void visitMethodInsn(
                int opcode, String owner, String name, String desc, boolean itf) {
            super.visitMethodInsn(opcode, owner, name, desc, itf);
        }

        @Override
        protected void onMethodExit(int opcode) {
            visitTraceEnd(this.mMethodName);
        }

        private void visitTraceBegin(String msg) {
            this.mv.visitLdcInsn(msg);
            this.mv.visitMethodInsn(
                    INVOKESTATIC, "util/Tracing", "traceBegin", "(Ljava/lang/String;)V", false);
        }

        private void visitTraceEnd(String msg) {
            this.mv.visitLdcInsn(msg);
            this.mv.visitMethodInsn(
                    INVOKESTATIC, "util/Tracing", "traceEnd", "(Ljava/lang/String;)V", false);
        }
    }

    private static class InstrumentationClassAdapter extends ClassVisitor {
        public InstrumentationClassAdapter(ClassVisitor cv) {
            super(Opcodes.ASM5, cv);
        }

        @Override
        public void visitEnd() {
            super.visitEnd();
        }

        @Override
        public MethodVisitor visitMethod(
                int access, String name, String desc, String signature, String[] exceptions) {
            MethodVisitor mv = super.visitMethod(access, name, desc, signature, exceptions);
            if (mv == null) {
                return null;
            } else {
                return new InstrumentationMethodVisitor(mv, access, name, desc);
            }
        }
    }

    private static byte[] transformClassBytes(byte[] classBytes) {
        ClassWriter cw = new ClassWriter(ClassWriter.COMPUTE_MAXS | ClassWriter.COMPUTE_FRAMES);
        InstrumentationClassAdapter icv = new InstrumentationClassAdapter(cw);

        ClassReader cr = new ClassReader(classBytes);
        cr.accept(icv, ClassReader.EXPAND_FRAMES);
        return cw.toByteArray();
    }

    private static Class<?> classOrNull(String className) {
        try {
            return Class.forName(className);
        } catch (ClassNotFoundException e) {
            System.err.println("Class not found: " + className);
        } catch (NoClassDefFoundError e) {
            System.err.println("Class definition not found: " + className);
        } catch (Exception e) {
            System.err.println("Other exception while reading class: " + className);
        }
        return null;
    }

    private static void transformJar(String jarFilePath) throws IOException {
        JarFile jf = new JarFile(jarFilePath);
        Manifest manifest = jf.getManifest();

        Map<String, byte[]> newEntries = new HashMap<String, byte[]>();
        for (Enumeration<JarEntry> eje = jf.entries(); eje.hasMoreElements();) {
            JarEntry je = eje.nextElement();

            String classFilePath = je.getName();
            String classFileExt = ".class";
            if (!classFilePath.endsWith(classFileExt) || classFilePath.indexOf("$") != -1) {
                continue;
            }
            Pattern forwardSlash = Pattern.compile("/");
            String className =
                    classFilePath.substring(0, classFilePath.length() - classFileExt.length());
            className = forwardSlash.matcher(className).replaceAll(".");

            Class<?> klass = classOrNull(className);

            if (klass != null) {
                InputStream is = klass.getClassLoader().getResourceAsStream(classFilePath);

                ByteArrayOutputStream buffer = new ByteArrayOutputStream();

                int nRead;
                byte[] data = new byte[16384];

                while ((nRead = is.read(data, 0, data.length)) != -1) {
                    buffer.write(data, 0, nRead);
                }

                buffer.flush();

                byte[] classBytes = buffer.toByteArray();

                byte[] transformedClassBytes = transformClassBytes(classBytes);
                newEntries.put(classFilePath, transformedClassBytes);
            }
        }

        jf.close();

        String newJarFilePath =
                jarFilePath.substring(0, jarFilePath.length() - 4) + "Transformed.jar";
        JarOutputStream jos = new JarOutputStream(new FileOutputStream(newJarFilePath), manifest);

        for (String classFilePath : newEntries.keySet()) {
            byte[] transformedClassBytes = newEntries.get(classFilePath);
            JarEntry entry = new JarEntry(classFilePath);

            jos.putNextEntry(entry);
            jos.write(transformedClassBytes);
            jos.closeEntry();
        }

        jos.close();
    }

    public static void main(String[] args) throws IOException {
        // String[] jarPaths = Pattern.compile(":").split(System.getProperty("java.class.path"));

        String jarPath = args[0];
        System.out.println(jarPath);

        transformJar(jarPath);
    }
}
