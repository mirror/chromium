# Traffic Annotation Scripts
This file describes the scripts in `tools/traffic_annotation/scripts`

# annotations_xml_downstream_updater.py
Whenever auditor updates `annotations.xml`, this script is called to run all
scripts that update a file based on it. Please add your script to the SCRIPTS
constant.

# annotations_xml_downstream_unittests.py
The unittest calls this script to test if all downstream files that depend on
`annotations.xml` are correctly updated. Please add your script to the SCRIPTS
constant.
