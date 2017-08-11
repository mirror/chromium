package org.chromium.bytecode;

import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

class ByteCodeProcessor {
    private static final String CLASS_FILE_SUFFIX = ".class";
    private static final String TEMPORARY_FILE_SUFFIX = ".temp";
    private static final int BUFFER_SIZE = 16384;

    static ByteCodeRewritableClassPool buildClassPool(List<String> inputJarPaths) {
        ByteCodeRewritableClassPool pool = new ByteCodeRewritableClassPool();
        for (String inputJarPath : inputJarPaths) {
            try (ZipInputStream inputStream = new ZipInputStream(
                         new BufferedInputStream(new FileInputStream(inputJarPath)))) {
                ZipEntry entry;
                while ((entry = inputStream.getNextEntry()) != null) {
                    if (!entry.isDirectory() && entry.getName().endsWith(CLASS_FILE_SUFFIX)) {
                        byte[] byteCode = readAllBytes(inputStream);
                        ClassReader reader = new ClassReader(byteCode);
                        pool.addClass(entry.getName(), new ByteCodeRewritableClass(reader));
                    }
                }
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
        return pool;
    }

    private static void enableAssertionInJar(String inputJarPath, String outputJarPath,
            ByteCodeRewritableClassPool classPool, boolean shouldAssert) {
        String tempJarPath = outputJarPath + TEMPORARY_FILE_SUFFIX;
        try (ZipInputStream inputStream = new ZipInputStream(
                     new BufferedInputStream(new FileInputStream(inputJarPath)));
                ZipOutputStream tempStream = new ZipOutputStream(
                        new BufferedOutputStream(new FileOutputStream(tempJarPath)))) {
            ZipEntry entry;

            while ((entry = inputStream.getNextEntry()) != null) {
                if (entry.isDirectory() || !entry.getName().endsWith(CLASS_FILE_SUFFIX)) {
                    tempStream.putNextEntry(entry);
                    tempStream.write(readAllBytes(inputStream));
                    tempStream.closeEntry();
                    continue;
                }
                ByteCodeRewritableClass clazz = classPool.get(entry.getName());
                ClassReader reader = clazz.getReader();
                ClassWriter writer = new ClassWriter(reader, 0);

                // Add a TraceClassVisitor to the chain to print bytecode for
                ClassVisitor chain = writer;
                if (shouldAssert) {
                    chain = new AssertionEnablerClassAdapter(chain);
                }
                chain = new CustomResourcesClassAdapter(chain, clazz);
                reader.accept(chain, 0);
                byte[] patchedByteCode = writer.toByteArray();
                tempStream.putNextEntry(new ZipEntry(entry.getName()));
                tempStream.write(patchedByteCode);
                tempStream.closeEntry();
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

        try {
            Path src = Paths.get(tempJarPath);
            Path dest = Paths.get(outputJarPath);
            Files.move(src, dest, StandardCopyOption.REPLACE_EXISTING);
        } catch (IOException ioException) {
            throw new RuntimeException(ioException);
        }
    }

    private static byte[] readAllBytes(InputStream inputStream) throws IOException {
        ByteArrayOutputStream buffer = new ByteArrayOutputStream();
        int numRead = 0;
        byte[] data = new byte[BUFFER_SIZE];
        while ((numRead = inputStream.read(data, 0, data.length)) != -1) {
            buffer.write(data, 0, numRead);
        }
        return buffer.toByteArray();
    }

    private static List<String> readFileArg(String inputPath) {
        ArrayList<String> files = new ArrayList<>();
        try {
            try (BufferedReader br = new BufferedReader(new FileReader(inputPath))) {
                String line;
                while ((line = br.readLine()) != null) {
                    files.add(line);
                }
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        return files;
    }

    public static void main(String[] args) {
        String inputJarPath = args[0];
        String outputJarPath = args[1];
        String classpathPath = args[2];
        String androidJarPath = args[3];
        String enableAssert = args[4];

        ArrayList<String> allJarPaths = new ArrayList<>();
        allJarPaths.add(inputJarPath);
        allJarPaths.add(androidJarPath);
        allJarPaths.addAll(readFileArg(classpathPath));
        ByteCodeRewritableClassPool classPool = buildClassPool(allJarPaths);
        boolean shouldAssert = enableAssert.equals("--enable-assert");
        enableAssertionInJar(inputJarPath, outputJarPath, classPool, shouldAssert);
    }
}
