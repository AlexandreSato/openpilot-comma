#!/usr/bin/env python3
from common.text_window import TextWindow
import subprocess
from openpilot.common.basedir import BASEDIR
import os

if __name__ == '__main__':
  os.system("pkill -f pandad")
  with TextWindow(subprocess.Popen("./extract_key.py", shell=True, cwd=os.path.join(BASEDIR), stdout=subprocess.PIPE).stdout.read()) as t:
    t.wait_for_exit()