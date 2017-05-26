import os
import numpy as np
import Image


def mark_image(path):
  print(path)
  im = Image.open(path).convert('RGBA')
  data = np.array(im)

  r, g, b = 255, 105, 180 # Value that we want to mark with

  width = len(data)
  height = len(data[0])

  for i in range(1, width//4 + 1):
    for j in range(1, height//4 + 1):
      data[i][j] = [r, g, b, 255]

  if width >= 4 and height >= 4:
    data[1][1] = [0, 0, 0, 255]

    data[width-3][height-3] = [r, g, b, 255]
    data[width-2][height-3] = [r, g, b, 255]
    data[width-3][height-2] = [r, g, b, 255]
    data[width-2][height-2] = [0, 0, 0, 255]

  im = Image.fromarray(data)
  im.save(path)

def get_folders():
  folders = []
  with open("folders.txt") as f:
    for line in f:
      folders.append(line.strip())
  return folders

def get_folder_file_pathes(folder_path):
  pathes = []
  for dirpath, dirnames, filenames in os.walk(folder_path):
    for f in filenames:
      pathes.append(os.path.join(dirpath, f))
  return pathes

def get_folder_size_bytes(path):
  return sum(map(os.path.getsize, get_folder_file_pathes(path)))

def get_folder_files_count(path):
  return len(get_folder_file_pathes(path))

total_size_bytes = 0;
total_files = 0
for f in get_folders():
  size_bytes = get_folder_size_bytes(f)
  total_size_bytes += size_bytes
  files = get_folder_files_count(f)
  total_files += files
  print(f, "\t", size_bytes, "\t", files)

print("overall", "\t", total_size_bytes, "\t", total_files)

for f in get_folders():
  map(os.remove, get_folder_file_pathes(f))
  #map(mark_image, get_folder_file_pathes(f))
