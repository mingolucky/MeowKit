#include "launcher.h"
#include <core/lv_obj.h>
#include "bsp/porting/lv_port_disp.h"
#include "bsp/porting/lv_port_indev.h"
#include "ui/src/ui.h"
#include "app/app.h"
#include <memory>
#include "deviceStateManager.h"
#include "timeSetting.h"
#include "luaEngine/luascriptManager.h"

namespace MOONCAKE
{
    namespace APPS
    {
        static bool system_time_set = false;

        void Launcher::updateAPPManager()
        {
            if (selected_app_index >= 0 && selected_app_index < 11)
            {
                // Disable LVGL
                _lvgl_enable = false;

                /* Delete Launcher screen and reaplace with an empty one */
                lv_disp_load_scr(lv_obj_create(NULL));
                if (selected_app_index >= 0 && selected_app_index < 5)
                {
                    lv_obj_del(ui_Apps_Menu_One);
                }
                else if (selected_app_index >= 5 && selected_app_index < 11)
                {
                    lv_obj_del(ui_Apps_Menu_Two);
                }
                else if (selected_app_index >= 11 && selected_app_index < 17)
                {
                    lv_obj_del(ui_Apps_Menu_Three);
                }

                /* Open app */
                _mooncake->openApp(selected_app_index);
                _mooncake->update();

                // Wait until app is running
                if (_mooncake->getAppCurrentState(selected_app_index) == AppAbility::State_t::StateRunning)
                {
                    while (1)
                    {
                        // run app loop
                        _mooncake->update();
                        vTaskDelay(5);

                        // Tick button state machine and check long-press-start to close the app
                        _device->button.B.tick();
                        if (_device->button.B.isLongPress())
                        {
                            // Close the app
                            _mooncake->closeApp(selected_app_index);
                            _mooncake->update();

                            // Re-initialize UI
                            reInitUI(selected_app_index);

                            // Break the loop to return to the launcher
                            break;
                        }
                    }
                }
            }
            else if (selected_app_index >= 11)
            {
                // Disable LVGL
                _lvgl_enable = false;

                /* Delete Launcher screen and reaplace with an empty one */
                lv_disp_load_scr(lv_obj_create(NULL));
                lv_obj_del(ui_Apps_Menu_Three);

                std::string appId = std::to_string(selected_app_index);
                _luaEngine->runScriptFromSD(appId.c_str());
                while (1)
                {
                    vTaskDelay(5);
                    _device->button.B.tick();
                    if (_device->button.B.isLongPress())
                    {
                        reInitUI(selected_app_index);
           
                        break;
                    }
                }
                
            }
            
        }

        void Launcher::updateDeviceStatus()
        {
            // [Disabled] CTP Touch to wake up the screen: temporarily block touch lighting and reset the active timer
            // if (_device->ctp.isTouched())
            // {
            //     /* Reset auto screen off time counting */
            //     lv_disp_trig_activity(NULL);
            //     _device->Lcd.setBrightness(_device_status.brightness);
            //     _device_status.updated = true;
            // }

            /* Device status manage */
            if (_device_status.updated)
            {
                _device_status.updated = false;
                /* Update brightness */
                _device->Lcd.setBrightness(_device_status.brightness);
                // Update volume
                //_device->audio.setVolume(_device_status.volume);
            }

            // [Disabled] Automatic screen off: Temporarily block automatic sleep based on inactivity time
            // if (_device_status.autoScreenOff && (lv_disp_get_inactive_time(NULL) > _device_status.autoScreenOffTime))
            // {
            //     // Set brightness to minimum
            //     _device->Lcd.setBrightness(255);
            //     sleep_mode();
            //
            //     /* Reset auto screen off time counting */
            //     lv_disp_trig_activity(NULL);
            // }

            if (_device_status.powerDetection == false)
            {
                // Throttling + Smoothing: Update at most once every 30 seconds; update the value only when the difference is >= 2, to avoid jumping back and forth between 78/79
                static unsigned long last_bat_check = 0; 
                static int last_shown_percent = -1;      
                static float filtered_percent = -1.0f;   

                unsigned long now = millis();
                int cur_percent = _device->getBatteryPercent();

                if (filtered_percent < 0.0f)
                {
                    filtered_percent = (float)cur_percent;
                }
                filtered_percent = filtered_percent * 0.7f + (float)cur_percent * 0.3f;
                int display_percent = (int)(filtered_percent + 0.5f); // rounding

                bool time_elapsed = (unsigned long)(now - last_bat_check) >= 30000UL; // At least 30 seconds
                bool big_change = (last_shown_percent < 0) || (abs(display_percent - last_shown_percent) >= 2);

                if (time_elapsed || big_change)
                {
                    last_bat_check = now;
                    last_shown_percent = display_percent;

                    // Display format changed to "X/100"
                    std::string percent = std::to_string(display_percent) + "/100";
                    lv_label_set_text(ui_bat_number, percent.c_str());
                    // lv_obj_set_style_text_font(ui_bat_number, &ui_font_apps_name, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_invalidate(ui_bat_number); // Refresh battery percentage display
                }
            }

            // Throttle updates to once per second
            static unsigned long last_clock_update = 0;
            unsigned long now_ms = millis();
            if ((unsigned long)(now_ms - last_clock_update) < 1000UL)
            {
                return;
            }
            last_clock_update = now_ms;
            // Detect SD card status
            if (_device_status.sdCardInserted == false)
            {
                // bool sd_available =  _device->sd.isInited();
                bool sd_available = _device->sd.checkCardPresent();
                // Check if there is an SD card
                if (ui_home_sd_off != NULL)
                {
                    if (sd_available)
                    {
                        lv_img_set_src(ui_home_sd_off, &ui_img_ui_home_sd_on_png);
                        // std::cout << "SD卡存在" << std::endl;
                    }
                    else
                    {
                        lv_img_set_src(ui_home_sd_off, &ui_img_ui_home_sd_off_png);
                        // std::cout << "SD卡不存在" << std::endl;
                    }
                    lv_obj_invalidate(ui_home_sd_off);
                }
            }

            // Update the clock
            ui_clock_update();

            // Determine whether the current interface is the clock interface
            lv_obj_t *current_screen = lv_disp_get_scr_act(NULL);
            if (current_screen == ui_Clock)
            {
                // If it is a clock interface, make sure the clock update function is called
                // ui_clock_update();

                // std::cout<<"Switched to clock page"<<std::endl;
                if (system_time_set == false)
                {
                    // std::cout<<"Setting time"<<std::endl;
                    timeSetting time_setter;
                    if (time_setter.initTime())
                    {
                        system_time_set = true;
                        // std::cout<<"Time set"<<std::endl;
                    }
                }
            }
        }

        void Launcher::onCreate()
        {
            /* Init lvgl */
            lv_deinit();
            lv_init();

            // Initialize devices
            lv_port_disp_init(&_device->Lcd);
            lv_port_indev_init(&_device->ctp);

            /* Init ui */
            ui_init();

            lv_disp_trig_activity(NULL);
            _mooncake = std::make_unique<mooncake::Mooncake>();
            MeowKit_app_install_callback(_mooncake.get(), _device);

            luaScriptManager& manager = luaScriptManager::getInstance();
            // set device
            manager.setDevice(_device);
            // load all scripts
            manager.loadScriptsFromSD();
            _luaEngine = std::make_unique<luaEngine>(std::shared_ptr<DEVICES>(_device));

            _lvgl_enable = true;
        }

        void Launcher::reInitUI(int pageIndex)
        {
            /* Re-init lvgl */
            lv_deinit();
            delay(100);
            lv_init();
            lv_port_disp_init(&_device->Lcd);
            lv_port_indev_init(&_device->ctp);

            /* Init ui */
            ui_init();
            if (pageIndex >= 0 && pageIndex < 5)
            {
                lv_disp_load_scr(ui_Apps_Menu_One);
            }
            else if (pageIndex >= 5 && pageIndex < 11)
            {
                lv_disp_load_scr(ui_Apps_Menu_Two);
            }
            else if (pageIndex >= 11 && pageIndex < 17)
            {
                lv_disp_load_scr(ui_Apps_Menu_Three);
            }

            selected_app_index = -1;
            _lvgl_enable = true;
            lv_disp_trig_activity(NULL);
        }

        void Launcher::onLoop()
        {
            /* Update device status */
            updateDeviceStatus();
            /* Update app manager */
            updateAPPManager();
            if (_lvgl_enable)
            {
                /* Handle LVGL tasks */
                lv_timer_handler();
                delay(5);
            }
        }

        void Launcher::onDestroy()
        {
        }

        // Sleep mode
        void Launcher::sleep_mode()
        {
            /* Reset button B interrupt pin (Disable touch interrupt wakeup) */
            gpio_reset_pin(GPIO_NUM_0);
            gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
            gpio_wakeup_enable(GPIO_NUM_0, GPIO_INTR_HIGH_LEVEL);
            // [Disabled] Touchpad interrupt as wakeup source
            // gpio_reset_pin(GPIO_NUM_21);
            // gpio_set_direction(GPIO_NUM_21, GPIO_MODE_INPUT);
            // gpio_wakeup_enable(GPIO_NUM_21, GPIO_INTR_HIGH_LEVEL);
            esp_sleep_enable_gpio_wakeup();

            /* Hold untill button released */
            while (gpio_get_level(GPIO_NUM_0) == 0)
            {
                delay(20);
            }

            /* Go to sleep */
            esp_light_sleep_start();
        }

        // Update clock display
        void Launcher::ui_clock_update()
        {
            RTC_Time now;
            if (_device->pcf.getTime(now))
            {
                char buf[9];
                snprintf(buf, sizeof(buf), "%02d:%02d:%02d", now.hour, now.min, now.sec);
                // Only update the text content without changing the layout - to prevent jumping
                for (int i = 0; i < 8; ++i)
                {
                    if (ui_time_digit[i])
                    {
                        lv_label_set_text_fmt(ui_time_digit[i], "%c", buf[i]);
                    }
                }
                // Refresh date and day of the week
                lv_label_set_text_fmt(ui_data, "%02d-%02d-%04d", now.day, now.month, now.year);
                lv_label_set_text(ui_week, week_str[now.weekday % 7]);
                // std::cout<<"RTC_Time get success"<<std::endl;
            }
            else
            {
                // std::cout<<"RTC_Time get error"<<std::endl;
            }
        }
    }
}
