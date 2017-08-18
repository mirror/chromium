# Network Traffic Annotations List
This file describes the `chrome/traffic_annotation/annotations_list.xml`.

# Content Description
`annotations_list.xml` includes the summary of all network traffic annotations
in Chromium repository. The content includes complete annotations and the merged
partial and completing (and branched completing) annotations.
For each annotation, unique id, hash code of unique id, and hash code of content
is presented. If annotation is a reserved one, instead of content hash code, a
`reserved` attribute is included.
Once an annotation is removed from the repository, a `deprecated` attribute is
added to its item in this file, with value equal to the deprecation date and
time. These items can be manually or automatically pruned after sufficient time.

# How to Generate/Update.
Run `traffic_annotation_auditor` to check for annotations correctness and
automatic update.