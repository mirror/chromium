# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# directory component prediction
# train on collected directory-component mapping
# predict component for any given directories
import numpy as np
import pandas as pd
import cPickle as pickle
import os
from collections import defaultdict

from sklearn.cross_validation import train_test_split
from sklearn.feature_extraction.text import CountVectorizer
from sklearn.feature_extraction.text import TfidfTransformer
from sklearn.multiclass import OneVsRestClassifier
from sklearn.preprocessing import MultiLabelBinarizer
from sklearn.svm import LinearSVC
from sklearn.pipeline import Pipeline
from sklearn import tree
from sklearn.ensemble import RandomForestClassifier


def TrainModel(mappings, components_info, method='decision_tree',
               data_path='./', saveModel=True):
  """train model

  Args:
  mappings: dict, key: dirtory path, value: components
  component_infos: component data
     components_info.compo_path_list: a list of component name
     e.g. [Blink, Blink>Animation, Blink>CSS>Filters]
     components_info.compo_par_list: a list of labels
     e.g. [[Blink], [Blink, Blink>Animation],
           [Blink, Blink>CSS, Blink>CSS>Filters]]
  method: 'decision tree' -- decision tree
          'svm' -- svm model
          'random_forest' -- random forest
  data_path: directory path to save model
  saveModel: True -- save model
             False -- not save model

  Returns:
  clf: classifier or model
  tfidf_transformer: tfidf transformer
  bigram_vectorizer: gram vectorizer
  compo_classes: component class names in order
  """
  dirs = list(mappings.keys())
  dir_components = list(mappings.values())
  component_data = components_info.compo_path_list
  component_label = components_info.compo_par_list

  bigram_vectorizer = CountVectorizer(ngram_range=(1, 2),
                                      token_pattern=r'\b\w+\b',
                                      min_df=1,
                                      max_df=0.5,
                                      stop_words='english')
  tfidf_transformer = TfidfTransformer()
  counts = bigram_vectorizer.fit_transform(dirs + component_data)
  tfidf = tfidf_transformer.fit_transform(counts)
  features = tfidf

  mlb = MultiLabelBinarizer()
  bins = mlb.fit_transform(dir_components + component_label)
  compo_classes = mlb.classes_

  if method == 'decision_tree':
    clf = tree.DecisionTreeClassifier()
  elif method == 'svm':
    clf = OneVsRestClassifier(LinearSVC(C=1.0))
  elif method == 'random_forest':
    clf = RandomForestClassifier(n_estimators=10)
  clf = clf.fit(features, bins)

  if saveModel:
    pickle.dump(tfidf_transformer,
                open(os.path.join(data_path, "transformer.pkl"), "wb"))
    pickle.dump(bigram_vectorizer,
                open(os.path.join(data_path, "vectorizer.pkl"), "wb"))
    pickle.dump(compo_classes,
                open(os.path.join(data_path, "component_def.pkl"), "wb"))
    pickle.dump(clf,
                open(os.path.join(data_path, "classifier.pkl"), "wb"))

  return tfidf_transformer, bigram_vectorizer, compo_classes, clf


def PredictComponentTFIDF(input_dir, tfidf_transformer,
                          bigram_vectorizer, compos, clf):
  """predict components for directory path using tfidf feature

  Args:
  input_dir: a list of directory paths
  tfidf_transformer: tfidf transformer to convert input
    directories into tfidf feature vector
  bigram_vectorizer: vectorizer for tfidf feature
  compos: all component labels
  clf: model

  Returns:
  a list of component predictions
  """
  counts_input = bigram_vectorizer.transform(input_dir)
  tfidf_input = tfidf_transformer.transform(counts_input)
  predicted_ind = clf.predict(tfidf_input)
  predicted_compo = []
  for i in range(0, len(predicted_ind)):
    compo_ind = [i for i, j in enumerate(predicted_ind[i]) if j == 1]
    compo_name = []
    for i in compo_ind:
      compo_name.append(compos[i])
    predicted_compo.append(compo_name)

  return predicted_ind, predicted_compo


def ValidateOnTrainSet(mappings, bigram_vectorizer, tfidf_transformer, clf):
  """Evaluate model performance on training set

  Args:
  mappings: training set
  bigram_vectorizer: vectorizer for tfidf feature
  tfidf_transformer: tfidf transformer to convert input directories
    into tfidf feature vector
  clf: model

  Returns:
  accu_val: predcition accuracy
  recall_val: recall
  recall_dir: path level recall, i.e. if one of the correct label is found,
              the path is labeled as correct prediction
  err_dir: key: path, value: false positive label
  err_dir_lab: key: path, value: true negative label,
    i.e. groundtruth label yet not be predicted
  """
  dirs = list(mappings.keys())
  compos = list(mappings.values()) + compo_tree.compo_par_list
  counts = bigram_vectorizer.transform(dirs)
  tfidf = tfidf_transformer.transform(counts)
  predicted = clf.predict(tfidf)
  mlb = MultiLabelBinarizer()
  bins = mlb.fit_transform(compos)
  bins = bins[0:len(predicted)]
  compo_classes = mlb.classes_
  recall_val = 0.0
  recall_dir = 0.0
  error_dir = defaultdict(list)
  error_dir_lab = defaultdict(list)
  for i in range(0, len(predicted)):
    dir_hit = False
    for j in range(0, len(predicted[0])):
      if (bins[i][j] == 1 and bins[i][j] == predicted[i][j]):
        recall_val += 1.0
        dir_hit = True
      elif (bins[i][j] == 1 and predicted[i][j] == 0):
          error_dir_lab[dirs[i]].append(compo_classes[j])
      elif (predicted[i][j] == 1 and bins[i][j] == 0):
          error_dir[dirs[i]].append(compo_classes[j])
    if dir_hit:
      recall_dir += 1.0
  recall_val = recall_val/sum(sum(bins))
  recall_dir = recall_dir/len(predicted)
  accu_val = np.mean(predicted == bins)
  return accu_val, recall_val, recall_dir, error_dir, error_dir_lab


def TrainVal(mappings, compo_tree, method='decision_tree'):
  """ Train and validation
  """
  dirs = list(mappings.keys())
  compos = list(mappings.values())
  compo_all = compo_tree.compo_path_list
  compo_compo = compo_tree.compo_par_list

  bigram_vectorizer = CountVectorizer(ngram_range=(1, 2),
                                      token_pattern=r'\b\w+\b',
                                      min_df=1,
                                      max_df=0.5,
                                      stop_words='english')
  tfidf_transformer = TfidfTransformer()
  counts = bigram_vectorizer.fit_transform(dirs + compo_all)
  tfidf = tfidf_transformer.fit_transform(counts)
  features = tfidf

  mlb = MultiLabelBinarizer()
  bins = mlb.fit_transform(compos + compo_compo)
  compo_classes = mlb.classes_

  x_train, x_val, y_train, y_val = train_test_split(features, bins,
                                                      train_size=0.6,
                                                      random_state=42)
  if method == 'decision_tree':
    clf = tree.DecisionTreeClassifier()
  elif method == 'svm':
    clf = OneVsRestClassifier(LinearSVC(C=1.0))
  elif method == 'random_forest':
    clf = RandomForestClassifier(n_estimators=10)
  clf = clf.fit(x_train, y_train)

  predicted = clf.predict(x_val)
  bins = y_val
  recall_val = 0.0
  recall_dir = 0.0
  error_dir = defaultdict(list)
  error_dir_lab = defaultdict(list)
  for i in range(0, len(predicted)):
    dir_hit = False
    for j in range(0, len(predicted[0])):
      if (bins[i][j] == 1 and bins[i][j] == predicted[i][j]):
        recall_val += 1.0
        dir_hit = True
      elif (bins[i][j] == 1 and predicted[i][j] == 0):
          error_dir_lab[dirs[i]].append(compo_classes[j])
      elif (predicted[i][j] == 1 and bins[i][j] == 0):
          error_dir[dirs[i]].append(compo_classes[j])
    if dir_hit:
      recall_dir += 1.0
  recall_val = recall_val/sum(sum(bins))
  recall_dir = recall_dir/len(predicted)
  accu_val = np.mean(predicted == bins)
  return accu_val, recall_val, recall_dir, error_dir, error_dir_lab
