import os, sys, subprocess

if len(sys.argv) < 2:
	print("Usage: python3 upload_craft.py skycheckers_1.x")
	sys.exit(1)

file_prefix = sys.argv[1]

for suffix in ["amd64", "arm64", "i386"]:
	filename = file_prefix + "_" + suffix + ".snap"
	subprocess.run(["snapcraft", "upload", "--release=stable", filename])

