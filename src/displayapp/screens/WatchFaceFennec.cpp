#include "displayapp/screens/WatchFaceFennec.h"

#include <lvgl/lvgl.h>
#include <cstdio>

#include "displayapp/screens/NotificationIcon.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/screens/WeatherSymbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "displayapp/screens/BleIcon.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/ble/SimpleWeatherService.h"
#include "components/settings/Settings.h"

using namespace Pinetime::Applications::Screens;

// Constructor
WatchFaceFennec::WatchFaceFennec(Controllers::DateTime& dateTimeController,
                                const Controllers::Battery& batteryController,
                                const Controllers::Ble& bleController,
                                Controllers::NotificationManager& notificationManager,
                                Controllers::Settings& settingsController,
                                Controllers::HeartRateController& heartRateController,
                                Controllers::MotionController& motionController,
                                Controllers::SimpleWeatherService& weatherService)
  : currentDateTime {{}},
    batteryIcon(false),
    dateTimeController {dateTimeController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController},
    weatherService {weatherService},
    batteryController {batteryController},
    bleController {bleController} {

  // The sky (with gradient)
  sky = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(sky, 240, 160);
  lv_obj_set_style_local_radius(sky, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_bg_grad_dir(sky, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
  lv_obj_align(sky, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  // The sun/moon (positioned based on mode -> See Refresh())
  sunMoon = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(sunMoon, 50, 50);
  lv_obj_set_style_local_radius(sunMoon, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_RADIUS_CIRCLE);

  // The sand ground
  sand = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(sand, 240, 80);
  lv_obj_set_style_local_radius(sand, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_align(sand, nullptr, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);

  // The cactus (drawn with lines)
  static constexpr lv_point_t cactusPoints[nCactusLines][2] = {
    { {190, 100}, {190, 140} },
    { {190, 150}, {160, 150} },
    { {160, 150}, {160, 120} },
    { {190, 130}, {220, 130} },
    { {220, 130}, {220, 110} },
  };
  static constexpr lv_style_int_t cactusWidths[nCactusLines] = {20, 10, 10, 10, 10};
  for (int i = 0; i < nCactusLines; i++) {
    cactus[i] = lv_line_create(lv_scr_act(), nullptr);
    lv_line_set_points(cactus[i], cactusPoints[i], 2);
    lv_obj_set_style_local_line_width(cactus[i], LV_LINE_PART_MAIN, LV_STATE_DEFAULT, cactusWidths[i]);
    lv_obj_set_style_local_line_rounded(cactus[i], LV_LINE_PART_MAIN, LV_STATE_DEFAULT, true);
  }

  // The bottom of the cactus. Drawn separately because it has square ends.
  static constexpr lv_point_t cactusBottomPoints[2] = {{190, 140}, {190, 180}};
  cactusBottom = lv_line_create(lv_scr_act(), nullptr);
  lv_line_set_points(cactusBottom, cactusBottomPoints, 2);
  lv_obj_set_style_local_line_width(cactusBottom, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, 20);
  lv_obj_set_style_local_line_rounded(cactusBottom, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, false);

  // The fennec
  fennec = lv_img_create(lv_scr_act(), nullptr);

  // Date label
  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_date, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  // Time label
  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_42);
  lv_obj_align(label_time, nullptr, LV_ALIGN_IN_TOP_MID, 0, 35);

  // AM/PM label
  label_time_ampm = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(label_time_ampm, "");
  lv_obj_align(label_time_ampm, label_time, LV_ALIGN_OUT_RIGHT_BOTTOM, 5, 0);

  // Temperature label
  temperature = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(temperature, "");
  lv_obj_align(temperature, label_time, LV_ALIGN_OUT_BOTTOM_MID, 14, 12);
  // 14 to account for (half) of the weather icon (size 25 + 5 gap). not 15 bc to look more center naturally

  // Weather icon
  weatherIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(weatherIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &fontawesome_weathericons);
  lv_label_set_text_static(weatherIcon, "");
  lv_obj_align(weatherIcon, temperature, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  // Heartbeat icon and label
  heartbeatIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(heartbeatIcon, Symbols::heartBeat);
  lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_obj_align(heartbeatIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_label_set_text_static(heartbeatValue, "");
  lv_obj_align(heartbeatValue, heartbeatIcon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  // Step icon and label
  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_label_set_text_static(stepValue, "0");
  lv_obj_align(stepValue, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);

  stepIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_label_set_text_static(stepIcon, Symbols::shoe);
  lv_obj_align(stepIcon, stepValue, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  // Top right icons: Notification, Battery, BLE
  label_battery_value = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_battery_value, nullptr, LV_ALIGN_IN_TOP_RIGHT, 0, 0);
  lv_label_set_text_static(label_battery_value, "");

  batteryIcon.Create(lv_scr_act());
  lv_obj_align(batteryIcon.GetObject(), label_battery_value, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  bleIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(bleIcon, Symbols::bluetooth);
  lv_obj_align(bleIcon, batteryIcon.GetObject(), LV_ALIGN_OUT_LEFT_MID, -5, 0);

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, bleIcon, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  // Set a task that will periodically refresh the screen
  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);

  // Default mode
  mode = WatchFaceFennec::Mode::Day;

  // Refresh the screen the first time
  Refresh();
}

// Destructor
WatchFaceFennec::~WatchFaceFennec() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceFennec::updateByMode() {
  int modeIdx = static_cast<int>(mode.Get());

  // Sky
  lv_obj_set_style_local_bg_color(sky, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, colors[0][modeIdx]);
  lv_obj_set_style_local_bg_grad_color(sky, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, colors[1][modeIdx]);

  // Sand
  lv_obj_set_style_local_bg_color(sand, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, colors[2][modeIdx]);

  // Cactus
  for (auto& cactusLine : cactus) {
    lv_obj_set_style_local_line_color(cactusLine, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, colors[3][modeIdx]);
  }
  lv_obj_set_style_local_line_color(cactusBottom, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, colors[3][modeIdx]);

  // Sun moon
  lv_obj_set_style_local_bg_color(sunMoon, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, colors[4][modeIdx]);
  lv_obj_set_size(sunMoon, sunMoonSize[modeIdx], sunMoonSize[modeIdx]);
  lv_obj_align(sunMoon, nullptr, LV_ALIGN_IN_TOP_LEFT, 150, sunMoonY[modeIdx]);

  // Fennec
  lv_img_set_src(fennec, fennecSrc[modeIdx]);
  lv_obj_align(fennec, nullptr, LV_ALIGN_IN_TOP_LEFT, fennecPos[modeIdx][0], fennecPos[modeIdx][1]);

  // Labels
  lv_obj_set_style_local_text_color(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, colors[5][modeIdx]);
  lv_obj_set_style_local_text_color(label_time_ampm, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, colors[5][modeIdx]);
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, colors[5][modeIdx]);
  lv_obj_set_style_local_text_color(temperature, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, colors[5][modeIdx]);
  lv_obj_set_style_local_text_color(weatherIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, colors[5][modeIdx]);

  // Top right icons
  lv_obj_set_style_local_text_color(label_battery_value, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, colors[5][modeIdx]);
  batteryIcon.SetColor(colors[5][modeIdx]);
  lv_obj_set_style_local_text_color(bleIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, colors[5][modeIdx]);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, colors[5][modeIdx]);
}

// Screen refresh callback
void WatchFaceFennec::Refresh() {
  // Update notification status
  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  // Update date time (and colors based on hour)
  currentDateTime = std::chrono::time_point_cast<std::chrono::minutes>(dateTimeController.CurrentDateTime());

  if (currentDateTime.IsUpdated()) {
    uint8_t hour = dateTimeController.Hours();
    uint8_t minute = dateTimeController.Minutes();

    // Change mode based on hour
    if (hour >= 20 || hour <= 4) { // Night: 20->4
      mode = WatchFaceFennec::Mode::Night;
    } else if (7 <= hour && hour <= 17) { // Day: 7->17
      mode = WatchFaceFennec::Mode::Day;
    } else { // Sun rise: 5->6 or Sun set: 18->19
      mode = WatchFaceFennec::Mode::Changing;
    }
    if (mode.IsUpdated()) {
      updateByMode();
    }

    if (settingsController.GetClockType() == Controllers::Settings::ClockType::H12) {
      // 12-Hour mode
      char ampmChar[3] = "AM";
      if (hour == 0) {
        hour = 12;
      } else if (hour == 12) {
        ampmChar[0] = 'P';
      } else if (hour > 12) {
        hour = hour - 12;
        ampmChar[0] = 'P';
      }
      lv_label_set_text(label_time_ampm, ampmChar);
    }
    lv_label_set_text_fmt(label_time, "%02d:%02d", hour, minute);
    lv_obj_realign(label_time);
    lv_obj_realign(label_time_ampm);

    currentDate = std::chrono::time_point_cast<std::chrono::days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      lv_label_set_text_fmt(label_date,
                            "%s %02d/%02d",
                            Pinetime::Controllers::DateTime::DayOfWeekShortToStringLow(dateTimeController.DayOfWeek()),
                            dateTimeController.Day(),
                            dateTimeController.Month());
      lv_obj_realign(label_date);
    }
  }

  // Update weather
  currentWeather = weatherService.Current();
  if (currentWeather.IsUpdated()) {
    auto optCurrentWeather = currentWeather.Get();
    if (optCurrentWeather) {
      int16_t temp = optCurrentWeather->temperature.Celsius();
      char tempUnit = 'C';
      if (settingsController.GetWeatherFormat() == Controllers::Settings::WeatherFormat::Imperial) {
        temp = optCurrentWeather->temperature.Fahrenheit();
        tempUnit = 'F';
      }
      lv_label_set_text_fmt(temperature, "%d°%c", temp, tempUnit);
      lv_label_set_text(weatherIcon, Symbols::GetSymbol(optCurrentWeather->iconId));
      lv_obj_align(label_time, nullptr, LV_ALIGN_IN_TOP_MID, 0, 35);
    } else {
      lv_label_set_text_static(temperature, "");
      lv_label_set_text(weatherIcon, "");

      // No weather -> Move time a little bit down to center
      lv_obj_align(label_time, nullptr, LV_ALIGN_IN_TOP_MID, 0, 50);
    }
    lv_obj_realign(temperature);
    lv_obj_realign(weatherIcon);
  }

  // Update heartbeat
  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
      lv_label_set_text_fmt(heartbeatValue, "%d", heartbeat.Get());
    } else {
      lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
      lv_label_set_text_static(heartbeatValue, "");
    }

    lv_obj_realign(heartbeatIcon);
    lv_obj_realign(heartbeatValue);
  }

  // Update step count
  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "%lu", stepCount.Get());
    lv_obj_realign(stepValue);
    lv_obj_realign(stepIcon);
  }

  // Update top right icons
  powerPresent = batteryController.IsPowerPresent();
  if (powerPresent.IsUpdated()) {
    if (powerPresent.Get()) { // Charging
      batteryIcon.SetColor(colors[6][static_cast<int>(mode.Get())]);
    }
    else { // Else use text color
      batteryIcon.SetColor(colors[5][static_cast<int>(mode.Get())]);
    }
  }

  batteryPercentRemaining = batteryController.PercentRemaining();
  if (batteryPercentRemaining.IsUpdated()) {
    auto batteryPercent = batteryPercentRemaining.Get();
    batteryIcon.SetBatteryPercentage(batteryPercent);
    lv_label_set_text_fmt(label_battery_value, "%d%%", batteryPercent);
  }

  bleState = bleController.IsConnected();
  bleRadioEnabled = bleController.IsRadioEnabled();
  if (bleState.IsUpdated() || bleRadioEnabled.IsUpdated()) {
    lv_label_set_text_static(bleIcon, BleIcon::GetIcon(bleState.Get()));
  }

  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  lv_obj_realign(label_battery_value);
  lv_obj_realign(batteryIcon.GetObject());
  lv_obj_realign(bleIcon);
  lv_obj_realign(notificationIcon);
}

// Available if the 2 fennec images are available
bool WatchFaceFennec::IsAvailable(Pinetime::Controllers::FS& filesystem) {
  lfs_file file = {};

  if (filesystem.FileOpen(&file, "/images/fennec_sit.bin", LFS_O_RDONLY) < 0) {
    return false;
  }

  filesystem.FileClose(&file);
  if (filesystem.FileOpen(&file, "/images/fennec_sleep.bin", LFS_O_RDONLY) < 0) {
    return false;
  }

  filesystem.FileClose(&file);
  return true;
}