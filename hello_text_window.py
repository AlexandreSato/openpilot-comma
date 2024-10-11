#!/usr/bin/env python3
from common.text_window import TextWindow
import subprocess
from openpilot.common.basedir import BASEDIR
import os
from openpilot.system.manager.process_config import managed_processes

if __name__ == '__main__':
  managed_processes['pandad'].stop()
  text = """https://icanhack.nl/"""
  out = subprocess.run("./justprintsomething.py", cwd=os.path.join(BASEDIR), capture_output=True, shell=True, check=False, encoding='utf8').stdout
  text += "\n"
  text += out
  print(f'debug: {out}')
  with TextWindow(text) as t:
    t.wait_for_exit()
    managed_processes['pandad'].start()