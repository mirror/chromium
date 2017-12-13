import groovy.json.JsonOutput
import org.gradle.api.DefaultTask
import org.gradle.api.tasks.TaskAction

import java.util.regex.Pattern

class BuildConfigGenerator extends DefaultTask {
    private static final BUILD_TOKEN_START = "# === Generated Code Start ==="
    private static final BUILD_TOKEN_END = "# === Generated Code End ==="
    static final BUILD_GEN_PATTERN = Pattern.compile(
            "${BUILD_TOKEN_START}(.*)${BUILD_TOKEN_END}",
            Pattern.DOTALL)

    /** Directory where the artifacts will be downloaded and where files will be generated. */
    String repositoryPath

    @TaskAction
    void setUpRepository() {
//        def config = project.configurations.getByName(configurationName)
        def graph = new ChromiumDepGraph(project: project)

        // 1. Parse the dependency data
        graph.collectDependencies()

        // 2. Import artifacts into the local repository
        def dependencyDirectories = []
        graph.dependencies.values().each { dependency ->
            logger.debug "Processing ${dependency.name}: \n${jsonDump(dependency)}"
            def dependencyDir = "${repositoryPath}/${dependency.localDirectory}"
            project.copy {
                from dependency.artifact.file
                into dependencyDir
            }

            new File("${dependencyDir}/README.chromium").write(makeReadme(dependency))
            if (!dependency.licenseUrl?.trim()?.isEmpty()) {
                downloadFile(dependency.licenseUrl, "${dependencyDir}/LICENSE")
            }
            dependencyDirectories.add(dependencyDir)
        }

        // 3. Generate the root level build files
        updateBuildTargetDeclaration(graph, new File('BUILD.gn'))
        new File('additional_license_paths.json').write(JsonOutput.prettyPrint(JsonOutput.toJson(dependencyDirectories)))
    }

    private String updateBuildTargetDeclaration(ChromiumDepGraph depGraph, File buildFile) {
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
              jar_path = "${repositoryPath}/${dependency.localDirectory}/${dependency.fileName}"
              supports_android = ${dependency.supportsAndroid}
              deps = [${depsStr}]
            }
            """.stripIndent())
        }

        def matcher = BUILD_GEN_PATTERN.matcher(buildFile.getText())
        if (!matcher.find()) throw new AssertionError("BUILD.gn insertion point not found.")
        buildFile.write(matcher.replaceAll(
                "${BUILD_TOKEN_START}\n${sb.toString()}\n${BUILD_TOKEN_END}"))
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

    static void downloadFile(String sourceUrl, String destinationFile) {
        new File(destinationFile).withOutputStream { out ->
            out << new URL(sourceUrl).openStream()
        }
    }

}