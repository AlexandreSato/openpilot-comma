#!/usr/bin/env python3
import time
from cereal import car, log, messaging
from cereal.services import SERVICE_LIST
from openpilot.selfdrive.controls.lib.events import EVENTS, Alert
from openpilot.common.realtime import DT_CTRL
from openpilot.system.manager.process_config import managed_processes

# For a specific alert
from openpilot.selfdrive.controls.lib.events import ET, Events
from openpilot.selfdrive.controls.lib.alertmanager import AlertManager

CP = car.CarParams.new_message()
CS = car.CarState.new_message()
sm = messaging.SubMaster(list(SERVICE_LIST.keys()))
pm = messaging.PubMaster(list(SERVICE_LIST.keys()))

EventName = car.CarEvent.EventName
specific_alerts = [
  # Include here your events to test
  (EventName.startupNoControl, ET.NO_ENTRY),
  # (EventName.personalityChanged, ET.WARNING),
  (EventName.calibrationInvalid, ET.PERMANENT),
  (EventName.stockAeb, ET.PERMANENT),
  # (EventName.stockAeb, ET.NO_ENTRY),
  (EventName.fcw, ET.PERMANENT),
]
duration = 200
is_metric = True


def publish_alert(alert):
  print(f'alertText1: {alert.alert_text_1}')
  print(f'alertText2: {alert.alert_text_2}')
  print(f'alertType: {alert.alert_type}')
  print('')
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


def create_specific_onroad_alerts():
  events = Events()
  AM = AlertManager()
  frame = 0
  for i in range(1):
  # for i in range(3):
    # pers = {v: k for k, v in log.LongitudinalPersonality.schema.enumerants.items()}[i]
    for alert, et in specific_alerts:
      events.clear()
      events.add(alert)
      # a = events.create_alerts([et, ], [CP, CS, sm, is_metric, 0, pers])
      a = events.create_alerts([et, ], [CP, CS, sm, is_metric, 0, log.LongitudinalPersonality.standard])
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
  try:
    while True:
      cycle_onroad_alerts()
      # create_all_onroad_alerts()
  except KeyboardInterrupt:
    managed_processes['ui'].stop()
