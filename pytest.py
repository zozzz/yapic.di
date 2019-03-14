import os
import sys

args = ""
if len(sys.argv) > 1:
    args = "-- " + " ".join(sys.argv[1:])

print("%s %s %s" % (sys.executable, "setup.py test", args))
os.system("%s %s %s" % (sys.executable, "setup.py test", args))
