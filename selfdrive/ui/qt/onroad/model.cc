#include "selfdrive/ui/qt/onroad/model.h"

#include "selfdrive/ui/qt/util.h"

constexpr int CLIP_MARGIN = 500;
constexpr float MIN_DRAW_DISTANCE = 10.0;
constexpr float MAX_DRAW_DISTANCE = 100.0;


// Ichiro Stuff
struct LeadcarLockon {
  float x,y,d,a,lxt,lxf,lockOK;
};
#define LeadcarLockon_MAX 2
LeadcarLockon leadcar_lockon[LeadcarLockon_MAX]; //この配列0番を推論1番枠と呼ぶことにする。

void ModelRenderer::drawLockon(QPainter &painter, const cereal::ModelDataV2::LeadDataV3::Reader &lead_data, const QPointF &vd , int num  /*使っていない , size_t leads_num , const cereal::RadarState::LeadData::Reader &lead0, const cereal::RadarState::LeadData::Reader &lead1 */, const QRect &surface_rect) {
  //const float speedBuff = 10.;
  //const float leadBuff = 40.;
  const float d_rel = lead_data.getX()[0];
  //const float d_rel = lead_data.getDRel();
  //const float v_rel = lead_data.getV()[0];
  //const float t_rel = lead_data.getT()[0];
  //const float y_rel = lead_data.getY()[0];
  float a_rel = lead_data.getA()[0];
  //float a_rel = lead_data.getARel(); //ある？
  // global_a_rel = a_rel;

  float sz = std::clamp((25 * 30) / (d_rel / 3 + 30), 15.0f, 30.0f) * 2.35;
  float x = std::clamp((float)vd.x(), 0.f, surface_rect.width() - sz / 2);
  //float y = std::fmin(height() /*- sz * .6*/, (float)vd.y());
  float y = (float)vd.y();

  //float g_xo = sz / 5;
  //float g_yo = sz / 10;

  //QPointF glow[] = {{x + (sz * 1.35) + g_xo, y + sz + g_yo}, {x, y - g_yo}, {x - (sz * 1.35) - g_xo, y + sz + g_yo}};

  painter.setCompositionMode(QPainter::CompositionMode_Plus);
  //p.setPen(QColor(0, 255, 0, 255));

  float prob_alpha = lead_data.getProb(); //getModelProb();
  if(prob_alpha < 0){
    prob_alpha = 0;
  } else if(prob_alpha > 1.0){
    prob_alpha = 1.0;
  }
  prob_alpha *= 245;

  painter.setPen(QPen(QColor(0.09*255, 0.945*255, 0.26*255, prob_alpha), 2));
  painter.setBrush(QColor(0, 0, 0, 0));
  float ww = 300 , hh = 300;
  if(Hardware::TICI()){
    ww *= 1.25; hh *= 1.25;
  }
  float d = d_rel; //距離をロックターケットの大きさに反映させる。
  if(d < 1){
    d = 1;
  }

  //動きに緩衝処理。
  leadcar_lockon[num].x = leadcar_lockon[num].x + (x - leadcar_lockon[num].x) / 6;
  leadcar_lockon[num].y = leadcar_lockon[num].y + (y - leadcar_lockon[num].y) / 6;
  leadcar_lockon[num].d = leadcar_lockon[num].d + (d - leadcar_lockon[num].d) / 6;
  x = leadcar_lockon[num].x;
  y = leadcar_lockon[num].y;
  d = leadcar_lockon[num].d;
  if(d < 1){
    d = 1;
  }

  leadcar_lockon[num].a = leadcar_lockon[num].a + (a_rel - leadcar_lockon[num].a) / 10;
  a_rel = leadcar_lockon[num].a;

  float dh = 50;
  if(uiState()->scene.wide_cam == false) { //dhに奥行き値を反映させる。
    float dd = d;
    dd -= 25; //dd=0〜75
    dd /= (75.0/2); //dd=0〜2
    dd += 1; //dd=1〜3
    if(dd < 1)dd = 1;
    dh /= dd;
  } else { //ワイドカメラ使用でロジック変更。リアルタイムで変わる。
    ww *= 0.5; hh *= 0.5;
    dh = 100;
    float dd = d;
    dd -= 5; //dd=0〜95
    dd /= (95.0/10); //dd=0〜10
    dd += 1; //dd=1〜11
    if(dd < 1)dd = 1;
    dh /= dd*dd;
  }

  ww = ww * 2 * 5 / d;
  hh = hh * 2 * 5 / d;
  y = std::fmin(surface_rect.height() /*- sz * .6*/, y - dh) + dh;
  QRect r = QRect(x - ww/2, y /*- g_yo*/ - hh - dh, ww, hh);

#if 0
  float y0 = lead0.getY()[0];
  float y1 = lead1.getY()[0];
#else
  //y?ってわかりにくいな。横方向なんだが。getYは使えなさそうだし。
  float y0 = leadcar_lockon[0].x * leadcar_lockon[0].d; //こうなったら画面座標から逆算。
  float y1 = leadcar_lockon[1].x * leadcar_lockon[1].d;
#endif

  painter.setFont(InterFont(38, QFont::DemiBold));
  if(num == 0 /* && uiState()->scene.mLockOnButton */){
    //推論1番
    painter.setPen(QPen(QColor(0.09*255, 0.945*255, 0.26*255, prob_alpha), 2));
    painter.drawRect(r);

    //painter.setPen(QPen(QColor(0.09*255, 0.945*255, 0.26*255, prob_alpha), 2));
    if(leadcar_lockon[0].x > leadcar_lockon[1].x - 20){
      leadcar_lockon[num].lxt = leadcar_lockon[num].lxt + (r.right() - leadcar_lockon[num].lxt) / 20;
      leadcar_lockon[num].lxf = leadcar_lockon[num].lxf + (surface_rect.width() - leadcar_lockon[num].lxf) / 20;
      //painter.drawLine(r.right(),r.top() , width() , 0);
    } else {
      leadcar_lockon[num].lxt = leadcar_lockon[num].lxt + (r.left() - leadcar_lockon[num].lxt) / 20;
      leadcar_lockon[num].lxf = leadcar_lockon[num].lxf + (0 - leadcar_lockon[num].lxf) / 20;
      //painter.drawLine(r.left(),r.top() , 0 , 0);
    }
    painter.drawText(r, Qt::AlignTop | Qt::AlignLeft, " " + QString::number(num+1));

    //painter.setPen(QPen(QColor(245, 245, 0, prob_alpha), 2));
    float lxt = leadcar_lockon[num].lxt;
    if(lxt < r.left()){
      lxt = r.left();
    } else if(lxt > r.right()){
      lxt = r.right();
    }
    painter.drawLine(lxt,r.top() , leadcar_lockon[num].lxf , 0);
    if(ww >= 40){
      //painter.drawText(r, Qt::AlignTop | Qt::AlignRight, QString::number((int)(lead_data.getProb()*100)) + "％");

      //num==0のロックオンの右端20ドットくらいをa_rel数値メーターとする。
      painter.setPen(Qt::NoPen);
      float wwa = ww * 0.15;
      if(wwa > 40){
        wwa = 40;
      } else if(wwa < 10){
        wwa = 10;
      }
      if(wwa > ww){
        wwa = ww;
      }

      float hha = 0;
      if(a_rel > 0){
        hha = 1 - 0.1 / a_rel;
        painter.setBrush(QColor(0.09*255, 0.945*255, 0.26*255, prob_alpha*0.9));

        if(hha < 0){
          hha = 0;
        }
        hha = hha * hh;
#if 0
        QRect ra = QRect(x - ww/2 + (ww - wwa), y /*- g_yo*/ - hh - dh + (hh-hha), wwa, hha);
        painter.drawRect(ra);
#else //メーターを斜めに切る
        QPointF meter[] = {{(float)x + ww/2 - wwa/2 - wwa/2 * hha / hh , (float)y /*- g_yo*/ - hh - dh + (hh-hha)},{(float)x + ww/2 , (float)y /*- g_yo*/ - hh - dh + (hh-hha)}, {(float)x + ww/2 , (float)y /*- g_yo*/ - hh - dh + hh}, {(float)x + ww/2 - wwa/2 , (float)y /*- g_yo*/ - hh - dh + hh}};
        painter.drawPolygon(meter, std::size(meter));
#endif
      }
      if(a_rel < 0){
        hha = 1 + 0.1 / a_rel;
        painter.setBrush(QColor(245, 0, 0, prob_alpha));
        //減速は上から下へ変更。
        if(hha < 0){
          hha = 0;
        }
        hha = hha * hh;
#if 0
        QRect ra = QRect(x - ww/2 + (ww - wwa), y /*- g_yo*/ - hh - dh , wwa, hha);
        painter.drawRect(ra);
#else //メーターを斜めに切る
        QPointF meter[] = {{(float)x + ww/2 - wwa/2 , (float)y /*- g_yo*/ - hh - dh},{(float)x + ww/2 , (float)y /*- g_yo*/ - hh - dh}, {(float)x + ww/2 , (float)y /*- g_yo*/ - hh - dh + hha}, {(float)x + ww/2 - wwa/2 - wwa/2 * hha / hh, (float)y /*- g_yo*/ - hh - dh + hha}};
        painter.drawPolygon(meter, std::size(meter));
#endif
      }
    }

    if(//lead0.getX()[0] > lead1.getX()[0] //lead1がlead0より後ろ
        //y0 > y1 //lead1がlead0より左
        std::abs(y0 - y1) <= 300 //大きく横にずれた→逆
        // ||ほかにv_relやa_relで前方の急減速を表示したり（num==0に表示してみた）
        //&& lead1.getX()[0] < 10 //lead1が自分の前10m以内
    ){
      leadcar_lockon[num].lockOK = leadcar_lockon[num].lockOK + (40 - leadcar_lockon[num].lockOK) / 5;
      //float td = 40;
    } else {
      leadcar_lockon[num].lockOK = leadcar_lockon[num].lockOK + (0 - leadcar_lockon[num].lockOK) / 5;
    }
    float td = leadcar_lockon[num].lockOK;
    //d:10〜100->1〜3へ変換
    if(td >= 3){
      float dd = leadcar_lockon[num].d;
      if(dd < 10){
        dd = 10;
      }
      dd -= 10; //dd=0〜90
      dd /= (90.0/2); //dd=0〜2
      dd += 1; //dd=1〜3
      td /= dd;

      float tlw = 8;
      float tlw_2 = tlw / 2;
      painter.setPen(QPen(QColor(0.09*255, 0.945*255, 0.26*255, prob_alpha), tlw));
      painter.drawLine(r.center().x() , r.top()-tlw_2 , r.center().x() , r.top() - td);
      painter.drawLine(r.left()-tlw_2 , r.center().y() , r.left() - td , r.center().y());
      painter.drawLine(r.right()+tlw_2 , r.center().y() , r.right() + td , r.center().y());
      painter.drawLine(r.center().x() , r.bottom()+tlw_2 , r.center().x() , r.bottom() + td);
    }

  } else if(/* uiState()->scene.mLockOnButton */ true){
    if(num == 1){
      //推論2番
      //邪魔な前右寄りを走るバイクを認識したい。
      if(//lead0.getX()[0] > lead1.getX()[0] //lead1がlead0より後ろ
        //y0 > y1 //lead1がlead0より左
        std::abs(y0 - y1) > 300 //大きく横にずれた
        // ||ほかにv_relやa_relで前方の急減速を表示したり（num==0に表示してみた）
        //&& lead1.getX()[0] < 10 //lead1が自分の前10m以内
      ){
        //painter.setPen(QPen(QColor(245, 0, 0, prob_alpha), 4));
        //painter.drawEllipse(r); //縁を描く
        //painter.setPen(QPen(QColor(0.09*255, 0.945*255, 0.26*255, prob_alpha), 1)); //文字を後で書くために色を再設定。->文字は赤でもいいや

        //円を（意味不明だから）書かないで、枠ごと赤くする。推論1が推論と別のものを捉えてるのを簡単に認識できる。
        painter.setPen(QPen(QColor(245, 0, 0, prob_alpha), 2));
      } else {
        painter.setPen(QPen(QColor(0.09*255, 0.945*255, 0.26*255, prob_alpha), 2));
      }

      if(leadcar_lockon[0].x > leadcar_lockon[1].x - 20){ //多少逆転しても許容する
        leadcar_lockon[num].lxt = leadcar_lockon[num].lxt + (r.left() - leadcar_lockon[num].lxt) / 20;
        leadcar_lockon[num].lxf = leadcar_lockon[num].lxf + (0 - leadcar_lockon[num].lxf) / 20;
        //painter.drawLine(r.left(),r.top() , 0 , 0);
      } else {
        leadcar_lockon[num].lxt = leadcar_lockon[num].lxt + (r.right() - leadcar_lockon[num].lxt) / 20;
        leadcar_lockon[num].lxf = leadcar_lockon[num].lxf + (surface_rect.width() - leadcar_lockon[num].lxf) / 20;
        //painter.drawLine(r.right(),r.top() , width() , 0);
      }
      float lxt = leadcar_lockon[num].lxt;
      if(lxt < r.left()){
        lxt = r.left();
      } else if(lxt > r.right()){
        lxt = r.right();
      }
      painter.drawLine(lxt,r.top() , leadcar_lockon[num].lxf , 0);

      if(ww >= 80){
        //float dy = y0 - y1;
        //painter.drawText(r, Qt::AlignBottom | Qt::AlignLeft, " " + QString::number(dy,'f',1) + "m");
        //painter.drawText(r, Qt::AlignBottom | Qt::AlignLeft, " " + QString::number(dy,'f',1));
      }
    } else if(num == 2){
      //推論3番
      //事実上ない。動かない0,0に居るみたい？
      painter.setPen(QPen(QColor(0.09*255, 0.945*255, 0.26*255, prob_alpha), 2));
      //painter.drawLine(r.right(),r.center().y() , width() , height());
    } else {
      //推論4番以降。
      //存在していない。
      painter.setPen(QPen(QColor(0.09*255, 0.945*255, 0.26*255, prob_alpha), 2));
      //painter.drawLine(r.left(),r.center().y() , 0 , height());
    }

    painter.drawRect(r);

    //painter.setPen(QPen(QColor(0.09*255, 0.945*255, 0.26*255, prob_alpha), 2));

    if(ww >= 80){
      //ここではy0,y1を参照できない。
      float d_lim = 12;
      if(uiState()->scene.wide_cam == false){
        d_lim = 32; //ロングカメラだとちょっと枠が大きい。実測
      }
      if(num == 0 || (num==1 && (d_rel < d_lim || std::abs(y0 - y1) > 300))){ //num==1のとき、'2'の表示と前走車速度表示がかぶるので、こちらを消す。
        painter.drawText(r, Qt::AlignBottom | Qt::AlignLeft, " " + QString::number(num+1));
      }
    }
    if(ww >= 160 /*80*/){
      //painter.drawText(r, Qt::AlignBottom | Qt::AlignRight, QString::number((int)(lead_data.getProb()*100)) + "％");
      //painter.drawText(r, Qt::AlignBottom | Qt::AlignRight, QString::number(a_rel,'f',1) + "a");
    }
  }
  painter.setPen(Qt::NoPen);
  painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
}


static int get_path_length_idx(const cereal::XYZTData::Reader &line, const float path_height) {
  const auto &line_x = line.getX();
  int max_idx = 0;
  for (int i = 1; i < line_x.size() && line_x[i] <= path_height; ++i) {
    max_idx = i;
  }
  return max_idx;
}

void ModelRenderer::draw(QPainter &painter, const QRect &surface_rect) {
  auto *s = uiState();
  auto &sm = *(s->sm);
  // Check if data is up-to-date
  if (sm.rcv_frame("liveCalibration") < s->scene.started_frame ||
      sm.rcv_frame("modelV2") < s->scene.started_frame) {
    return;
  }

  clip_region = surface_rect.adjusted(-CLIP_MARGIN, -CLIP_MARGIN, CLIP_MARGIN, CLIP_MARGIN);
  experimental_mode = sm["selfdriveState"].getSelfdriveState().getExperimentalMode();
  longitudinal_control = sm["carParams"].getCarParams().getOpenpilotLongitudinalControl();

  painter.save();

  const auto &model = sm["modelV2"].getModelV2();
  const auto &radar_state = sm["radarState"].getRadarState();
  const auto &lead_one = radar_state.getLeadOne();

  update_model(model, lead_one);
  drawLaneLines(painter);
  drawPath(painter, model, surface_rect.height());

  if (longitudinal_control && sm.alive("radarState")) {
    // Begin Ichiro Stuff
    const auto leads = model.getLeadsV3();
    size_t leads_num = leads.size();
    for(size_t i=0; i<leads_num && i < LeadcarLockon_MAX; i++){
      if(leads[i].getProb() > .2){ //信用度20%以上で表示。調整中。
        drawLockon(painter, leads[i], lead_vertices[i] , i /*, leads_num , leads[0] , leads[1]*/, surface_rect);
      }
    }
    // End Ichiro Stuff
    update_leads(radar_state, model.getPosition());
    const auto &lead_two = radar_state.getLeadTwo();
    if (lead_one.getStatus()) {
      drawLead(painter, lead_one, lead_vertices[0], surface_rect);
    }
    if (lead_two.getStatus() && (std::abs(lead_one.getDRel() - lead_two.getDRel()) > 3.0)) {
      drawLead(painter, lead_two, lead_vertices[1], surface_rect);
    }
  }

  painter.restore();
}

void ModelRenderer::update_leads(const cereal::RadarState::Reader &radar_state, const cereal::XYZTData::Reader &line) {
  for (int i = 0; i < 2; ++i) {
    const auto &lead_data = (i == 0) ? radar_state.getLeadOne() : radar_state.getLeadTwo();
    if (lead_data.getStatus()) {
      float z = line.getZ()[get_path_length_idx(line, lead_data.getDRel())];
      mapToScreen(lead_data.getDRel(), -lead_data.getYRel(), z + 1.22, &lead_vertices[i]);
    }
  }
}

void ModelRenderer::update_model(const cereal::ModelDataV2::Reader &model, const cereal::RadarState::LeadData::Reader &lead) {
  const auto &model_position = model.getPosition();
  float max_distance = std::clamp(*(model_position.getX().end() - 1), MIN_DRAW_DISTANCE, MAX_DRAW_DISTANCE);

  // update lane lines
  const auto &lane_lines = model.getLaneLines();
  const auto &line_probs = model.getLaneLineProbs();
  int max_idx = get_path_length_idx(lane_lines[0], max_distance);
  for (int i = 0; i < std::size(lane_line_vertices); i++) {
    lane_line_probs[i] = line_probs[i];
    mapLineToPolygon(lane_lines[i], 0.025 * lane_line_probs[i], 0, &lane_line_vertices[i], max_idx);
  }

  // update road edges
  const auto &road_edges = model.getRoadEdges();
  const auto &edge_stds = model.getRoadEdgeStds();
  for (int i = 0; i < std::size(road_edge_vertices); i++) {
    road_edge_stds[i] = edge_stds[i];
    mapLineToPolygon(road_edges[i], 0.025, 0, &road_edge_vertices[i], max_idx);
  }

  // update path
  if (lead.getStatus()) {
    const float lead_d = lead.getDRel() * 2.;
    max_distance = std::clamp((float)(lead_d - fmin(lead_d * 0.35, 10.)), 0.0f, max_distance);
  }
  max_idx = get_path_length_idx(model_position, max_distance);
  mapLineToPolygon(model_position, 0.9, 1.22, &track_vertices, max_idx, false);
}

void ModelRenderer::drawLaneLines(QPainter &painter) {
  // lanelines
  for (int i = 0; i < std::size(lane_line_vertices); ++i) {
    painter.setBrush(QColor::fromRgbF(1.0, 1.0, 1.0, std::clamp<float>(lane_line_probs[i], 0.0, 0.7)));
    painter.drawPolygon(lane_line_vertices[i]);
  }

  // road edges
  for (int i = 0; i < std::size(road_edge_vertices); ++i) {
    painter.setBrush(QColor::fromRgbF(1.0, 0, 0, std::clamp<float>(1.0 - road_edge_stds[i], 0.0, 1.0)));
    painter.drawPolygon(road_edge_vertices[i]);
  }
}

void ModelRenderer::drawPath(QPainter &painter, const cereal::ModelDataV2::Reader &model, int height) {
  QLinearGradient bg(0, height, 0, 0);
  if (experimental_mode) {
    // The first half of track_vertices are the points for the right side of the path
    const auto &acceleration = model.getAcceleration().getX();
    const int max_len = std::min<int>(track_vertices.length() / 2, acceleration.size());

    for (int i = 0; i < max_len; ++i) {
      // Some points are out of frame
      int track_idx = max_len - i - 1;  // flip idx to start from bottom right
      if (track_vertices[track_idx].y() < 0 || track_vertices[track_idx].y() > height) continue;

      // Flip so 0 is bottom of frame
      float lin_grad_point = (height - track_vertices[track_idx].y()) / height;

      // speed up: 120, slow down: 0
      float path_hue = fmax(fmin(60 + acceleration[i] * 35, 120), 0);
      // FIXME: painter.drawPolygon can be slow if hue is not rounded
      path_hue = int(path_hue * 100 + 0.5) / 100;

      float saturation = fmin(fabs(acceleration[i] * 1.5), 1);
      float lightness = util::map_val(saturation, 0.0f, 1.0f, 0.95f, 0.62f);        // lighter when grey
      float alpha = util::map_val(lin_grad_point, 0.75f / 2.f, 0.75f, 0.4f, 0.0f);  // matches previous alpha fade
      bg.setColorAt(lin_grad_point, QColor::fromHslF(path_hue / 360., saturation, lightness, alpha));

      // Skip a point, unless next is last
      i += (i + 2) < max_len ? 1 : 0;
    }

  } else {
    updatePathGradient(bg);
  }

  painter.setBrush(bg);
  painter.drawPolygon(track_vertices);
}

void ModelRenderer::updatePathGradient(QLinearGradient &bg) {
  static const QColor throttle_colors[] = {
      QColor::fromHslF(148. / 360., 0.94, 0.51, 0.4),
      QColor::fromHslF(112. / 360., 1.0, 0.68, 0.35),
      QColor::fromHslF(112. / 360., 1.0, 0.68, 0.0)};

  static const QColor no_throttle_colors[] = {
      QColor::fromHslF(148. / 360., 0.0, 0.95, 0.4),
      QColor::fromHslF(112. / 360., 0.0, 0.95, 0.35),
      QColor::fromHslF(112. / 360., 0.0, 0.95, 0.0),
  };

  // Transition speed; 0.1 corresponds to 0.5 seconds at UI_FREQ
  constexpr float transition_speed = 0.1f;

  // Start transition if throttle state changes
  bool allow_throttle = (*uiState()->sm)["longitudinalPlan"].getLongitudinalPlan().getAllowThrottle() || !longitudinal_control;
  if (allow_throttle != prev_allow_throttle) {
    prev_allow_throttle = allow_throttle;
    // Invert blend factor for a smooth transition when the state changes mid-animation
    blend_factor = std::max(1.0f - blend_factor, 0.0f);
  }

  const QColor *begin_colors = allow_throttle ? no_throttle_colors : throttle_colors;
  const QColor *end_colors = allow_throttle ? throttle_colors : no_throttle_colors;
  if (blend_factor < 1.0f) {
    blend_factor = std::min(blend_factor + transition_speed, 1.0f);
  }

  // Set gradient colors by blending the start and end colors
  bg.setColorAt(0.0f, blendColors(begin_colors[0], end_colors[0], blend_factor));
  bg.setColorAt(0.5f, blendColors(begin_colors[1], end_colors[1], blend_factor));
  bg.setColorAt(1.0f, blendColors(begin_colors[2], end_colors[2], blend_factor));
}

QColor ModelRenderer::blendColors(const QColor &start, const QColor &end, float t) {
  if (t == 1.0f) return end;
  return QColor::fromRgbF(
      (1 - t) * start.redF() + t * end.redF(),
      (1 - t) * start.greenF() + t * end.greenF(),
      (1 - t) * start.blueF() + t * end.blueF(),
      (1 - t) * start.alphaF() + t * end.alphaF());
}

void ModelRenderer::drawLead(QPainter &painter, const cereal::RadarState::LeadData::Reader &lead_data,
                             const QPointF &vd, const QRect &surface_rect) {
  const float speedBuff = 10.;
  const float leadBuff = 40.;
  const float d_rel = lead_data.getDRel();
  const float v_rel = lead_data.getVRel();

  float fillAlpha = 0;
  if (d_rel < leadBuff) {
    fillAlpha = 255 * (1.0 - (d_rel / leadBuff));
    if (v_rel < 0) {
      fillAlpha += 255 * (-1 * (v_rel / speedBuff));
    }
    fillAlpha = (int)(fmin(fillAlpha, 255));
  }

  float sz = std::clamp((25 * 30) / (d_rel / 3 + 30), 15.0f, 30.0f) * 2.35;
  float x = std::clamp<float>(vd.x(), 0.f, surface_rect.width() - sz / 2);
  float y = std::min<float>(vd.y(), surface_rect.height() - sz * 0.6);

  float g_xo = sz / 5;
  float g_yo = sz / 10;

  // QPointF glow[] = {{x + (sz * 1.35) + g_xo, y + sz + g_yo}, {x, y - g_yo}, {x - (sz * 1.35) - g_xo, y + sz + g_yo}};
  float homebase_h = 12;
  QPointF glow[] = {{x + (sz * 1.35) + g_xo, y + sz + g_yo + homebase_h},{x + (sz * 1.35) + g_xo, y + sz + g_yo}, {x, y - g_yo}, {x - (sz * 1.35) - g_xo, y + sz + g_yo},{x - (sz * 1.35) - g_xo, y + sz + g_yo + homebase_h}, {x, y + sz + homebase_h + g_yo + 10}};
  painter.setBrush(QColor(218, 202, 37, 255));
  painter.drawPolygon(glow, std::size(glow));

  // chevron
  // QPointF chevron[] = {{x + (sz * 1.25), y + sz}, {x, y}, {x - (sz * 1.25), y + sz}};
  QPointF chevron[] = {{x + (sz * 1.25), y + sz + homebase_h},{x + (sz * 1.25), y + sz}, {x, y}, {x - (sz * 1.25), y + sz},{x - (sz * 1.25), y + sz + homebase_h}, {x, y + sz + homebase_h - 7}};
  painter.setBrush(QColor(201, 34, 49, fillAlpha));
  painter.drawPolygon(chevron, std::size(chevron));
}

// Projects a point in car to space to the corresponding point in full frame image space.
bool ModelRenderer::mapToScreen(float in_x, float in_y, float in_z, QPointF *out) {
  Eigen::Vector3f input(in_x, in_y, in_z);
  auto pt = car_space_transform * input;
  *out = QPointF(pt.x() / pt.z(), pt.y() / pt.z());
  return clip_region.contains(*out);
}

void ModelRenderer::mapLineToPolygon(const cereal::XYZTData::Reader &line, float y_off, float z_off,
                                     QPolygonF *pvd, int max_idx, bool allow_invert) {
  const auto line_x = line.getX(), line_y = line.getY(), line_z = line.getZ();
  QPointF left, right;
  pvd->clear();
  for (int i = 0; i <= max_idx; i++) {
    // highly negative x positions  are drawn above the frame and cause flickering, clip to zy plane of camera
    if (line_x[i] < 0) continue;

    bool l = mapToScreen(line_x[i], line_y[i] - y_off, line_z[i] + z_off, &left);
    bool r = mapToScreen(line_x[i], line_y[i] + y_off, line_z[i] + z_off, &right);
    if (l && r) {
      // For wider lines the drawn polygon will "invert" when going over a hill and cause artifacts
      if (!allow_invert && pvd->size() && left.y() > pvd->back().y()) {
        continue;
      }
      pvd->push_back(left);
      pvd->push_front(right);
    }
  }
}
