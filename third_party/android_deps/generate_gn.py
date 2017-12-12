
import sys
import os
import re
import logging
import fnmatch
import json
import xml.etree.ElementTree as ET
import textwrap
from collections import namedtuple
import argparse

def readPOM(artifact_path):
  artifact = ArtifactData(artifact_path)
  # print(artifact)
  # print(artifact._properties)
  # print(artifact._licenses)
  # print(artifact._dependencies)
  print(GNGenerator().process_artifact(artifact))
  print(ReadmeGenerator().process_artifact(artifact))


class DepsGenerator:
  def __init__(self, base_repo_location):
    self._base_repo_location = base_repo_location
    self._artifacts = []

  def load_artifacts(self):
    for root, dirnames, filenames in os.walk(self._base_repo_location):
      for filename in fnmatch.filter(filenames, '*.pom'):
        pom_path = os.path.join(root, filename)
        self._artifacts.append(ArtifactData(pom_path))

  def generate_build_files(self):
    readme_generator = ReadmeGenerator()
    gn_generator = GNGenerator(self._base_repo_location)
    build_str = []
    locations = []

    for artifact in self._artifacts:
      build_str.append(gn_generator.process_artifact(artifact))
      locations.append(artifact.location)
      with open(os.path.join(artifact.location, 'README.chromium'), 'w') as f:
        f.write(readme_generator.process_artifact(artifact))

    with open(os.path.join(self._base_repo_location, 'BUILD.gn'), 'w') as f:
      f.write(gn_generator.get_gn_template())
      f.write('\n'.join(build_str))
    with open(os.path.join(self._base_repo_location, 'additional_license_paths.json'), 'w') as f:
      json.dump(locations, f)

  def validate_dependencies(self, warn_only=False):
    # TODO(dgn): Some dependencies don't specify a version which screws up here.
    [logging.debug(art.id) for art in self._artifacts]
    artifact_dict = dict((art.id, art) for art in self._artifacts)
    missing_dependencies = []
    for art in self._artifacts:
      for dep in art.dependencies:
        if artifact_dict.get(dep.id) is None:
          missing_dependencies.append(dep.id + ' from ' + art._properties.artifact_id)

    if missing_dependencies:
      missing_dependencies_str = 'Missing dependencies:\n  ' + '\n  '.join(missing_dependencies)
      if warn_only:
        logging.warning(missing_dependencies_str)
      else:
        raise Exception(missing_dependencies_str)


class ReadmeGenerator:
  def __init__(self):
    pass

  def process_artifact(self, artifact):
    args = {
      'artifact_name': artifact.name,
      'artifact_url': artifact.url,
      'artifact_short_name': artifact._properties.artifact_id,
      'artifact_version': artifact._properties.version,
      'artifact_description': artifact.description,
      'license_name': artifact.license.name,
      'license_url': artifact.license.url,
    }
    return textwrap.dedent('''\
        Name: {artifact_name}
        Short Name: {artifact_short_name}
        URL: {artifact_url}
        Version: {artifact_version}
        License: {license_name}
        License File: {license_url}
        Security Critical: no

        Description:
        {artifact_description}

        Local Modifications:
        No modifications.
        ''').format(**args)


class GNGenerator:
  def __init__(self, base_path):
    self._base_path = base_path

  def get_gn_template(self):
    return textwrap.dedent('''\
    # Copyright 2017 The Chromium Authors. All rights reserved.
    # Use of this source code is governed by a BSD-style license that can be
    # found in the LICENSE file.

    import("//build/config/android/rules.gni")

    ''')

  def process_artifact(self, artifact):
    args = {
      'target_name': self._get_gn_target_name(artifact._properties),
      'artifact_path': os.path.relpath(artifact._artifact_path, self._base_path),
      'deps_str': '\n    '.join(['":' + self._get_gn_target_name(dep) + '",'
                            for dep in artifact.dependencies])
    }
    return textwrap.dedent('''\
    java_prebuilt("{target_name}") {{
      jar_path = "{artifact_path}"
      deps = [
        {deps_str}
      ]
    }}
    ''').format(**args)

  def _get_gn_target_name(self, artifact_properties):
    return artifact_properties.artifact_id + '_java'


class ArtifactData:
  LicenseData = namedtuple('LicenseData', 'name url')

  def __init__(self, pom_path):
    logging.debug('Parsing pom file at %s', pom_path)
    self._location = os.path.dirname(pom_path)
    self._xml_element = ArtifactData._parse_pom(pom_path)
    self._properties = ArtifactProperties(self._xml_element)
    self._artifact_path = pom_path[:-3] + self.packaging
    self._dependencies = None
    self._license = None

  # def __cmp__(self, other):
  #   return cmp(self.id, other.id)

  @property
  def license(self):
    if self._license is None:
      licenses = []
      for license_elt in self._xml_element.findall("./licenses/license"):
        licenses.append(ArtifactData._get_license_properties(license_elt))
      if len(licenses) > 1:
        logging.warning(self._error_str('Multiple licenses registered, please '
                        'review manually: {}', licenses))
      try:
        self._license = licenses[0]
      except IndexError:
        logging.error(self._error_str('License not found, please fix manually'))
        self._license = ArtifactData.LicenseData('TODO manually', 'TODO manually')
    return self._license

  @property
  def id(self):
    return self._properties.id

  @property
  def name(self):
    return self._get('./name')

  @property
  def url(self):
    default_value = 'TODO manually'
    ret = self._get('./url', default_value)
    if ret is default_value:
      logging.warning(self._error_str('URL not found'))
    return ret

  @property
  def packaging(self):
    packaging = self._get('./packaging', 'jar')
    if packaging not in ('jar', 'aar'):
      logging.warning(self._error_str('Irregular packaging type: {}, overriding to jar', packaging))
      packaging = 'jar'
    return packaging

  @property
  def description(self):
    # Cleaning up the description that can be indented in a block in the XML,
    # have leading and trailing new lines, etc.
    return textwrap.dedent(self._get('./description')).strip('\n')

  @property
  def dependencies(self):
    if self._dependencies is None:
      self._dependencies = []
      for dependency_elt in self._xml_element.findall("./dependencies/dependency"):
        properties = ArtifactProperties(dependency_elt)
        if properties.optional == 'true': continue
        if properties.scope == 'test': continue
        self._dependencies.append(properties)
      self._dependencies.sort()
    return self._dependencies

  @property
  def location(self):
    return self._location

  def _error_str(self, message, *args):
    return ('Artifact {} - ' + message).format(self._location, *args)

  def _get(self, xpath, default_value=None):
    val = ArtifactData._resolve_xpath(self._xml_element, xpath, None)
    if not val:
      if default_value is None:
        raise AttributeError(self._error_str('Failed resolving xpath "{}" in pom', xpath))
      else:
        logging.warning(self._error_str('Returning default value for xpath "{}"', xpath))

    return val or default_value

  @staticmethod
  def _resolve_xpath(xml_element, xpath, default_value=None):
    resolved_element = xml_element.find(xpath)
    return default_value if resolved_element is None else resolved_element.text

  @staticmethod
  def _get_license_properties(xml_element):
    return ArtifactData.LicenseData(ArtifactData._resolve_xpath(xml_element, 'name'),
                                    ArtifactData._resolve_xpath(xml_element, 'url'))

  @staticmethod
  def _parse_pom(pom_path):
    with open(pom_path, 'r') as f:
        xmlstring = f.read()

    # etree requires to prepend all tags with their namespace while making
    # queries, including the ones with the default namespace. Removing the default
    # namespace definition (xmlns="http://some/namespace") allows not having to
    # do that and makes the rest of the code behave more intuitively.
    xmlstring = re.sub(r'\sxmlns="[^"]+"', '', xmlstring, count=1)
    return ET.fromstring(xmlstring)


class ArtifactProperties:
  def __init__(self, xml_element):
    self.group_id = ArtifactData._resolve_xpath(xml_element, 'groupId') or \
        ArtifactData._resolve_xpath(xml_element, './parent/groupId')
    self.artifact_id = ArtifactData._resolve_xpath(xml_element, 'artifactId')
    self.version = ArtifactData._resolve_xpath(xml_element, 'version') or \
        ArtifactData._resolve_xpath(xml_element, './parent/version')
    self.optional = ArtifactData._resolve_xpath(xml_element, 'optional', None)
    self.scope = ArtifactData._resolve_xpath(xml_element, 'scope', None)
    self.id = '{}:{}:{}'.format(self.group_id, self.artifact_id, self.version)

  def __lt__(self, other):
    return self.id < other.id


if __name__ == '__main__':
  logging.basicConfig(level=logging.DEBUG)
  parser = argparse.ArgumentParser(description='TODO')
  parser.add_argument('repo',
                      help='path to the repo we want to generate files for')
  parser.add_argument('--no-validate', action='store_true',
                      help='do not exit on validation error')

  args = parser.parse_args()
  print(args)

  generator = DepsGenerator(args.repo)
  generator.load_artifacts()
  generator.validate_dependencies(args.no_validate)
  generator.generate_build_files()
  # readPOM(sys.argv[1])
