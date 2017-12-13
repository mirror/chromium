import groovy.json.JsonOutput
import org.gradle.api.DefaultTask
import org.gradle.api.tasks.TaskAction

import java.util.regex.Pattern

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
            def depDir = "${DOWNLOAD_DIRECTORY_NAME}/${dependency.localDirectory}"
            def absoluteDepDir = "${normalisedPath}/${depDir}"

            project.copy {
                from dependency.artifact.file
                into absoluteDepDir
            }

            new File("${absoluteDepDir}/README.chromium").write(makeReadme(dependency))
            if (!dependency.licenseUrl?.trim()?.isEmpty()) {
                downloadFile(dependency.licenseUrl, new File("${absoluteDepDir}/LICENSE"))
            }
            dependencyDirectories.add(depDir)
        }

        // 3. Generate the root level build files
        updateBuildTargetDeclaration(graph, normalisedPath)
        updateReadmeReferenceFile(dependencyDirectories, normalisedPath)
    }

    private static void updateBuildTargetDeclaration(ChromiumDepGraph depGraph, String location) {
        File buildFile = new File("${location}/BUILD.gn")
        def sb = new StringBuilder()

        depGraph.dependencies.values().each { dependency ->
            def depsStr = ""
            if (!dependency.children.isEmpty()) {
                dependency.children.each { childDep ->
                    depsStr += "\":${depGraph.dependencies[childDep].name}_java\","
                }
            }

            sb.append("""\
            java_prebuilt("${dependency.name}_java") {
              jar_path =
                "${DOWNLOAD_DIRECTORY_NAME}/${dependency.localDirectory}/${dependency.fileName}"
              supports_android = ${dependency.supportsAndroid}
              deps = [${depsStr}]
            }
            """.stripIndent())
        }

        def matcher = BUILD_GEN_PATTERN.matcher(buildFile.getText())
        if (!matcher.find()) throw new IllegalStateException("BUILD.gn insertion point not found.")
        buildFile.write(matcher.replaceAll(
                "${BUILD_TOKEN_START}\n${sb.toString()}\n${BUILD_TOKEN_END}"))
    }


    private static void updateReadmeReferenceFile(List<String> directories, String location) {
        File refFile = new File("${location}/additional_readme_paths.json")
        if (directories.isEmpty()) {
            refFile.delete()
        } else {
            refFile.write(JsonOutput.prettyPrint(JsonOutput.toJson(directories)) + "\n")
        }
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