#include "selfdrive/ui/qt/onroad/hud.h"

#include <cmath>

#include "selfdrive/ui/qt/util.h"

constexpr int SET_SPEED_NA = 255;

HudRenderer::HudRenderer() {}

void HudRenderer::updateState(const UIState &s) {
  is_metric = s.scene.is_metric;
  status = s.status;

  const SubMaster &sm = *(s.sm);
  if (sm.rcv_frame("carState") < s.scene.started_frame) {
    is_cruise_set = false;
    set_speed = SET_SPEED_NA;
    speed = 0.0;
    return;
  }

  const auto &controls_state = sm["controlsState"].getControlsState();
  const auto &car_state = sm["carState"].getCarState();

  // Handle older routes where vCruiseCluster is not set
  set_speed = car_state.getVCruiseCluster() == 0.0 ? controls_state.getVCruiseDEPRECATED() : car_state.getVCruiseCluster();
  is_cruise_set = set_speed > 0 && set_speed != SET_SPEED_NA;

  if (is_cruise_set && !is_metric) {
    set_speed *= KM_TO_MILE;
  }

  // Handle older routes where vEgoCluster is not set
  v_ego_cluster_seen = v_ego_cluster_seen || car_state.getVEgoCluster() != 0.0;
  float v_ego = v_ego_cluster_seen ? car_state.getVEgoCluster() : car_state.getVEgo();
  speed = std::max<float>(0.0f, v_ego * (is_metric ? MS_TO_KPH : MS_TO_MPH));

  // AleSato stuff
  enginerpm = sm["carState"].getCarState().getEngineRpm();
  engineColorSpeed = enginerpm > 0;
  float distance_traveled = sm["selfdriveState"].getSelfdriveState().getDistanceTraveled() / 1000;
  if(!s.scene.is_metric) {distance_traveled *= KM_TO_MILE;}
  distanceTraveled = distance_traveled;
  // End AleSato stuff
}

void HudRenderer::draw(QPainter &p, const QRect &surface_rect) {
  p.save();

  // Draw header gradient
  QLinearGradient bg(0, UI_HEADER_HEIGHT - (UI_HEADER_HEIGHT / 2.5), 0, UI_HEADER_HEIGHT);
  bg.setColorAt(0, QColor::fromRgbF(0, 0, 0, 0.45));
  bg.setColorAt(1, QColor::fromRgbF(0, 0, 0, 0));
  p.fillRect(0, 0, surface_rect.width(), UI_HEADER_HEIGHT, bg);


  drawSetSpeed(p, surface_rect);
  drawCurrentSpeed(p, surface_rect);

  p.restore();
}

void HudRenderer::drawSetSpeed(QPainter &p, const QRect &surface_rect) {
  // Draw outer box + border to contain set speed
  const QSize default_size = {172, 204};
  QSize set_speed_size = is_metric ? QSize(200, 204) : default_size;
  QRect set_speed_rect(QPoint(60 + (default_size.width() - set_speed_size.width()) / 2, 45), set_speed_size);

  // Draw set speed box
  p.setPen(QPen(QColor(255, 255, 255, 75), 6));
  p.setBrush(QColor(0, 0, 0, 166));
  p.drawRoundedRect(set_speed_rect, 32, 32);

  // Colors based on status
  QColor max_color = QColor(0xa6, 0xa6, 0xa6, 0xff);
  QColor set_speed_color = QColor(0x72, 0x72, 0x72, 0xff);
  if (is_cruise_set) {
    set_speed_color = QColor(255, 255, 255);
    if (status == STATUS_DISENGAGED) {
      max_color = QColor(255, 255, 255);
    } else if (status == STATUS_OVERRIDE) {
      max_color = QColor(0x91, 0x9b, 0x95, 0xff);
    } else {
      max_color = QColor(0x80, 0xd8, 0xa6, 0xff);
    }
  }

  // Draw "MAX" text
  p.setFont(InterFont(40, QFont::DemiBold));
  p.setPen(max_color);
  p.drawText(set_speed_rect.adjusted(0, 27, 0, 0), Qt::AlignTop | Qt::AlignHCenter, tr("MAX"));

  // Draw set speed
  QString setSpeedStr = is_cruise_set ? QString::number(std::nearbyint(set_speed)) : "–";
  p.setFont(InterFont(90, QFont::Bold));
  p.setPen(set_speed_color);
  p.drawText(set_speed_rect.adjusted(0, 77, 0, 0), Qt::AlignTop | Qt::AlignHCenter, setSpeedStr);

  // Begin Ale Sato
  QString engineRPMStr = engineColorSpeed? QString::number(std::nearbyint(enginerpm)) : "OFF";
  int my_rect_width = 344;
  int my_rect_height = 204;
  int my_top_radius = 32;
  int my_bottom_radius = 32;

  QRect my_engine_rpm_rect(20, 450, my_rect_width, my_rect_height);
  p.setPen(QPen(whiteColor(75), 6));
  p.setBrush(blackColor(166));
  drawRoundedRect(p, my_engine_rpm_rect, my_top_radius, my_top_radius, my_bottom_radius, my_bottom_radius);

  // Draw colored ENGINE RPM
  p.setPen(interpColor(
    enginerpm,
    {1500, 2100, 3000},
    {QColor(0x80, 0xd8, 0xa6, 0xff), QColor(0xff, 0xe4, 0xbf, 0xff), QColor(0xff, 0xbf, 0xbf, 0xff)}
  ));
  p.setFont(InterFont(40, QFont::DemiBold));
  p.drawText(my_engine_rpm_rect.adjusted(0, 97, 0, 0), Qt::AlignTop | Qt::AlignCenter, tr("ENGINE RPM"));

  // Draw colored RPM numbers
  if (engineColorSpeed) {
    p.setPen(interpColor(
      enginerpm,
      {1500, 2100, 3000},
      {whiteColor(), QColor(0xff, 0x95, 0x00, 0xff), QColor(0xff, 0x00, 0x00, 0xff)}
    ));
  } else {
    p.setPen(QColor(0x72, 0x72, 0x72, 0xff));
  }
  p.setFont(InterFont(90, QFont::Bold));
  p.drawText(my_engine_rpm_rect.adjusted(0, 17, 0, 0), Qt::AlignTop | Qt::AlignHCenter,  engineRPMStr);
  // End AleSato

// Begin2 Ale Sato
  char distanceTraveledStr[16];
  snprintf(distanceTraveledStr, sizeof(distanceTraveledStr), "%.1f", distanceTraveled);

  // Draw outer box + border to contain set speed and speed limit
  int my2_rect_width = 344;
  int my2_rect_height = 204;
  int my2_top_radius = 32;
  int my2_bottom_radius = 32;

  QRect my_trip_distance_rect(surface_rect.width() - 367, 450, my2_rect_width, my2_rect_height);
  p.setPen(QPen(whiteColor(75), 6));
  p.setBrush(blackColor(166));
  drawRoundedRect(p, my_trip_distance_rect, my2_top_radius, my2_top_radius, my2_bottom_radius, my2_bottom_radius);

  // Draw colored TRIP DIST
  p.setPen(interpColor(
    distanceTraveled,
    {3, 5, 10},
    {QColor(0xff, 0xbf, 0xbf, 0xff), QColor(0xff, 0xe4, 0xbf, 0xff), QColor(0x80, 0xd8, 0xa6, 0xff)}
  ));
  p.setFont(InterFont(40, QFont::DemiBold));
  p.drawText(my_trip_distance_rect.adjusted(0, 97, 0, 0), Qt::AlignTop | Qt::AlignCenter, tr("TRIP DIST"));

  // Draw trip distance
  p.setPen(interpColor(
    distanceTraveled,
    {3, 5, 10},
    {QColor(0xff, 0x00, 0x00, 0xff), QColor(0xff, 0x95, 0x00, 0xff), whiteColor()}
  ));
  p.setFont(InterFont(90, QFont::Bold));
  p.drawText(my_trip_distance_rect.adjusted(0, 17, 0, 0), Qt::AlignTop | Qt::AlignHCenter,  distanceTraveledStr);
  // End2 AleSato
}

void HudRenderer::drawCurrentSpeed(QPainter &p, const QRect &surface_rect) {
  QString speedStr = QString::number(std::nearbyint(speed));

  // p.setFont(InterFont(176, QFont::Bold));
  p.setFont(InterFont(230, QFont::Bold));
  // Turning the speed blue
  // drawText(p, surface_rect.center().x(), 210, speedStr);
  drawTextWithColor(p, surface_rect.center().x(), 210, speedStr, engineColorSpeed ? whiteColor() : QColor(20, 255, 20, 255));

  p.setFont(InterFont(66));
  drawText(p, surface_rect.center().x(), 290, is_metric ? tr("km/h") : tr("mph"), 200);
}

void HudRenderer::drawText(QPainter &p, int x, int y, const QString &text, int alpha) {
  QRect real_rect = p.fontMetrics().boundingRect(text);
  real_rect.moveCenter({x, y - real_rect.height() / 2});

  p.setPen(QColor(0xff, 0xff, 0xff, alpha));
  p.drawText(real_rect.x(), real_rect.bottom(), text);
}

void HudRenderer::drawTextWithColor(QPainter &p, int x, int y, const QString &text, QColor color) {
  QRect real_rect = p.fontMetrics().boundingRect(text);
  real_rect.moveCenter({x, y - real_rect.height() / 2});

  p.setPen(color);
  p.drawText(real_rect.x(), real_rect.bottom(), text);
}
