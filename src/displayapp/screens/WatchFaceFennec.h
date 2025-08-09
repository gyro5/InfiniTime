#pragma once

#include <lvgl/src/lv_core/lv_obj.h>
#include <chrono>
#include <cstdint>
// #include <memory>
#include "displayapp/screens/Screen.h"
#include <displayapp/Controllers.h>
#include "components/datetime/DateTimeController.h"
#include "components/ble/SimpleWeatherService.h"
#include "components/ble/BleController.h"
#include "displayapp/screens/BatteryIcon.h"
#include "utility/DirtyValue.h"
#include "displayapp/apps/Apps.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
    class Battery;
    class Ble;
    class AlarmController;
    class NotificationManager;
    class HeartRateController;
    class MotionController;
  }

  namespace Applications {
    namespace Screens {

      class WatchFaceFennec : public Screen {
      public:
        // Constructor
        WatchFaceFennec(Controllers::DateTime& dateTimeController,
                        const Controllers::Battery& batteryController,
                        const Controllers::Ble& bleController,
                        Controllers::NotificationManager& notificationManager,
                        Controllers::Settings& settingsController,
                        Controllers::HeartRateController& heartRateController,
                        Controllers::MotionController& motionController,
                        Controllers::SimpleWeatherService& weather);

        // Destructor
        ~WatchFaceFennec() override;

        void Refresh() override;

        static bool IsAvailable(Pinetime::Controllers::FS& filesystem);

      private:
        // Colors for 3 modes {Day, Night, Changing}
        static constexpr lv_color_t colors[8][3] {
          {LV_COLOR_MAKE(0x8a, 0xd4, 0xe1), LV_COLOR_MAKE(0x0A, 0x1D, 0x47), LV_COLOR_MAKE(0x53, 0x38, 0x9F)}, // Sky gradient 1
          {LV_COLOR_MAKE(0xf8, 0xfd, 0xfd), LV_COLOR_MAKE(0x16, 0x5A, 0x91), LV_COLOR_MAKE(0xFF, 0x96, 0x3B)}, // Sky gradient 2 (bottom)
          {LV_COLOR_MAKE(0xfe, 0xea, 0x86), LV_COLOR_MAKE(0xb0, 0x9d, 0x90), LV_COLOR_MAKE(0xb4, 0x4d, 0x13)}, // Sand
          {LV_COLOR_MAKE(0x56, 0x7c, 0x17), LV_COLOR_MAKE(0x4d, 0x56, 0x44), LV_COLOR_MAKE(0x3e, 0x59, 0x10)}, // Cactus
          {LV_COLOR_MAKE(0xff, 0xfc, 0x9d), LV_COLOR_MAKE(0xad, 0xb2, 0xbf), LV_COLOR_MAKE(0xf8, 0xe2, 0x38)}, // Sun moon
          {LV_COLOR_MAKE(0x05, 0x0b, 0x21), LV_COLOR_MAKE(0xea, 0xea, 0xea), LV_COLOR_MAKE(0xea, 0xea, 0xea)}, // Text
          {LV_COLOR_MAKE(0x02, 0x76, 0x5d), LV_COLOR_MAKE(0x04, 0xef, 0xbc), LV_COLOR_MAKE(0x04, 0xef, 0xbc)}, // Charging battery
        };

        // Size and y-offset for sun moon
        static constexpr lv_coord_t sunMoonSize[3] {50, 40, 50};
        static constexpr lv_coord_t sunMoonY[3] {25, 30, 130};

        // Image source and position of fennec
        static constexpr char fennecSit[] = "F:/images/fennec_sit.bin";
        static constexpr char fennecSleep[] = "F:/images/fennec_sleep.bin";
        static constexpr const char* fennecSrc[3] {fennecSit, fennecSleep, fennecSit};
        static constexpr lv_coord_t fennecPos[3][2] = {{30, 115}, {20, 125}, {30, 115}};

        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>> currentDateTime;
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::days>> currentDate;

        Utility::DirtyValue<uint32_t> stepCount;
        Utility::DirtyValue<uint8_t> heartbeat;
        Utility::DirtyValue<bool> heartbeatRunning;
        Utility::DirtyValue<std::optional<Pinetime::Controllers::SimpleWeatherService::CurrentWeather>> currentWeather;

        Utility::DirtyValue<bool> notificationState;
        Utility::DirtyValue<uint8_t> batteryPercentRemaining;
        Utility::DirtyValue<bool> powerPresent;
        Utility::DirtyValue<bool> bleState;
        Utility::DirtyValue<bool> bleRadioEnabled;

        // For mode-based colors and decorations
        enum class Mode : std::uint8_t {Day, Night, Changing};
        Utility::DirtyValue<Mode> mode;
        void updateByMode();

        // Background and decorations
        lv_obj_t* sand;
        lv_obj_t* sky;
        lv_obj_t* sunMoon;
        lv_obj_t* fennec;
        static constexpr int nCactusLines = 5;
        lv_obj_t* cactus[nCactusLines];
        lv_obj_t* cactusBottom;

        // Date time labels
        lv_obj_t* label_time;
        lv_obj_t* label_time_ampm;
        lv_obj_t* label_date;

        // Sensor icons
        lv_obj_t* heartbeatIcon;
        lv_obj_t* heartbeatValue;
        lv_obj_t* stepIcon;
        lv_obj_t* stepValue;
        lv_obj_t* weatherIcon;
        lv_obj_t* temperature;

        // Top right icons
        lv_obj_t* notificationIcon;
        lv_obj_t* bleIcon;
        lv_obj_t* label_battery_value;
        BatteryIcon batteryIcon;

        Controllers::DateTime& dateTimeController;
        Controllers::NotificationManager& notificationManager;
        Controllers::Settings& settingsController;
        Controllers::HeartRateController& heartRateController;
        Controllers::MotionController& motionController;
        Controllers::SimpleWeatherService& weatherService;
        const Controllers::Battery& batteryController;
        const Controllers::Ble& bleController;

        lv_task_t* taskRefresh;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Fennec> {
      static constexpr WatchFace watchFace = WatchFace::Fennec;
      static constexpr const char* name = "Fennec Face";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceFennec(controllers.dateTimeController,
                                            controllers.batteryController,
                                            controllers.bleController,
                                            controllers.notificationManager,
                                            controllers.settingsController,
                                            controllers.heartRateController,
                                            controllers.motionController,
                                            *controllers.weatherController);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& filesystem) {
        return Screens::WatchFaceFennec::IsAvailable(filesystem);
      }
    };
  }
}
