#!/usr/bin/env python3
import time
import sys
from cereal import car, log, messaging
from cereal.services import SERVICE_LIST
from openpilot.selfdrive.controls.lib.events import EVENTS, Alert
from openpilot.common.realtime import DT_CTRL
from openpilot.system.manager.process_config import managed_processes

# For a specific alert
from openpilot.selfdrive.controls.lib.events import ET, Events
from openpilot.selfdrive.controls.lib.alertmanager import AlertManager

# For click
from collections import namedtuple
import pyautogui

# For generate a background image
from msgq.visionipc import VisionIpcServer, VisionStreamType
from openpilot.common.transformations.camera import CameraConfig, DEVICE_CAMERAS
from openpilot.tools.lib.logreader import LogReader
from openpilot.tools.lib.framereader import FrameReader
from openpilot.tools.lib.route import Route
from openpilot.selfdrive.ui.tests.test_ui.run import DATA, STREAMS, TEST_ROUTE
from openpilot.selfdrive.test.process_replay.migration import migrate_selfdriveState


CP = car.CarParams.new_message()
CS = car.CarState.new_message()
sm = messaging.SubMaster(list(SERVICE_LIST.keys()))
pm = messaging.PubMaster(list(SERVICE_LIST.keys()))

EventName = car.CarEvent.EventName
specific_alerts = [
  # Include here your events to test
  (EventName.calibrationInvalid, ET.PERMANENT),
  (EventName.startupNoControl, ET.NO_ENTRY),
  (EventName.promptDriverDistracted, ET.PERMANENT),
  # (EventName.stockAeb, ET.NO_ENTRY),
  # (EventName.fcw, ET.PERMANENT),
  (EventName.personalityChanged, ET.WARNING), # this guy needs to be tested separately (und uncoment in range(3))
]
duration = 200
is_metric = True


def setup_onroad() -> None:
  segnum = 2
  route = Route(TEST_ROUTE)
  lr = LogReader(route.qlog_paths()[segnum])
  DATA['carParams'] = next((event.as_builder() for event in lr if event.which() == 'carParams'), None)
  for event in migrate_selfdriveState(lr):
    if event.which() in DATA:
      DATA[event.which()] = event.as_builder()

    if all(DATA.values()):
      break
  cam = DEVICE_CAMERAS[("tici", "ar0231")]
  road_img = FrameReader(route.camera_paths()[segnum]).get(0, pix_fmt="nv12")[0]
  STREAMS.append((VisionStreamType.VISION_STREAM_ROAD, cam.fcam, road_img.flatten().tobytes()))
  vipc_server = VisionIpcServer("camerad")
  for stream_type, cam, _ in STREAMS:
    vipc_server.create_buffers(stream_type, 5, False, cam.width, cam.height)
  vipc_server.start_listener()
  packet_id = 0
  for _ in range(20):
    for service, data in DATA.items():
      if data:
        data.clear_write_flag()
        pm.send(service, data)

    packet_id = packet_id + 1
    for stream_type, _, image in STREAMS:
      vipc_server.send(stream_type, image, packet_id, packet_id, packet_id)

    time.sleep(0.05)

def click(x, y) -> None:
  sys.modules["mouseinfo"] = False
  ui = namedtuple("bb", ["left", "top", "width", "height"])(0,0,2160,1080)
  pyautogui.click(ui.left + x, ui.top + y)
  time.sleep(DT_CTRL) # give enough time for the UI to react

def publish_alert(alert):
  print(f'alertText1: {alert.alert_text_1}')
  print(f'alertText2: {alert.alert_text_2}')
  print(f'alertType: {alert.alert_type}')
  print('')
  click(500, 500)
  for _ in range(duration):
    dat = messaging.new_message('selfdriveState')
    dat.selfdriveState.enabled = False
    dat.selfdriveState.alertText1 = alert.alert_text_1
    dat.selfdriveState.alertText2 = alert.alert_text_2
    dat.selfdriveState.alertSize = alert.alert_size
    dat.selfdriveState.alertStatus = alert.alert_status
    dat.selfdriveState.alertType = alert.alert_type
    dat.selfdriveState.alertSound = alert.audible_alert
    pm.send('selfdriveState', dat)
    dat = messaging.new_message('deviceState')
    dat.deviceState.started = True
    pm.send('deviceState', dat)
    dat = messaging.new_message('pandaStates', 1)
    dat.pandaStates[0].ignitionLine = True
    dat.pandaStates[0].pandaType = log.PandaState.PandaType.dos
    pm.send('pandaStates', dat)
    time.sleep(DT_CTRL)
    if _ == duration // 2:
      click(500, 500)


def create_specific_onroad_alerts():
  events = Events()
  AM = AlertManager()
  frame = 0
  for i in range(1):
  # for i in range(3): # uncoment this to test only EventName.personalityChanged (remove anothers from the list)
    pers = {v: k for k, v in log.LongitudinalPersonality.schema.enumerants.items()}[i]
    for alert, et in specific_alerts:
      events.clear()
      events.add(alert)
      a = events.create_alerts([et, ], [CP, CS, sm, is_metric, 0, pers])
      AM.add_many(frame, a)
      alert = AM.process_alerts(frame, [])
      if alert:
        publish_alert(alert)
      frame += 1


def create_all_onroad_alerts():
  for event in EVENTS.values():
    for alert in event.values():
      if not isinstance(alert, Alert):
        alert = alert(CP, CS, sm, is_metric, 0, log.LongitudinalPersonality.standard)
        if alert:
          publish_alert(alert)


def cycle_onroad_alerts():
  create_specific_onroad_alerts()


if __name__ == '__main__':
  managed_processes['ui'].start()
  setup_onroad()
  try:
    while True:
      cycle_onroad_alerts()
      # create_all_onroad_alerts()
  except KeyboardInterrupt:
    managed_processes['ui'].stop()
