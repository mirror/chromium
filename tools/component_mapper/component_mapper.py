# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Train model from collected directory-component mappings and predict
component label(s) for any input directory path.

This script contains major functions to train model and predict
components. e.g.
Input:
['src/chrome/browser/push_messaging/',
 'src/third_party/WebKit/Source/core/page/']
Ouput:
[[Blink, Blink>PushAPI],
 [Blink, Blink>Scroll]]

Please refer to README.md for usage, test, and general project
information.
"""

from collections import defaultdict
import cPickle as pickle
import numpy
import optparse
import os
import sys

from sklearn import tree
from sklearn.cross_validation import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.feature_extraction.text import CountVectorizer
from sklearn.feature_extraction.text import TfidfTransformer
from sklearn.multiclass import OneVsRestClassifier
from sklearn.preprocessing import MultiLabelBinarizer
from sklearn.svm import LinearSVC

from component_tree import ComponentTree
from gt_mapping import GroundTruthMapping


def train_model(mappings, component_data, component_label,
               method='decision_tree', model_path=None):
  """Train model from directory-component mappings, using tfidf feature
  to represent directories.

  Args:
    mappings (dict): a dict of "directory_path: components"
    component_data (list): a list of component name.
      e.g. [Blink, Blink>Animation]
    component_label (list): component label for component_data. (maintain
      1:1 correspondence between component_label and component_data.)
      e.g. [[Blink], [Blink, Blink>Animation]]
    method (string): supported method options include 'decision tree', 'svm',
            'random_forest'.
    model_path (string): directory path to save model. No saving if None.

  Returns:
    tfidf_transformer: a transformer which transfer directory path into
      tfidf feature.
    bigram_vectorizer: vectorizer for tfidf feature.
    component_definition: complete list of component label definitions.
    clf: classifier or model.
  """
  assert len(component_data) == len(component_label),\
    "Component label does not match data."

  dirs = mappings.keys()
  dir_components = mappings.values()

  bigram_vectorizer = CountVectorizer(ngram_range=(1, 2),
                                      token_pattern=r'\b\w+\b',
                                      min_df=1, max_df=0.5,
                                      stop_words='english')
  tfidf_transformer = TfidfTransformer()
  counts = bigram_vectorizer.fit_transform(dirs + component_data)
  tfidf = tfidf_transformer.fit_transform(counts)
  features = tfidf
  mlb = MultiLabelBinarizer()
  groundtruth_label = mlb.fit_transform(dir_components + component_label)
  component_definition = mlb.classes_

  if method == 'decision_tree':
    clf = tree.DecisionTreeClassifier()
  elif method == 'svm':
    clf = OneVsRestClassifier(LinearSVC(C=1.0))
  elif method == 'random_forest':
    clf = RandomForestClassifier(n_estimators=10)
  clf = clf.fit(features, groundtruth_label)

  #TODO(ymzhang): Save model in JSON/protobufs instead of pickle.
  if model_path:
    if not os.path.exists(model_path):
      os.makedirs(model_path)
    pickle.dump(tfidf_transformer,
                open(os.path.join(model_path, "transformer.pkl"), "wb"))
    pickle.dump(bigram_vectorizer,
                open(os.path.join(model_path, "vectorizer.pkl"), "wb"))
    pickle.dump(component_definition,
                open(os.path.join(model_path, "component_def.pkl"), "wb"))
    pickle.dump(clf,
                open(os.path.join(model_path, "classifier.pkl"), "wb"))

  return tfidf_transformer, bigram_vectorizer, component_definition, clf


def predict_component(input_dirs, tfidf_transformer,
                      bigram_vectorizer, component_definition, clf):
  """Generate a list of components that best match each of the given
  directories.

  Args:
    input_dirs (list): a list of directory paths.
    tfidf_transformer: tfidf transformer to convert input directories into tfidf
      feature vector.
    bigram_vectorizer: vectorizer for tfidf feature.
    component_definition (list): all component label definitions.
    clf: classifier to predict component label.

  Returns:
    component_prediction: a list of component predictions.
  """
  counts_input = bigram_vectorizer.transform(input_dirs)
  tfidf_input = tfidf_transformer.transform(counts_input)
  prediction_result = clf.predict(tfidf_input)
  component_prediction = []
  # get component definition from prediction result matrix
  for input_i in range(0, len(prediction_result)):
    tmp_prediction_index = [input_i for input_i, predict_label_j
                            in enumerate(prediction_result[input_i])
                            if predict_label_j == 1]
    tmp_prediction_component = []
    for tmp_index in tmp_prediction_index:
      tmp_prediction_component.append(component_definition[tmp_index])
    component_prediction.append(tmp_prediction_component)
  return component_prediction


def evaluate_model_on_trainset(mappings, component_label,
                               bigram_vectorizer, tfidf_transformer, clf):
  """Evaluate model performance on training set for sanity check.

  This will compute recall and accuracy of prediction for directories in
  training set, i.e. directories in mappings.

  Args:
    mappings (dict): a dict of "directory_path: components".
    bigram_vectorizer: vectorizer for tfidf feature.
    tfidf_transformer: tfidf transformer to convert input directories
      into tfidf feature vector.
    clf: classifier to predict component label.

  Returns:
    model_metric: a dict measures model performance, refer to definition in
      evaluate_prediction.
  """
  dirs = mappings.keys()
  dir_components = mappings.values()
  # including all components since dir_components doesn't contain all
  # monorail components
  all_components = dir_components + component_label
  counts = bigram_vectorizer.transform(dirs)
  tfidf = tfidf_transformer.transform(counts)
  prediction_result = clf.predict(tfidf)
  mlb = MultiLabelBinarizer()
  groundtruth_label = mlb.fit_transform(dir_components + component_label)
  component_definition = mlb.classes_
  groundtruth_label = mlb.fit_transform(all_components)
  # keep prediction for directories, remove prediction for component words
  groundtruth_label = groundtruth_label[0:len(prediction_result)]
  return evaluate_prediction(dirs, component_definition,
                             prediction_result, groundtruth_label)


def tune_model_on_trainset(mappings, component_data, component_label,
                           method='decision_tree'):
  """Tune for hyperparameter on training set.

  Split training set into train_train and train_val to evaluate model
  performance. Based on the performance (recall, accuracy) of with
  different method parameter, the best method is selected for
  predict_component.

  Args:
    mappings (dict): a dict of "directory_xpath: components"
    component_data (list): a list of component name.
    component_label (list): label for component_data.
    method (string): select one model 'svm', 'decision_tree', 'random_forest'.

  Returns:
    model_metric: a dict measures model performance, refer to definition in
      evaluate_prediction.
  """
  dirs = mappings.keys()
  dir_components = mappings.values()

  bigram_vectorizer = CountVectorizer(ngram_range=(1, 2),
                                      token_pattern=r'\b\w+\b',
                                      min_df=1, max_df=0.5,
                                      stop_words='english')
  tfidf_transformer = TfidfTransformer()
  counts = bigram_vectorizer.fit_transform(dirs + component_data)
  tfidf = tfidf_transformer.fit_transform(counts)
  features = tfidf

  mlb = MultiLabelBinarizer()
  groundtruth_label = mlb.fit_transform(dir_components + component_label)
  component_definition = mlb.classes_

  x_train, x_val, y_train, y_val = train_test_split(features,
                                                    groundtruth_label,
                                                    train_size=0.6,
                                                    random_state=42)
  if method == 'decision_tree':
    clf = tree.DecisionTreeClassifier()
  elif method == 'svm':
    clf = OneVsRestClassifier(LinearSVC(C=1.0))
  elif method == 'random_forest':
    clf = RandomForestClassifier(n_estimators=10)
  clf = clf.fit(x_train, y_train)
  prediction_result = clf.predict(x_val)
  return evaluate_prediction(dirs, component_definition, prediction_result,
                             y_val)


def evaluate_prediction(dirs_for_predict, component_definition,
                        prediction_result, groundtruth_label):
  """ Compute recall and precision (accuracy) of prediction result.

  Compare the prediction result with the actual component label(s).

  Args:
    dirs_for_predict (list): list of dirs to predict.
    component_definition (list): all component definitions.
    prediction_result (2-d matrix): predicted components.
    groundtruth_label (2-d matrix): actual component labels.

  Returns:
    model_metric: a dict measures model performance, the data in form {
      accuracy_instance: instance level predcition accuracy
      recall_instance: instance level recall
      recall_dir: directory_path level recall, i.e. if one of the
        correct label is found, the path is labeled as correct prediction
      err_dir: a dict of "dir_path: false positive label"
      err_dir_lab: a dict of "dir_path: true negative label" i.e.
        groundtruth label yet not be predicted}.
  """
  recall_instance = 0.0
  recall_dir = 0.0
  error_dir = defaultdict(list)
  error_dir_missing_component = defaultdict(list)
  for sample_index, sample_prediction in list(enumerate(prediction_result)):
    tmp_dir_predict_correct = False
    for label_index, label_predict in list(enumerate(sample_prediction)):
      if (groundtruth_label[sample_index][label_index] == 1
          and label_predict == 1):
        recall_instance += 1.0
        tmp_dir_predict_correct = True
      elif (groundtruth_label[sample_index][label_index] == 1
            and label_predict == 0):
          error_dir_missing_component[dirs_for_predict[
              sample_index]].append(component_definition[label_index])
      elif (groundtruth_label[sample_index][label_index] == 0
            and label_predict == 1):
          error_dir[dirs_for_predict[sample_index]].append(
              component_definition[label_index])
    if tmp_dir_predict_correct:
      recall_dir += 1.0

  recall_instance = recall_instance/sum(sum(groundtruth_label))
  recall_dir = recall_dir/len(prediction_result)
  accuracy_instance = numpy.mean(prediction_result == groundtruth_label)
  model_metric = {"accuracy_instance": accuracy_instance,
                  "recall_instance": recall_instance,
                  "recall_dir": recall_dir, "error_dir": error_dir,
                  "error_dir_missing_component":
                  error_dir_missing_component}
  return model_metric


def load_pkl_file(file_name):
  if os.path.isfile(file_name):
    f = open(file_name, 'rb')
    loaded_obj = pickle.load(f)
    f.close()
  else:
    print ("no model file: %s", file_name)
    raise
  return loaded_obj


def load_model_from_dir(model_dir):
  """Load tfidf_transformer, bigram_vectorizer, components, clf from model."""
  transformer_file = model_dir + '/transformer.pkl'
  tfidf_transformer = load_pkl_file(transformer_file)
  vectorizer_file = model_dir + '/vectorizer.pkl'
  bigram_vectorizer = load_pkl_file(vectorizer_file)
  components_file = model_dir + '/component_def.pkl'
  components = load_pkl_file(components_file)
  clf_file = model_dir + '/classifier.pkl'
  clf = load_pkl_file(clf_file)
  return tfidf_transformer, bigram_vectorizer, components, clf


def main(argv):
  print "Start Component Mapper ..."
  usage = """ Usage: python %prog dir_path [options]
  dir_path  directory path(s) to be predicted for component(s).

Examples:
  python %prog /src/third_party/WebKit
  python %prog /src/third_party/WebKit -s ./model
  python %prog /src/third_party/WebKit -m ./model0324
  """
  parser = optparse.OptionParser(usage=usage)
  parser.add_option('-s', '--save_model', help='Specify directory to save '
                    'trained model')
  parser.add_option('-m', '--model_dir', help='Specify directory to load'
                    'pre-trained model')
  options, args = parser.parse_args(argv[1:])
  if args:
    input_dirs = args[0]
  else:
    print "Please specify directory you want to predict"
    raise

  if options.model_dir:
    print "load model ..."
    tfidf_transformer, bigram_vectorizer, components, clf = load_model_from_dir(
        options.model_dir)
  else:
    print "train model ..."
    # get groundtruth mappings
    groundtruth_mapping = GroundTruthMapping()
    groundtruth_mapping.load_mapping_from_json_file()
    groundtruth_mapping.add_parent_component()
    groundtruth_mapping.normalize_mapping()

    component_tree = ComponentTree()
    component_tree.get_component_labels()

    if options.save_model:
      print ("save model in %s"%options.save_model)
      tfidf_transformer, bigram_vectorizer, components, clf = train_model(
          groundtruth_mapping.complete_mappings, component_tree.component_defs,
          component_tree.component_labels, 'decision_tree', options.save_model)
    else:
      tfidf_transformer, bigram_vectorizer, components, clf = train_model(
          groundtruth_mapping.complete_mappings, component_tree.component_defs,
          component_tree.component_labels, 'decision_tree')

  input_dirs = input_dirs.split(',')
  predict_compos = predict_component(input_dirs, tfidf_transformer,
                                     bigram_vectorizer, components, clf)
  print "Component Suggestions:"
  for input_path_index in range(0, len(input_dirs)):
    print ", ".join(predict_compos[input_path_index])

  return


if __name__ == '__main__':
  sys.exit(main(sys.argv))
