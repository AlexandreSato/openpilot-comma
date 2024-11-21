#include <QDebug>

#include "selfdrive/ui/qt/offroad/developer_panel.h"
#include "selfdrive/ui/qt/widgets/ssh_keys.h"
#include "selfdrive/ui/qt/widgets/controls.h"
#include "common/util.h"

DeveloperPanel::DeveloperPanel(SettingsWindow *parent) : ListWidget(parent) {
  // SSH keys
  addItem(new SshToggle());
  addItem(new SshControl());

  joystickToggle = new ParamControl("JoystickDebugMode", tr("Joystick Debug Mode"), "", "");
  QObject::connect(joystickToggle, &ParamControl::toggleFlipped, [=](bool state) {
    params.putBool("LongitudinalManeuverMode", false);
    longManeuverToggle->refresh();
  });
  addItem(joystickToggle);

  longManeuverToggle = new ParamControl("LongitudinalManeuverMode", tr("Longitudinal Maneuver Mode"), "", "");
  QObject::connect(longManeuverToggle, &ParamControl::toggleFlipped, [=](bool state) {
    params.putBool("JoystickDebugMode", false);
    joystickToggle->refresh();
  });
  addItem(longManeuverToggle);

  alphaLongToggle = new ParamControl("ExperimentalLongitudinalEnabled", tr("openpilot Longitudinal Control (Alpha)"), "", "../assets/offroad/icon_speed_limit.png");
  QObject::connect(alphaLongToggle, &ParamControl::toggleFlipped, [=](bool state) {
    updateToggles(offroad);
  });
  addItem(alphaLongToggle);
  alphaLongToggle->setConfirmation(true, false);

  // Joystick and longitudinal maneuvers should be hidden on release branches
  is_release = params.getBool("IsReleaseBranch");

  // Toggles should be not available to change in onroad state
  QObject::connect(uiState(), &UIState::offroadTransition, this, &DeveloperPanel::updateToggles);
}

void DeveloperPanel::updateToggles(bool _offroad) {
  for (auto btn : findChildren<ParamControl *>()) {
    btn->setVisible(!is_release);
    btn->setEnabled(_offroad);
  }

  // longManeuverToggle should not be toggleable if the car don't have longitudinal control
  auto cp_bytes = params.get("CarParamsPersistent");
  if (!cp_bytes.empty()) {
    AlignedBuffer aligned_buf;
    capnp::FlatArrayMessageReader cmsg(aligned_buf.align(cp_bytes.data(), cp_bytes.size()));
    cereal::CarParams::Reader CP = cmsg.getRoot<cereal::CarParams>();

    const QString alpha_long_description = QString("<b>%1</b><br><br>%2")
      .arg(tr("WARNING: openpilot longitudinal control is in alpha for this car and will disable Automatic Emergency Braking (AEB)."))
      .arg(tr("On this car, openpilot defaults to the car's built-in ACC instead of openpilot's longitudinal control. "
              "Enable this to switch to openpilot longitudinal control. Enabling Experimental mode is recommended when enabling openpilot longitudinal control alpha."));
    alphaLongToggle->setDescription("<b>" + alpha_long_description + "</b>");

    if (!CP.getExperimentalLongitudinalAvailable() && !CP.getOpenpilotLongitudinalControl()) {
      params.remove("ExperimentalLongitudinalEnabled");
      alphaLongToggle->setEnabled(false);
      alphaLongToggle->setDescription("<b>" + tr("openpilot longitudinal control may come in a future update.") + "</b>");
    } else {
      if (is_release) {
        params.remove("ExperimentalLongitudinalEnabled");
        alphaLongToggle->setEnabled(false);
        alphaLongToggle->setDescription("<b>" + tr("An alpha version of openpilot longitudinal control can be tested, along with Experimental mode, on non-release branches.") + "</b>");
      }
    }

    // The car already have openpilot longitudinal control and the toggle not is necessary
    if (CP.getOpenpilotLongitudinalControl()) {
      alphaLongToggle->setVisible(false);
    }
    alphaLongToggle->refresh();
    longManeuverToggle->setEnabled(hasLongitudinalControl(CP) && _offroad);
  } else {
    alphaLongToggle->setDescription("<b>" + tr("openpilot longitudinal control may come in a future update.") + "</b>");
    longManeuverToggle->setEnabled(false);
  }

  offroad = _offroad;
}

void DeveloperPanel::showEvent(QShowEvent *event) {
  updateToggles(offroad);
}
