// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import groovy.json.JsonOutput
import org.gradle.api.DefaultTask
import org.gradle.api.tasks.TaskAction

import java.util.regex.Pattern

/**
 * Task to download dependencies specified in {@link ChromiumPlugin} and configure the
 * Chromium build to integrate them. Used by declaring a new task in a {@code build.gradle}
 * file:
 * <pre>
 * task myTaskName(type: BuildConfigGenerator) {
 *   repositoryPath 'build_files_and_repository_location/'
 * }
 * </pre>
 */
class BuildConfigGenerator extends DefaultTask {
    private static final BUILD_TOKEN_START = "# === Generated Code Start ==="
    private static final BUILD_TOKEN_END = "# === Generated Code End ==="
    private static final BUILD_GEN_PATTERN = Pattern.compile(
            "${BUILD_TOKEN_START}(.*)${BUILD_TOKEN_END}",
            Pattern.DOTALL)
    private static final DOWNLOAD_DIRECTORY_NAME = "repository"

    /**
     * Directory where the artifacts will be downloaded and where files will be generated.
     * Note: this path is specified as relative to the chromium source root, and must be normalised
     * to an absolute path before being used, as Groovy would base relative path where the script
     * is being executed.
     */
    String repositoryPath

    @TaskAction
    void main() {
        def graph = new ChromiumDepGraph(project: project)
        def normalisedPath = normalisePath(repositoryPath)

        // 1. Parse the dependency data
        graph.collectDependencies()

        // 2. Import artifacts into the local repository
        def dependencyDirectories = []
        graph.dependencies.values().each { dependency ->
            logger.debug "Processing ${dependency.name}: \n${jsonDump(dependency)}"
            def depDir = "${DOWNLOAD_DIRECTORY_NAME}/${dependency.id}"
            def absoluteDepDir = "${normalisedPath}/${depDir}"

            dependencyDirectories.add(depDir)

            if (new File("${absoluteDepDir}/${dependency.fileName}").exists()) {
                getLogger().quiet("${dependency.id} exists, skipping.")
                return
            }

            project.copy {
                from dependency.artifact.file
                into absoluteDepDir
            }

            new File("${absoluteDepDir}/README.chromium").write(makeReadme(dependency))
            new File("${absoluteDepDir}/cipd.yaml").write(makeCipdYaml(dependency, repositoryPath))
            if (!dependency.licenseUrl?.trim()?.isEmpty()) {
                downloadFile(dependency.licenseUrl, new File("${absoluteDepDir}/LICENSE"))
            }
        }

        // 3. Generate the root level build files
        updateBuildTargetDeclaration(graph, normalisedPath)
        updateEnsureDeclaration(graph, normalisedPath, repositoryPath)
        dependencyDirectories.sort { path1, path2 -> return path1.compareTo(path2) }
        updateReadmeReferenceFile(dependencyDirectories, normalisedPath)
    }

    private static void updateBuildTargetDeclaration(ChromiumDepGraph depGraph, String location) {
        File buildFile = new File("${location}/BUILD.gn")
        def sb = new StringBuilder()

        // Comparator to sort the dependency in alphabetical order, with the visible ones coming
        // before all the internal ones.
        def dependencyComparator = { dependency1, dependency2 ->
            def visibilityResult = Boolean.compare(dependency1.visible, dependency2.visible)
            if (visibilityResult != 0) return -visibilityResult

            return dependency1.id.compareTo(dependency2.id)
        }

        depGraph.dependencies.values().sort(dependencyComparator).each { dependency ->
            def depsStr = ""
            if (!dependency.children.isEmpty()) {
                dependency.children.each { childDep ->
                    depsStr += "\":${depGraph.dependencies[childDep].id}_java\","
                }
            }

            def libPath = "${DOWNLOAD_DIRECTORY_NAME}/${dependency.id}"
            if (dependency.extension == 'jar') {
                def targetTemplate = dependency.supportsAndroid  ? 'android_java_prebuilt'
                                                                 : 'java_prebuilt'
                sb.append("""\
                ${targetTemplate}("${dependency.id}_java") {
                  jar_path = "${libPath}/${dependency.fileName}"
                  output_name = "${dependency.id}"
                """.stripIndent())
            } else if (dependency.extension == 'aar') {
                sb.append("""\
                android_aar_prebuilt("${dependency.id}_java") {
                  aar_path = "${libPath}/${dependency.fileName}"
                  info_path = "${libPath}/${dependency.id}.info"
                """.stripIndent())
            } else {
                throw new IllegalStateException("Dependency type should be JAR or AAR")
            }

            if (!dependency.visible) sb.append("  visibility = [ \":*\" ]\n")
            if (!depsStr.empty) sb.append("  deps = [${depsStr}]\n")
            addSpecialTreatment(sb, dependency.id)

            sb.append("}\n\n")
        }

        def matcher = BUILD_GEN_PATTERN.matcher(buildFile.getText())
        if (!matcher.find()) throw new IllegalStateException("BUILD.gn insertion point not found.")
        buildFile.write(matcher.replaceFirst(
                "${BUILD_TOKEN_START}\n${sb.toString()}\n${BUILD_TOKEN_END}"))
    }

    private static void updateEnsureDeclaration(ChromiumDepGraph depGraph, String location, String repoPath) {
        File ensureFile = new File("${location}/android_deps.ensure")
        def sb = new StringBuilder()

        sb.append("""\
        # Copyright 2017 The Chromium Authors. All rights reserved.
        # Use of this source code is governed by a BSD-style license that can be
        # found in the LICENSE file.

        # File generated by //tools/android/roll/android_deps/fetch_all.sh

        """.stripIndent())

        // Comparator to sort the dependency in alphabetical order, with the visible ones coming
        // before all the internal ones.
        def dependencyComparator = { dependency1, dependency2 ->
            // def visibilityResult = Boolean.compare(dependency1.visible, dependency2.visible)
            // if (visibilityResult != 0) return -visibilityResult

            return dependency1.id.compareTo(dependency2.id)
        }

        depGraph.dependencies.values().sort(dependencyComparator).each { dependency ->
            def depPath = "${DOWNLOAD_DIRECTORY_NAME}/${dependency.id}"
            sb.append("""\
            @Subdir ${depPath}
            chromium/${repoPath}/${depPath} version:${dependency.version}

            # Three unchanging lines
            # avoid the horror that is
            # endless merge conflicts

            """.stripIndent())
        }

        ensureFile.write(sb.toString())
    }

    private static void addSpecialTreatment(StringBuilder sb, String dependencyId) {
        switch(dependencyId) {
            case 'com_android_support_support_compat':
            case 'com_android_support_support_media_compat':
                // Target has AIDL, but we don't support it yet: http://crbug.com/644439
                sb.append('  ignore_aidl = true\n')
                break;
            case 'com_android_support_transition':
                // Not specified in the POM, compileOnly dependency not supposed to be used unless
                // the library is present: b/70887421
                sb.append('  deps += [":com_android_support_support_fragment_java"]')
                break;
        }
    }

    private static void updateReadmeReferenceFile(List<String> directories, String location) {
        File refFile = new File("${location}/additional_readme_paths.json")
        refFile.write(JsonOutput.prettyPrint(JsonOutput.toJson(directories)) + "\n")
    }

    private String normalisePath(String pathRelativeToChromiumRoot) {
        def pathToChromiumRoot = "../../../.." // Relative to build.gradle, the project root.
        return project.file("${pathToChromiumRoot}/${pathRelativeToChromiumRoot}").absolutePath
    }

    static String makeReadme(ChromiumDepGraph.DependencyDescription dependency) {
        def licenseString
        // Replace license names with ones that are whitelisted, see third_party/PRESUBMIT.py
        switch (dependency.licenseName) {
            case "The Apache Software License, Version 2.0":
                licenseString = "Apache Version 2.0"
                break
            default:
                licenseString = dependency.licenseName
        }

        return """\
        Name: ${dependency.displayName}
        Short Name: ${dependency.name}
        URL: ${dependency.url}
        Version: ${dependency.version}
        License: ${licenseString}
        License File: ${dependency.supportsAndroid ? "LICENSE" : "NOT_SHIPPED"}
        Security Critical: no

        Description:
        ${dependency.description}

        Local Modifications:
        No modifications.
        """.stripIndent()
    }

    static String makeCipdYaml(ChromiumDepGraph.DependencyDescription dependency, String repoPath) {
        def str = """\
        # Copyright 2017 The Chromium Authors. All rights reserved.
        # Use of this source code is governed by a BSD-style license that can be
        # found in the LICENSE file.

        # To create CIPD package run the following command.
        # cipd create --pkg-def cipd.yaml -tag version:${dependency.version}
        package: chromium/${repoPath}/${DOWNLOAD_DIRECTORY_NAME}/${dependency.id}
        description: ${dependency.displayName}
        data:
        - file: ${dependency.fileName}
        - file: LICENSE
        """.stripIndent()

        if (dependency.extension == 'aar') {
            str += "- file: ${dependency.id}.info\n"
        }

        return str
    }

    static String jsonDump(obj) {
        return JsonOutput.prettyPrint(JsonOutput.toJson(obj))
    }

    static void printDump(obj) {
        getLogger().warn(jsonDump(obj))
    }

    static void downloadFile(String sourceUrl, File destinationFile) {
        destinationFile.withOutputStream { out ->
            out << new URL(sourceUrl).openStream()
        }
    }

}