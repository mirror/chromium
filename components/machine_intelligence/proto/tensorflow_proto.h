// This head file is just a temporal warpper for tensorflow::Example and
// tensorflow::Feature proto. So that later on when the tensorflow framework is
// ready in chromium, we can just change this file to include the right head.

#ifndef COMPONENTS_MACHINE_INTELLIGENCE_PROTO_TENSORFLOW_EXAMPLE_PROTO_H_
#define COMPONENTS_MACHINE_INTELLIGENCE_PROTO_TENSORFLOW_EXAMPLE_PROTO_H_

// This is currently where we put tensorflow::Example and tensorflow::Feature.
#include "components/machine_intelligence/proto/tensorflow.proto.h"

// This is where it's expected to be; change to these files when it's availible.
//#include "third_party/tensorflow/core/example/example.proto.h"
//#include "third_party/tensorflow/core/example/feature.proto.h"

#endif  // COMPONENTS_MACHINE_INTELLIGENCE_PROTO_TENSORFLOW_EXAMPLE_PROTO_H_
