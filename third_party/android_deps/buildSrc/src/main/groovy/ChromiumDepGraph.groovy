import groovy.json.JsonOutput
import groovy.util.slurpersupport.GPathResult
import org.gradle.api.Project
import org.gradle.api.artifacts.*
import org.gradle.api.artifacts.component.ComponentIdentifier
import org.gradle.api.artifacts.result.ResolvedArtifactResult
import org.gradle.maven.MavenModule
import org.gradle.maven.MavenPomArtifact

class ChromiumDepGraph {
    final def dependencies = new HashMap<String, DependencyDescription>()
    final def topLevelIds = new ArrayList<String>()
    Project project

    void collectDependencies() {
        def androidConfig = project.configurations.getByName('compile').resolvedConfiguration
        Set<ResolvedConfiguration> deps = []
        deps += androidConfig.firstLevelModuleDependencies
        deps += project.configurations.getByName('annotationProcessor').resolvedConfiguration
                .firstLevelModuleDependencies

        deps.each { dependency ->
            topLevelIds.add(makeModuleId(dependency.module))
            collectDependenciesInternal(dependency)
        }

        androidConfig.resolvedArtifacts.each { artifact ->
            def dep = dependencies.get(makeModuleId(artifact))
            assert dep != null : "No dependency collected for artifact ${artifact.name}"

            dep.supportsAndroid = true
        }
    }

    private ResolvedArtifactResult getPomFromArtifact(ComponentIdentifier componentId) {
        def component = project.dependencies.createArtifactResolutionQuery()
                .forComponents(componentId)
                .withArtifacts(MavenModule, MavenPomArtifact)
                .execute()
                .resolvedComponents[0]
        return component.getArtifacts(MavenPomArtifact)[0]
    }


    private void collectDependenciesInternal(ResolvedDependency dependency) {
        def id = makeModuleId(dependency.module)
        if (dependencies.containsKey(id)) {
            if (dependencies.get(id).version <= dependency.module.id.version) return
        }

        def childModules = []
        dependency.children.each { childDependency ->
            childModules += makeModuleId(childDependency.module)
        }

        if (dependency.getModuleArtifacts().size() != 1) {
            throw new IllegalStateException("The dependency ${id} does not have exactly one artifact: ${dependency.getModuleArtifacts()}")
        }
        def artifact = dependency.getModuleArtifacts()[0]
        if (artifact.extension != 'jar' && artifact.extension != 'aar') {
            throw new IllegalStateException("Type ${artifact.extension} of ${id} not supported.")
        }

        dependencies.put(id, buildDepDescription(dependency, artifact, childModules))
        dependency.children.each {
            childDependency -> collectDependenciesInternal(childDependency)
        }
    }


    static void jsonDump(obj) {
        println JsonOutput.prettyPrint(JsonOutput.toJson(obj))
    }

    static String makeModuleId(ResolvedModuleVersion module) {
        // Does not include version because by default the resolution strategy for gradle is to use the
        // newest version among the required ones. We want to be able to match it in the BUILD.gn file.
        return "${module.id.group}:${module.id.name}"
    }

    static String makeModuleId(ResolvedArtifact artifact) {
        // Does not include version because by default the resolution strategy for gradle is to use the
        // newest version among the required ones. We want to be able to match it in the BUILD.gn file.
        return "${artifact.id.componentIdentifier.group}:${artifact.id.componentIdentifier.module}"
    }


    private static String makeLocalDirName(ComponentIdentifier componentIdentifier) {
        return componentIdentifier.displayName.replaceAll("[:.]", "_")
    }

    private buildDepDescription(ResolvedDependency dependency, ResolvedArtifact artifact, List<String> childModules) {
        def pom = getPomFromArtifact(artifact.id.componentIdentifier).file
        def licenseName = ''
        def licenseUrl = ''
        def pomContent = new XmlSlurper(false, false).parse(pom)
        GPathResult licenses = pomContent?.licenses?.license
        if (!licenses || licenses.size() == 0) {
            project.logger.warn("No license found on " + makeModuleId(dependency.module))
        } else if (licenses.size() > 1) {
            project.logger.warn("More than one license found on " + makeModuleId(dependency.module))
        } else {
            licenseName = licenses[0].name.text()
            licenseUrl = licenses[0].url.text()
        }

        return new DependencyDescription(
                artifact: artifact,
                group: dependency.module.id.group,
                name: dependency.module.id.name,
                version: dependency.module.id.version,
                extension: artifact.extension,
                componentId: artifact.id.componentIdentifier,
                children: Collections.unmodifiableList(new ArrayList<>(childModules)),
                licenseName: licenseName,
                licenseUrl: licenseUrl,
                localDirectory: makeLocalDirName(artifact.id.componentIdentifier),
                fileName: artifact.file.name,
                description: pomContent.description?.text(),
                url: pomContent.url?.text(),
                displayName: pomContent.name?.text()
        )
    }

    static class DependencyDescription {
        ResolvedArtifact artifact
        String group, name, version, extension, displayName, description, url
        String licenseName, licenseUrl
        String localDirectory, fileName
        boolean supportsAndroid
        ComponentIdentifier componentId
        List<String> children
    }
}