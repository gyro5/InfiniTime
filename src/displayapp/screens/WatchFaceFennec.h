#pragma once

#include <lvgl/src/lv_core/lv_obj.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "components/ble/SimpleWeatherService.h"
#include "components/ble/BleController.h"
#include "displayapp/widgets/StatusIcons.h"
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
                        const Controllers::AlarmController& alarmController,
                        Controllers::NotificationManager& notificationManager,
                        Controllers::Settings& settingsController,
                        Controllers::HeartRateController& heartRateController,
                        Controllers::MotionController& motionController,
                        Controllers::SimpleWeatherService& weather,
                        Controllers::FS& filesystem);

        // Destructor
        ~WatchFaceFennec() override;

        void Refresh() override;

      private:
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>> currentDateTime;
        Utility::DirtyValue<uint32_t> stepCount;
        Utility::DirtyValue<uint8_t> heartbeat;
        Utility::DirtyValue<bool> heartbeatRunning;
        Utility::DirtyValue<bool> notificationState;
        Utility::DirtyValue<std::optional<Pinetime::Controllers::SimpleWeatherService::CurrentWeather>> currentWeather;

        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::days>> currentDate;

        enum class Mode : std::uint8_t {Day, Night, Changing};
        Utility::DirtyValue<Mode> mode;
        void updateColor();

        // TODO re-arange these
        lv_obj_t* label_time;
        lv_obj_t* label_time_ampm;
        lv_obj_t* label_date;
        lv_obj_t* heartbeatIcon;
        lv_obj_t* heartbeatValue;
        lv_obj_t* stepIcon;
        lv_obj_t* stepValue;
        lv_obj_t* notificationIcon;
        lv_obj_t* weatherIcon;
        lv_obj_t* temperature;

        lv_obj_t* sand;
        lv_obj_t* sky;
        lv_obj_t* sunMoon;
        lv_obj_t* fennec;

        static constexpr int nCactusLines = 5;
        lv_obj_t* cactus[nCactusLines];
        lv_obj_t* cactusBottom;

        Controllers::DateTime& dateTimeController;
        Controllers::NotificationManager& notificationManager;
        Controllers::Settings& settingsController;
        Controllers::HeartRateController& heartRateController;
        Controllers::MotionController& motionController;
        Controllers::SimpleWeatherService& weatherService;

        lv_task_t* taskRefresh;
        Widgets::StatusIcons statusIcons;
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
                                            controllers.alarmController,
                                            controllers.notificationManager,
                                            controllers.settingsController,
                                            controllers.heartRateController,
                                            controllers.motionController,
                                            *controllers.weatherController,
                                            controllers.filesystem);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true; // Always available
      }
    };
  }
}
