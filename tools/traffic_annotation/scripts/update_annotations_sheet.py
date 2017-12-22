#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Add Comments Here.

Create client_secret.json and put it in this folder using:
https://pantheon.corp.google.com/flows/enableapi?
apiid=sheets.googleapis.com&debugUI=DEVELOPERS

"""

import argparse
import datetime
import httplib2
import io
import os
import shlex
import sys

from apiclient import discovery
from oauth2client import client
from oauth2client import tools
from oauth2client.file import Storage


# If modifying these scopes, delete your previously saved credentials
# at ~/.credentials/CREDENTIALS_FILE
SCOPES = 'https://www.googleapis.com/auth/spreadsheets'
CREDENTIALS_FILE = 'sheets.googleapis.com-annotation-updater.json'
CLIENT_SECRET_FILE = 'client_secret.json'
APPLICATION_NAME = 'Chrome Network Traffic Annotations Spreadsheet Updater'
SPREADSHEET_ID = '1GgEGqbr3cD4G6VPnYMmTSvOWc_WrddBHq7Ex1QEj3cs'
RANGE_NAME = 'annotations'


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
    print("ERROR: UNEXPECTED IN QUOTE.")
    return None

  if token:
    tokens.append(token)
  return tokens


def LoadAnnotationsSheet():
  """Loads the 'Chrome Network Traffic Annotations Sheet'."""
  credentials = GetCredentials()
  http = credentials.authorize(httplib2.Http())
  discoveryUrl = ('https://sheets.googleapis.com/$discovery/rest?'
                  'version=v4')
  service = discovery.build('sheets', 'v4', http=http,
                            discoveryServiceUrl=discoveryUrl)

  result = service.spreadsheets().values().get(
      spreadsheetId=SPREADSHEET_ID, range=RANGE_NAME).execute()
  return result.get('values', [])


def LoadTSVFile(file_path):
  """Loads annotations TSV file."""
  rows = []
  lines = SplitConsideringQuotes(io.open(file_path, mode="r", encoding="utf-8").read(), "\n")

  for line in lines:
    rows.append(SplitConsideringQuotes(line, "\t"))
  return rows


def CompareContents(sheet_content, file_content):
  """Compares sheet's contents with file's content and generates update list,
  and generates the change list and updated sheet."""
  new_content = []
  last_update = datetime.datetime.now().strftime("%m/%d/%Y")
  added_count = 0
  remove_count = 0
  updated_count = 0
  other_platforms = 0

  # Remove headers.
  header = sheet_content.pop(0)
  file_content = file_content[1:]

  for sheet_row in sheet_content:
    unique_id = sheet_row[0]

    # Find unique id in the file.
    file_row = None
    for i in range(len(file_content)):
      if file_content[i][0] == unique_id:
        file_row = file_content.pop(i)
        break

    if not file_row:
      print(" >> DELETED: %s" % unique_id)
      remove_count += 1
    elif not file_row[-1]:
      # This row is not available for current platform.
      other_platforms += 1
      new_content.append(sheet_row)
    elif file_row[2:] != sheet_row[2:]:
      print(" >> UPDATED: %s" % unique_id)
      for i in range(2, min(len(file_row), len(sheet_row))):
        if file_row[i] != sheet_row[i]:
          print("[%s]\n[%s]\n\n" % (file_row[i], sheet_row[i]))
      updated_count += 1
      file_row[1] = last_update
      new_content.append(file_row)
    else:
      new_content.append(sheet_row)

  if file_content:
    for file_row in file_content:
      if file_row[-1]:
        print(" >> ADDED: %s" % unique_id)
        added_count += 1
        file_row[1] = last_update
        new_content.append(file_row)
      else:
        print(" >> UNEXPECTED EMPTY: %s" % file_row[0])

  new_content.sort(cmp=lambda x: x[0])
  new_content.insert(0, header)
  if other_platforms:
    print(" >> There are annotations from other platforms. Run again there.")

  return new_content, added_count, remove_count, updated_count



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
    return -1

  file_content = LoadTSVFile(args.annotations_file[0])
  if not file_content:
    print("Could not read annotations file.")
    return -1

  new_content, added, removed, udpated = CompareContents(
      sheet_content, file_content)

  if added or removed or udpated:
    print("Proceed with udpate?")
    if raw_input('(Y/n): ').strip().lower() != 'y':
      return 0
    print("UPDATING MAIN SHEET...")
    print("UPDATING CHANGE LIST SHEET...")

  return 0


if __name__ == '__main__':
    sys.exit(main())