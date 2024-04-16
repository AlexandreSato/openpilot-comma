#include "selfdrive/ui/qt/onroad/onroad_home.h"

#include <QPainter>

#ifdef ENABLE_MAPS
#include "selfdrive/ui/qt/maps/map_helpers.h"
#include "selfdrive/ui/qt/maps/map_panel.h"
#endif

#include "selfdrive/ui/qt/util.h"

OnroadWindow::OnroadWindow(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *main_layout  = new QVBoxLayout(this);
  main_layout->setMargin(UI_BORDER_SIZE);
  QStackedLayout *stacked_layout = new QStackedLayout;
  stacked_layout->setStackingMode(QStackedLayout::StackAll);
  main_layout->addLayout(stacked_layout);

  nvg = new AnnotatedCameraWidget(VISION_STREAM_ROAD, this);

  QWidget * split_wrapper = new QWidget;
  split = new QHBoxLayout(split_wrapper);
  split->setContentsMargins(0, 0, 0, 0);
  split->setSpacing(0);
  split->addWidget(nvg);

  if (getenv("DUAL_CAMERA_VIEW")) {
    CameraWidget *arCam = new CameraWidget("camerad", VISION_STREAM_ROAD, true, this);
    split->insertWidget(0, arCam);
  }

  if (getenv("MAP_RENDER_VIEW")) {
    CameraWidget *map_render = new CameraWidget("navd", VISION_STREAM_MAP, false, this);
    split->insertWidget(0, map_render);
  }

  stacked_layout->addWidget(split_wrapper);

  alerts = new OnroadAlerts(this);
  alerts->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  stacked_layout->addWidget(alerts);

  // setup stacking order
  alerts->raise();

  setAttribute(Qt::WA_OpaquePaintEvent);
  QObject::connect(uiState(), &UIState::uiUpdate, this, &OnroadWindow::updateState);
  QObject::connect(uiState(), &UIState::offroadTransition, this, &OnroadWindow::offroadTransition);
  QObject::connect(uiState(), &UIState::primeChanged, this, &OnroadWindow::primeChanged);
}

void OnroadWindow::updateState(const UIState &s) {
  if (!s.scene.started) {
    return;
  }

  if (s.scene.map_on_left) {
    split->setDirection(QBoxLayout::LeftToRight);
  } else {
    split->setDirection(QBoxLayout::RightToLeft);
  }

  alerts->updateState(s);
  nvg->updateState(s);

  QColor bgColor = (Params("/dev/shm/params").getBool("AleSato_SteerAlwaysOn") && s.status != STATUS_ENGAGED)? bg_colors[STATUS_OVERRIDE] : bg_colors[s.status];
  if (bg != bgColor) {
    // repaint border
    bg = bgColor;
    update();
  }

  // Ale Sato blinker indicator at borders
  UIState *my_s = uiState();
  if (s.scene.blinkerstatus || my_s->scene.prev_blinkerstatus) {
    update();
    my_s->scene.prev_blinkerstatus = s.scene.blinkerstatus;
    my_s->scene.blinkerframe += my_s->scene.blinkerframe < 255? +20 : -255;
  }
}

void OnroadWindow::mousePressEvent(QMouseEvent* e) {
#ifdef ENABLE_MAPS
  if (map != nullptr) {
    bool sidebarVisible = geometry().x() > 0;
    bool show_map = !sidebarVisible;
    map->setVisible(show_map && !map->isVisible());
  }
#endif
  // propagation event to parent(HomeWindow)
  QWidget::mousePressEvent(e);
}

void OnroadWindow::createMapWidget() {
#ifdef ENABLE_MAPS
  auto m = new MapPanel(get_mapbox_settings());
  map = m;
  QObject::connect(m, &MapPanel::mapPanelRequested, this, &OnroadWindow::mapPanelRequested);
  QObject::connect(nvg->map_settings_btn, &MapSettingsButton::clicked, m, &MapPanel::toggleMapSettings);
  nvg->map_settings_btn->setEnabled(true);

  m->setFixedWidth(topWidget(this)->width() / 2 - UI_BORDER_SIZE);
  split->insertWidget(0, m);
  // hidden by default, made visible when navRoute is published
  m->setVisible(false);
#endif
}

void OnroadWindow::offroadTransition(bool offroad) {
#ifdef ENABLE_MAPS
  if (!offroad) {
    if (map == nullptr && (uiState()->hasPrime() || !MAPBOX_TOKEN.isEmpty())) {
      createMapWidget();
    }
  }
#endif
  alerts->clear();
}

void OnroadWindow::primeChanged(bool prime) {
#ifdef ENABLE_MAPS
  if (map && (!prime && MAPBOX_TOKEN.isEmpty())) {
    nvg->map_settings_btn->setEnabled(false);
    nvg->map_settings_btn->setVisible(false);
    map->deleteLater();
    map = nullptr;
  } else if (!map && (prime || !MAPBOX_TOKEN.isEmpty())) {
    createMapWidget();
  }
#endif
}

void OnroadWindow::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  p.fillRect(rect(), QColor(bg.red(), bg.green(), bg.blue(), 255));

  // Begin AleSato Blinker Indicator
  p.setPen(Qt::NoPen);
  UIState *s = uiState();
  p.setBrush(QBrush(QColor(0, 0, 0, 0xff)));
  if (s->scene.blinkerstatus == 1) {
    // left rectangle for blinker indicator
    float rightcorner = width() * 0.75;
    QRect blackground = QRect(0, height()*0.75, rightcorner, height());
    p.drawRect(blackground);
    float bottomsect = rightcorner / (rightcorner + (height()/4)); // time proportion
    float delta = 1 - (float(s->scene.blinkerframe)/(255*bottomsect));
    delta = std::clamp(delta, 0.0f, 1.0f);
    QRect r = QRect(rightcorner*delta, height()-30, rightcorner-(rightcorner*delta), 30);
    p.setBrush(QBrush(QColor(255, 150, 0, 255)));
    p.drawRect(r);
    float delta2 = (float(s->scene.blinkerframe) - float(255 * bottomsect)) / (255 * (1 - bottomsect));
    delta2 = std::clamp(delta2, 0.0f, 1.0f);
    r = QRect(0, height() - height()*0.25*delta2, 30, height());
    p.drawRect(r);
  } else if (s->scene.blinkerstatus == 2) {
    // right rectangle for blinker indicator
    float leftcorner = width() * 0.25;
    QRect blackground = QRect(leftcorner, height()*0.75, width(), height());
    p.drawRect(blackground);
    float bottomsect = (width() - leftcorner) / (width() - leftcorner + (height()/4)); // time proportion
    float delta = float(s->scene.blinkerframe)/(255*bottomsect);
    delta = std::clamp(delta, 0.0f, 1.0f);
    QRect r = QRect(leftcorner, height()-30, (width()-leftcorner)*delta, 30);
    p.setBrush(QBrush(QColor(255, 150, 0, 255)));
    p.drawRect(r);
    float delta2 = (float(s->scene.blinkerframe) - float(255 * bottomsect)) / (255 * (1 - bottomsect));
    delta2 = std::clamp(delta2, 0.0f, 1.0f);
    r = QRect(width()-30, height() - height()*0.25*delta2, width(), height());
    p.drawRect(r);
  }
  // End AleSato Blinker Indicator
}
