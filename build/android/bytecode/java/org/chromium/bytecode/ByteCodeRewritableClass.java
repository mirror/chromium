package org.chromium.bytecode;

import org.objectweb.asm.ClassReader;

class ByteCodeRewritableClass {
    private static ByteCodeRewritableClassPool classPool = new ByteCodeRewritableClassPool();

    private ClassReader reader;

    ByteCodeRewritableClass(ClassReader reader) {
        this.reader = reader;
        classPool.addClass(reader.getClassName(), this);
    }

    boolean subClassOf(String parentClazz) {
        String superName = reader.getSuperName();
        if (superName == null || classPool.get(superName) == null) return false;
        if (superName == parentClazz) return true;
        return classPool.get(superName).subClassOf(parentClazz);
    }

    ClassReader getReader() {
        return reader;
    }

    String getSuperName() {
        return reader.getSuperName();
    }

    String getName() {
        return reader.getClassName();
    }
}
