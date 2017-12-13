# Android Deps Repository Generator

Tool to generate a gradle-specified repository for Android and Java
dependencies.

Usage:

```
> cd $CHROMIUM_SRC/src
> which gradlew
$CHROMIUM_SRC/src/third_party/gradle_wrapper/gradlew
> gradlew -b tools/android/roll/android_deps/build.gradle setUpRepository
```

A Gradle wrapper is included in the repository for convenience, see
`//third_party/gradle_wrapper`.

For each of the dependencies specified in `build.gradle`, the above command
will take care of the following tasks:

- Download the library
- Generate a README.chromium file
- Download the LICENSE
- Generate a GN target in BUILD.gn
- Set up a CIPD package (TODO)

Expected output layout:

```
third_party/android_deps
├── .gitignore (hand written)
├── additional_license_paths.json
├── BUILD.gn (partly generated)
├── repository
│   ├── dependency_group_name_and_version
│   │   ├── dependency_name_and_version.jar
│   │   ├── LICENSE
│   │   └── README.chromium
│   └── other_dependency_group_name_and_version
│       ├── other_dependency_name_and_version.jar
│       ├── LICENSE
│       └── README.chromium
└── README.chromium (hand written)
```

Implementation notes:
The script is written as a Gradle plugin to leverage its dependency resolution
features. An alternative way to implement it is to mix gradle to purely fetch
dependencies and their pom.xml files, and use Python to process and generate
the files. This approach was not as successful, as some information about the
dependencies does not seem to be available purely from the POM file, which
resulted in expecting dependencies that gradle considered unnecessary.
