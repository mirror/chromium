import groovy.json.JsonOutput
import org.gradle.api.DefaultTask
import org.gradle.api.tasks.TaskAction

class BuildConfigGenerator extends DefaultTask {
    /** Main configuration, that contains all the dependencies to include in the generated files. */
    String configurationName

    /** Configuration whose targets will be marked "supports_android" */
    String androidCompatibleConfigurationName

    /** Directory where the artifacts will be downloaded and where files will be generated. */
    String repositoryPath

    @TaskAction
    void setUpRepository() {
        def config = project.configurations.getByName(configurationName)
        def graph = new ChromiumDepGraph(project: project)

        // 1. Parse the dependency data
        graph.collectDependencies(config.resolvedConfiguration)
        graph.markAndroidCompatible(project.configurations.getByName(androidCompatibleConfigurationName).resolvedConfiguration)
        assert config.resolvedConfiguration.resolvedArtifacts.size() == graph.dependencies.size()

        // 2. Import artifacts into the local repository
        def dependencyDirectories = []
        config.resolvedConfiguration.resolvedArtifacts.each { artifact ->
            def dependency = graph.dependencies.get(ChromiumDepGraph.makeModuleId(artifact))
            logger.debug "Processing ${dependency.name}: \n${jsonDump(dependency)}"
            def dependencyDir = "${repositoryPath}/${dependency.localDirectory}"
            project.copy {
                from artifact.file
                into dependencyDir
            }

            new File("${dependencyDir}/README.chromium").write(makeReadme(dependency))
            dependencyDirectories.add(dependencyDir)
        }

        // 3. Generate the root level build files
        new File('BUILD.gn').write(makeBuildTargetDeclaration(graph))
        new File('additional_license_paths.json').write(JsonOutput.prettyPrint(JsonOutput.toJson(dependencyDirectories)))
    }


    private String makeBuildTargetDeclaration(ChromiumDepGraph depGraph) {
        StringBuilder sb = new StringBuilder()
        sb.append("""\
        # Copyright 2017 The Chromium Authors. All rights reserved.
        # Use of this source code is governed by a BSD-style license that can be
        # found in the LICENSE file.
        
        import("//build/config/android/rules.gni")
        
        """.stripIndent())

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

        return sb.toString()
    }


    static String makeReadme(ChromiumDepGraph.DependencyDescription dependency) {
        return """\
        Name: ${dependency.displayName}
        Short Name: ${dependency.name}
        URL: ${dependency.url}
        Version: ${dependency.version}
        License: ${dependency.licenseName}
        License File: ${dependency.licenseUrl}
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

}