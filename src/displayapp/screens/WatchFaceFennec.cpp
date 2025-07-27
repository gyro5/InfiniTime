#include "displayapp/screens/WatchFaceFennec.h"

#include <lvgl/lvgl.h>
#include <cstdio>

#include "displayapp/screens/NotificationIcon.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/screens/WeatherSymbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
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
                                const Controllers::AlarmController& alarmController,
                                Controllers::NotificationManager& notificationManager,
                                Controllers::Settings& settingsController,
                                Controllers::HeartRateController& heartRateController,
                                Controllers::MotionController& motionController,
                                Controllers::SimpleWeatherService& weatherService)
  : currentDateTime {{}},
    dateTimeController {dateTimeController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController},
    weatherService {weatherService},
    statusIcons(batteryController, bleController, alarmController) {

  // The sand ground
  sand = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(sand, 240, 80);
  lv_obj_set_style_local_bg_color(sand, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
  lv_obj_set_style_local_radius(sand, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_align(sand, nullptr, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);

  // The sky
  sky = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(sky, 240, 160);
  lv_obj_set_style_local_bg_color(sky, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_CYAN);
  lv_obj_set_style_local_radius(sky, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_align(sky, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  // The sun/moon
  sun = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(sun, 50, 50);
  lv_obj_set_style_local_bg_color(sun, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_ORANGE);
  lv_obj_set_style_local_radius(sun, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_RADIUS_CIRCLE);
  lv_obj_align(sun, nullptr, LV_ALIGN_IN_TOP_LEFT, 145, 25);

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
    lv_obj_set_style_local_line_color(cactus[i], LV_LINE_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
    lv_obj_set_style_local_line_width(cactus[i], LV_LINE_PART_MAIN, LV_STATE_DEFAULT, cactusWidths[i]);
    lv_obj_set_style_local_line_rounded(cactus[i], LV_LINE_PART_MAIN, LV_STATE_DEFAULT, true);
  }

  // The bottom of the cactus. Drawn separately because it has square ends.
  static constexpr lv_point_t cactusBottomPoints[2] = {{190, 140}, {190, 180}};
  cactusBottom = lv_line_create(lv_scr_act(), nullptr);
  lv_line_set_points(cactusBottom, cactusBottomPoints, 2);
  lv_obj_set_style_local_line_color(cactusBottom, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
  lv_obj_set_style_local_line_width(cactusBottom, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, 20);
  lv_obj_set_style_local_line_rounded(cactusBottom, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, false);

  // Time label
  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_42);
  lv_obj_set_style_local_text_color(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_IN_TOP_LEFT, 10, 10);

  // AM/PM label
  label_time_ampm = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(label_time_ampm, "");
  lv_obj_set_style_local_text_color(label_time_ampm, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_obj_align(label_time_ampm, label_time, LV_ALIGN_OUT_RIGHT_BOTTOM, 5, 0);

  // Date label
  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_date, label_time, LV_ALIGN_OUT_BOTTOM_MID, -2, 6); // -2 for natural look
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);

  // Temperature label
  temperature = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(temperature, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_label_set_text(temperature, "");
  lv_obj_align(temperature, label_date, LV_ALIGN_OUT_BOTTOM_MID, 14, 3);
  // 14 to account for (half) of the weather icon (size 25 + 5 gap). not 15 bc to look more center naturally

  // Weather icon
  weatherIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(weatherIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_obj_set_style_local_text_font(weatherIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &fontawesome_weathericons);
  lv_label_set_text(weatherIcon, "");
  lv_obj_align(weatherIcon, temperature, LV_ALIGN_OUT_LEFT_MID, -5, 0);
  // lv_obj_set_auto_realign(weatherIcon, true); // Probably not necessary because already realign in Refresh()

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






  // Status icon widget: Battery, Charging, Alarm
  statusIcons.Create();

  // Notification icon
  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_LIME);
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  // Set a task that will periodically refresh the screen
  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);

  // Refresh the screen the first time
  Refresh();
}

// Destructor
WatchFaceFennec::~WatchFaceFennec() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

// Screen refresh callback
void WatchFaceFennec::Refresh() {
  statusIcons.Update();

  // Update notification status
  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  // Update date time
  currentDateTime = std::chrono::time_point_cast<std::chrono::minutes>(dateTimeController.CurrentDateTime());

  if (currentDateTime.IsUpdated()) {
    uint8_t hour = dateTimeController.Hours();
    uint8_t minute = dateTimeController.Minutes();

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
                            "%s %d/%d",
                            Pinetime::Controllers::DateTime::DayOfWeekShortToStringLow(dateTimeController.DayOfWeek()),
                            dateTimeController.Day(),
                            dateTimeController.Month());
      lv_obj_realign(label_date);
    }
  }

  // Update heartbeat
  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
      lv_label_set_text_fmt(heartbeatValue, "%d", heartbeat.Get());
    } else {
      lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
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
    } else {
      lv_label_set_text_static(temperature, "");
      lv_label_set_text(weatherIcon, "");
    }
    lv_obj_realign(temperature);
    lv_obj_realign(weatherIcon);
  }
}
