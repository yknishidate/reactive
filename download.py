import os
import requests
import zipfile

url = "http://lightfield.stanford.edu/data/lego_lf/rectified.zip"
filename = "asset/rectified.zip"

if os.path.isfile(filename):
    print("already downloaded:", filename)
else:
    print("downloading:", filename)
    data = requests.get(url).content
    with open(filename, mode='wb') as f:
        f.write(data)

print("unzip:", filename)
with zipfile.ZipFile(filename) as zf:
    zf.extractall("asset")
