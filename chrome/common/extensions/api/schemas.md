# Extension API Specifications

[TOC]

## Summary
Extension APIs are specified in JSON or IDL files.  These files
describe the API and the functions, events, and properties it contains.  This
specification is used for multiple purposes, including setting up the API’s
bindings (the JavaScript end points that are exposed to extensions), generating
strong types and conversion for extension API objects, creating the public
documentation, generating JS externs, and registering extension API functions.

## Applications

### Bindings
The Extension API specifications generate the JSON objects that describe the
extension APIs.  These JSON objects are then used in the creation of the
extension API bindings to set up the functions, events, and properties that are
exposed to the extensions in JavaScript.  For more information, see the
[bindings documentation](/extensions/renderer/bindings.md).

### Type Generation
Extension API implementations use strong types in the browser
process, generated from the arguments passed by the renderer.  Both the type
definitions themselves and the conversion code to generate between the
serialized base::Values and strong types are generated based on the API
specification.  Additionally, we also generate constants for enums and event
names used in extension APIs.

### Documentation
The public documentation hosted at https://developer.chrome.com/extensions is
largely generated from the API specifications.  Everything from the function
descriptions to parameter ordering to exposed types and enums is generated from
the specification.  Because this becomes our public documentation, special care
should be given to ensure that descriptions for events, functions, properties,
and arguments are clear and well-formed.

### JS Externs
JS externs, which are used with
[Closure Compilation](https://developers.google.com/closure/compiler/), are
generated from the API specifications as well.  These are used both internally
within Chrome (e.g., in our settings WebUI code) as well as externally by
developers writing extensions.

## Structure
The API specification consists of four different sections: functions,
events, types, and properties.  Functions describe the API functions exposed to
extensions.  Events describe the different events that extensions can register
listeners for.  Types are used to define object types to use in either
functions or events.  Finally, properties are used to expose certain properties
on the API object itself.

## Compilation
The extension API compilation is triggered as part of the normal
compilation process (e.g., `ninja -C out/debug chrome`).  The compilation
process itself takes is defined in src/tools/json_schema_compiler/.  As part of
this step, both IDL and JSON extension API specifications are converted to the
same object, which can be output as a JSON object.  This API object is then
used in each of the compilation steps (bindings, type generation, documentation,
and JS externs).

## Adding a New File
To add a new API, create a new IDL or JSON file that defines
the API and add it to the appropriate BUILD.gn groups.  Files can be included
in different targets depending on their platform availability and whether they
need to be included in various compilation steps.

## IDL vs JSON
Extension APIs can be specified in either IDL or JSON.  During
compilation, all files are converted to JSON objects.  The benefit to using a
JSON file is that it is more clear what the JSON object output will look like
(since it’s simply the result of parsing the file).  IDL, on the other hand, is
typically much more readable - especially when an API will have many methods or
long descriptions.  However, IDL is not as fully-featured as JSON in terms of
accepted properties on different nodes.
