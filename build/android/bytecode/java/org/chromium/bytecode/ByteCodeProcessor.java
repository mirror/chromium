// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.bytecode;

import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassWriter;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

class ByteCodeProcessor {
    private static final String CLASS_FILE_SUFFIX = ".class";
    private static final String TEMPORARY_FILE_SUFFIX = ".temp";

    private static final int BUFFER_SIZE = 16384;

    private static void transformJar(
            String inputJarPath, String outputJarPath, ClassVisitorProvider visitorProvider) {
        String tempJarPath = outputJarPath + TEMPORARY_FILE_SUFFIX;
        try (ZipInputStream inputStream = new ZipInputStream(
                     new BufferedInputStream(new FileInputStream(inputJarPath)));
                ZipOutputStream tempStream = new ZipOutputStream(
                        new BufferedOutputStream(new FileOutputStream(tempJarPath)))) {
            ZipEntry entry;

            while ((entry = inputStream.getNextEntry()) != null) {
                byte[] byteCode = readAllBytes(inputStream);

                if (entry.isDirectory() || !entry.getName().endsWith(CLASS_FILE_SUFFIX)) {
                    tempStream.putNextEntry(entry);
                    tempStream.write(byteCode);
                    tempStream.closeEntry();
                    continue;
                }
                ClassReader reader = new ClassReader(byteCode);
                ClassWriter writer = new ClassWriter(reader, 0);
                reader.accept(visitorProvider.get(writer), 0);
                byte[] patchedByteCode = writer.toByteArray();
                tempStream.putNextEntry(new ZipEntry(entry.getName()));
                tempStream.write(patchedByteCode);
                tempStream.closeEntry();
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

        copyFile(tempJarPath, outputJarPath);
    }

    private static void copyFile(String inputPath, String outputPath) {
        try {
            Path src = Paths.get(inputPath);
            Path dest = Paths.get(outputPath);
            Files.move(src, dest, StandardCopyOption.REPLACE_EXISTING);
        } catch (IOException ioException) {
            throw new RuntimeException(ioException);
        }
    }

    private static byte[] readAllBytes(InputStream inputStream) throws IOException {
        ByteArrayOutputStream buffer = new ByteArrayOutputStream();
        int numRead;
        byte[] data = new byte[BUFFER_SIZE];
        while ((numRead = inputStream.read(data, 0, data.length)) != -1) {
            buffer.write(data, 0, numRead);
        }

        return buffer.toByteArray();
    }

    public static void main(String[] args) {
        if (args.length != 2) {
            System.out.println("Incorrect number of arguments.");
            System.out.println("Example usage: bytecode_rewriter input.jar output.jar");
            System.exit(-1);
        }

        String inputJar = args[0];
        String outputJar = args[1];
        ArrayList<ClassVisitorProvider> visitorProviders = new ArrayList<>();
        visitorProviders.add(new AssertionEnablerProvider());
        String currentInputFile = inputJar;
        String currentOutputFile = null;
        try {
            Path tmpDir = Files.createTempDirectory(
                    Paths.get(System.getProperty("java.io.tmpdir")), "bytecode");
            tmpDir.toFile().deleteOnExit();
            for (ClassVisitorProvider visitorProvider : visitorProviders) {
                currentOutputFile =
                        Paths.get(tmpDir.toString(), visitorProvider.getClass().toString())
                                .toString();
                transformJar(currentInputFile, currentOutputFile, visitorProvider);
                currentInputFile = currentOutputFile;
            }
            copyFile(currentOutputFile, outputJar);
        } catch (IOException ioException) {
            throw new RuntimeException(ioException);
        }
    }
}
