#include "pc_monitor.h"
#include "pc_monitor_view.h"
extern "C" {
#include "asset/ui.h"
}

namespace MOONCAKE::APPS {
PCMonitor::PCMonitor(DEVICES*) { setAppInfo().name = "PCMonitor"; }
void PCMonitor::onOpen() {
    _parser.reset(); _hasData = false; _stale = false; _lastDataMs = 0;
    _previousScreen = lv_scr_act();
    ui_pc_monitor_init(); render(pc_monitor::Frame());
    ui_pc_monitor_status("", false);
}
void PCMonitor::onRunning() {
    bool updated = false;
    // Bound receive work so the launcher keeps polling long-press B.
    for (size_t n = 0; n < 512 && Serial.available(); ++n)
        updated = _parser.feed((char)Serial.read(), millis()) || updated;
    if (!Serial.available()) updated = _parser.idle(millis()) || updated;
    if (updated) {
        _lastDataMs = millis(); _hasData = true; _stale = false;
        render(_parser.frame()); ui_pc_monitor_status("Live  |  Hold B to exit", true);
    } else if (_hasData && !_stale && millis() - _lastDataMs >= 5000) {
        _stale = true; render(pc_monitor::Frame());
        ui_pc_monitor_status("Data stopped - check the PC client", false);
    }
    lv_timer_handler(); delay(1);
}
void PCMonitor::onClose() {
    // Launcher normally restores the menu before calling onClose.
    if (ui_PC_Monitor && lv_scr_act() == ui_PC_Monitor && _previousScreen &&
        lv_obj_is_valid(_previousScreen)) lv_scr_load(_previousScreen);
    ui_pc_monitor_destroy(); _previousScreen = nullptr;
    _parser.reset(); _hasData = false; _stale = false;
}
}
