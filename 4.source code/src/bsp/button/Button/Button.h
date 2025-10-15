/*
	Button - a small library for Arduino to handle button debouncing
	
	MIT licensed.
*/

#ifndef Button_h
#define Button_h
#include "Arduino.h"

class __Button
{
	public:
		__Button(uint8_t pin, uint16_t debounce_ms = 100);
		void begin();
		bool read();
		bool toggled();
		bool pressed();
		bool released();
		bool has_changed();
		bool held(uint16_t hold_time);   // Checks if a key is held down for more than hold_time milliseconds

		// ----New: Single click/double click/long press support (non-blocking)----
		void tick();                                    // Put it in loop() or timer and call it periodically (every 5-10ms is recommended)
		void setDoubleClickTime(uint16_t gap_ms);       // Double-click interval (default 400ms)
		void setLongPressTime(uint16_t long_ms);        // Long press judgment time (default 800ms)
		void setLongPressRepeat(uint16_t repeat_ms);    // Repeat callback period during long press (0 to disable)

		// Event query (return true once and reset)
		bool isSingleClick();
		bool isDoubleClick();
		bool isLongPress();                             // Long press start event

		// Optional: Register callback (if not set, only query via isXxx)
		void attachClick(void (*cb)());
		void attachDoubleClick(void (*cb)());
		void attachLongPressStart(void (*cb)());
		void attachLongPressStop(void (*cb)());
		void attachDuringLongPress(void (*cb)());
		
		const static bool PRESSED = LOW;
		const static bool RELEASED = HIGH;
	
	private:
		uint8_t  _pin;
		uint16_t _delay;
		bool     _state;
		uint32_t _ignore_until;
		bool     _has_changed;
		// ---- New: Timing and Status ----
		uint16_t _dc_gap;                    // Double-click maximum interval
		uint16_t _longpress_ms;              // Long press threshold
		uint16_t _longpress_repeat_ms;       // Long press repeat callback cycle (0 off)
		uint32_t _down_millis;               // Last pressed time
		uint32_t _up_millis;                 // Last lift time
		bool     _long_fired;                // Whether the long press start has been triggered
		uint8_t  _clicks;                    // Count: Single/Double Click
		uint32_t _last_click_millis;         // Last click-up time
		uint32_t _last_long_repeat_millis;   // The last long press repeat callback time

		// ---- New: One-time flag ----
		bool _flag_single;
		bool _flag_double;
		bool _flag_long;

		// ----New: Callback ----
		void (*_cb_click)();
		void (*_cb_double)();
		void (*_cb_long_start)();
		void (*_cb_long_stop)();
		void (*_cb_long_during)();
};

#endif