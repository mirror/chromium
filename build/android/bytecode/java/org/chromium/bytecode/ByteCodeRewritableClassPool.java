package org.chromium.bytecode;

import java.util.HashMap;

class ByteCodeRewritableClassPool {
    private HashMap<String, ByteCodeRewritableClass> classes = new HashMap<>();

    void addClass(String name, ByteCodeRewritableClass clazz) {
        classes.put(name, clazz);
    }

    ByteCodeRewritableClass get(String name) {
        return classes.get(name);
    }
}
