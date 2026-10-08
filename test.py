import sys
import yaml

with open('.github/workflows/build.yml', 'r') as f:
    data = f.read()

print(data)
