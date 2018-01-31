#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Add Comments Here.
This script updates network traffic annotations trix.

Create client_secret.json and put it in current folder as specified here:
https://developers.google.com/sheets/api/quickstart/python
"""

import argparse
import datetime
import httplib2
import io
import json
import os
import shlex
import sys

from apiclient import discovery
from oauth2client import client
from oauth2client import tools
from oauth2client.file import Storage

VERBOSE = False

# If modifying these scopes, delete your previously saved credentials
# at ~/.credentials/CREDENTIALS_FILE
SCOPES = 'https://www.googleapis.com/auth/spreadsheets'
CREDENTIALS_FILE = 'sheets.googleapis.com-annotation-updater.json'
CLIENT_SECRET_FILE = 'client_secret.json'
APPLICATION_NAME = 'Chrome Network Traffic Annotations Spreadsheet Updater'
TEST_SPREADSHEET_ID = '1GgEGqbr3cD4G6VPnYMmTSvOWc_WrddBHq7Ex1QEj3cs'
SPREADSHEET_ID = '1TmBr9jnf1-hrjntiVBzT9EtkINGrtoBYFMWad2MBeaY'
ANNOTATIONS_RANGE_NAME = 'Annotations'
CHANGES_RANGE_NAME = 'Changes Stats'

# The columns whose change is not reported as a change.
SILENT_CHANGE_COLUMNS = ["Source File"]


def GetCredentials():
    """Gets valid user credentials from storage.

    If nothing has been stored, or if the stored credentials are invalid,
    the OAuth2 flow is completed to obtain the new credentials.

    Returns:
        Credentials, the obtained credential.
    """
    home_dir = os.path.expanduser('~')
    credential_dir = os.path.join(home_dir, '.credentials')
    if not os.path.exists(credential_dir):
        os.makedirs(credential_dir)
    credential_path = os.path.join(credential_dir, CREDENTIALS_FILE)
    client_secret_file = os.path.join(os.path.dirname(
                                  os.path.abspath(__file__)),CLIENT_SECRET_FILE)

    store = Storage(credential_path)
    credentials = store.get()

    if not credentials or credentials.invalid:
        flow = client.flow_from_clientsecrets(client_secret_file, SCOPES)
        flow.user_agent = APPLICATION_NAME
        if flags:
            credentials = tools.run_flow(flow, store, flags)
        else: # Needed only for compatibility with Python 2.6
            credentials = tools.run(flow, store)
        print('Storing credentials to ' + credential_path)
    return credentials


def SplitConsideringQuotes(text, splitter):
  tokens = []
  in_qoute = False
  token = ""
  for character in text:
    if character == '"':
      in_qoute = not in_qoute
    if character == splitter and not in_qoute:
      tokens.append(token.strip(' "'))
      token = ""
    else:
      token += character

  if in_qoute:
    print(">>> ERROR: Unexpected quotes in splitting %s." % text)
    return None
  if token:
    tokens.append(token)
  return tokens


def LoadAnnotationsSheet():
  """Loads the 'Chrome Network Traffic Annotations Sheet'.
  Returns:
    list of list Table of annotations loaded from the trix.
  """
  credentials = GetCredentials()
  http = credentials.authorize(httplib2.Http())
  discoveryUrl = ('https://sheets.googleapis.com/$discovery/rest?'
                  'version=v4')
  service = discovery.build('sheets', 'v4', http=http,
                            discoveryServiceUrl=discoveryUrl)
  result = service.spreadsheets().values().get(
      spreadsheetId=SPREADSHEET_ID, range=ANNOTATIONS_RANGE_NAME).execute()
  return result.get('values', [])


def create_insert_request(sheet_id, row):
  return { "insertDimension": {
            "range": {
              "sheetId": sheet_id,
              "dimension": "ROWS",
              "startIndex": row, # 0 index.
              "endIndex": row + 1
            }
          }
        }

def create_delete_request(sheet_id, row):
  return { "deleteDimension": {
              "range": {
                "sheetId": sheet_id,
                "dimension": "ROWS",
                "startIndex": row,
                "endIndex": row + 1
              }
            }
          }

def create_append_request(sheet_id, row_count):
  return { "appendDimension": {
              "sheetId": sheet_id,
              "dimension": "ROWS",
              "length": row_count
            }
          }

def create_update_request(row, column, value):
  cell_name = "%s%i" % (chr(65+column), 1+row)
  return { "range": "%s!%s:%s" % (ANNOTATIONS_RANGE_NAME, cell_name, cell_name),
           "values": [[value]] }


def SaveAnnotationsSheet(previous_contents, new_contents, update_report):
  """Updates the trix with new annotations and an update report.
  Args:
    previous_contents: list of list Full table of annotations before update.
    new_contents: list of list Full table of annotations, after update.
    update_report: str A one line report of how many updates have been done.
  """
  credentials = GetCredentials()
  http = credentials.authorize(httplib2.Http())
  discoveryUrl = ('https://sheets.googleapis.com/$discovery/rest?'
                  'version=v4')
  service = discovery.build('sheets', 'v4', http=http,
                            discoveryServiceUrl=discoveryUrl)


  response = service.spreadsheets().get(
    spreadsheetId=SPREADSHEET_ID, ranges=ANNOTATIONS_RANGE_NAME,
    includeGridData=False).execute()
  sheet_id = response['sheets'][0]['properties']['sheetId']

  requests = []

  # Step 1: Delete/Insert Rows.
  old_set = set(row[0] for row in previous_contents[1:])
  new_set = set(row[0] for row in new_contents[1:])
  removed = old_set - new_set
  added = list(new_set - old_set)
  added.sort()

  row = 0
  while row < len(previous_contents):
    row_id = previous_contents[row][0]
    # If a row is removed, remove it from previous sheet.
    if row_id in removed:
      if VERBOSE:
        print("Removing %s" % row_id)
      requests.append(create_delete_request(sheet_id, row))
      previous_contents.pop(row)
      continue
    # If there are rows to add, and they should be before current row, insert
    # one.
    if len(added) and added[0] < row_id:
      if VERBOSE:
        print("Inserting %s" % added[0])
      requests.append(create_insert_request(sheet_id, row))
      previous_contents.insert(row, [])
      added.pop(0)
    row += 1

  if len(added):
    if VERBOSE:
      print("Appending %i" % len(added))
    requests.append(create_append_request(sheet_id, len(added)))
    while len(added):
      previous_contents.append([])
      added.pop()

  assert(len(new_contents) == len(previous_contents))
  service.spreadsheets().batchUpdate(
      spreadsheetId=SPREADSHEET_ID, body={'requests': requests}).execute()

  # Step 2: Compare cells and update.
  requests = []
  for row in range(len(new_contents)):
    for col in range(len(new_contents[0])):
      previous_value = "" if col >= len(previous_contents[row]) else \
          previous_contents[row][col]
      new_value = "" if col >= len(new_contents[row]) else \
          new_contents[row][col]
      if previous_value != new_value:
        if VERBOSE:
          print("Updating %i,%i to %s." % (row, col, new_value))
        requests.append(create_update_request(row, col, new_value))

  batch_update_values_request_body = {
    'value_input_option': 'RAW',
    'data': requests
  }

  service.spreadsheets().values().batchUpdate(
    spreadsheetId=SPREADSHEET_ID,
    body=batch_update_values_request_body).execute()

  # Add Report Line
  value_range = "%s!A1:B1000" % CHANGES_RANGE_NAME
  append_body = {
      "range": value_range,
      "majorDimension": "ROWS",
      "values": [[datetime.datetime.now().strftime("%m/%d/%Y"), update_report]]
  }

  service.spreadsheets().values().append(
    spreadsheetId=SPREADSHEET_ID,
    range=value_range, valueInputOption='RAW',
    body=append_body).execute()


def LoadTSVFile(file_path):
  """Loads annotations TSV file."""
  rows = []
  lines = SplitConsideringQuotes(io.open(
      file_path, mode="r", encoding="utf-8").read(), "\n")

  for line in lines:
    rows.append(SplitConsideringQuotes(line, "\t"))
  return rows


def CompareContents(sheet_content, file_content):
  """Compares sheet's contents with file's content.
  Args:
    sheet_content: list of list Loaded annotations from the trix.
    file_content: list of list Loaded annotations from the file.

  Returns:
    new_content: list of list Updated list of annotations, generated from
      comparing the two inputs.
    updated_report: str Summery of the updates. None if no update is requred.
  """
  new_content = []
  last_update = datetime.datetime.now().strftime("%m/%d/%Y")
  added_count = 0
  removed_count = 0
  updated_count = 0
  silent_updates = 0
  other_platforms = 0

  # Remove headers.
  headers = sheet_content.pop(0)
  file_content = file_content[1:]

  silent_change_columns = []
  for title in SILENT_CHANGE_COLUMNS:
    if title not in headers:
      print(">>> ERROR: Could not find source file column.")
      return None, None
    silent_change_columns.append(headers.index(title))

  for sheet_row in sheet_content:
    unique_id = sheet_row[0]

    # Find unique id in the file.
    file_row = None
    for i in range(len(file_content)):
      if file_content[i][0] == unique_id:
        file_row = file_content.pop(i)
        break

    if not file_row:
      print("Deleted: %s" % unique_id)
      removed_count += 1
    elif not file_row[-1]:
      # This row is not available for current platform.
      other_platforms += 1
      new_content.append(sheet_row)
    else:
      # Search for differences.
      major_change = len(file_row) != len(sheet_row)
      minor_change = False
      for column in range(2, min(len(file_row), len(sheet_row))):
        if file_row[column] != sheet_row[column]:
          if column in silent_change_columns:
            minor_change = True
          else:
            major_change = True
            print("Updated: %s, %s" % (unique_id, headers[column]))
            print("[%s]\nto\n[%s]\n" % (sheet_row[column], file_row[column]))

      if major_change:
        # If the change is major, use file row and update last_update date.
        updated_count += 1
        file_row[1] = last_update
      elif minor_change:
        # If the change is minor, use file row but keep last_update date.
        file_row[1] = sheet_row[1]
        silent_updates += 1
      else:
        # If no change, use sheet_row
        file_row = sheet_row
      new_content.append(file_row)

  if file_content:
    for file_row in file_content:
      if file_row[-1]:
        print("Added: %s" % file_row[0])
        added_count += 1
        file_row[1] = last_update
        new_content.append(file_row)
      else:
        print(">>> ERROR: Unexpected empty line: %s" % file_row[0])

  new_content.sort(key=lambda x: x[0])
  new_content.insert(0, headers)
  if other_platforms:
    print(">>> WARNING: There are annotations from other platforms. "
          "Run again there.")

  # Give back the removed headers to input.
  sheet_content.insert(0, headers)

  update_report = None
  if added_count or removed_count or updated_count:
    update_report = "New annotations: %s, Modified annotations: %s, " \
                    "Removed annotations: %s" % (
                        added_count, updated_count, removed_count)
  elif silent_updates:
    update_report = "Some file locations updated."

  return new_content, update_report


def main():
  parser = argparse.ArgumentParser(description=APPLICATION_NAME)
  parser.add_argument(
      'annotations_file', nargs=1,
      help='Specifies the TSV annotations file exported from auditor.')

  args = parser.parse_args()

  sheet_content = LoadAnnotationsSheet()
  if not sheet_content:
    print("Could not read sheet data.")
    return -1

  # DEVELOPMENT SAFE GAURD: REMOVE BEFORE LANDING
  if sheet_content[0][0] != "TEST COPY":
    print("Have you loaded the correct sheet? %s" % sheet_content[0][0])
    if raw_input('(Y/n): ').strip().lower() != 'y':
      return -1

  file_content = LoadTSVFile(args.annotations_file[0])
  if not file_content:
    print("Could not read annotations file.")
    return -1

  new_content, update_report = CompareContents(
      sheet_content, file_content)

  if update_report:
    print("Proceed with udpate?")
    if raw_input('(Y/n): ').strip().lower() != 'y':
      return 0
    SaveAnnotationsSheet(sheet_content, new_content, update_report)

  return 0


if __name__ == '__main__':
    sys.exit(main())