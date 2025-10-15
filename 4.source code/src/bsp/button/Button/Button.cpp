/*
	Button - a small library for Arduino to handle button debouncing
	
	MIT licensed.
*/

#include "Button.h"
#include <Arduino.h>

__Button::__Button(uint8_t pin, uint16_t debounce_ms)
:  _pin(pin)
,  _delay(debounce_ms)
,  _state(HIGH)
,  _ignore_until(0)
,  _has_changed(false)   // New member initialization
,  _dc_gap(400)
,  _longpress_ms(800)
,  _longpress_repeat_ms(0)
,  _down_millis(0)
,  _up_millis(0)
,  _long_fired(false)
,  _clicks(0)
,  _last_click_millis(0)
,  _last_long_repeat_millis(0)
,  _flag_single(false)
,  _flag_double(false)
,  _flag_long(false)
,  _cb_click(nullptr)
,  _cb_double(nullptr)
,  _cb_long_start(nullptr)
,  _cb_long_stop(nullptr)
,  _cb_long_during(nullptr)
{
    pinMode(pin, INPUT_PULLUP);
}

void __Button::begin()
{
	pinMode(_pin, INPUT_PULLUP);
}

// 
// public methods
// 

bool __Button::read()
{
	// ignore pin changes until after this delay time
	if (_ignore_until > millis())
	{
		// ignore any changes during this period
	}
    
	// pin has changed 
	else if (digitalRead(_pin) != _state)
	{
		_ignore_until = millis() + _delay;
		_state = !_state; // toggle to new state
		_has_changed = true;
		// Record timestamp at edge and reset long press state
		if (_state == PRESSED) {
			_down_millis = millis();
			_up_millis = 0;
			// When switching to pressed, reset the long press trigger flag and repeat timer
			_long_fired = false;
			_last_long_repeat_millis = 0;
		} else { // RELEASED
			_up_millis = millis();
			_down_millis = 0; // Cleared to zero when released to avoid inheriting old time
		}
	}
	return _state;
}

// has the button been toggled from on -> off, or vice versa
bool __Button::toggled()
{
	read();
	return has_changed();
}

// mostly internal, tells you if a button has changed after calling the read() function
bool __Button::has_changed()
{
	if (_has_changed)
	{
		_has_changed = false;
		return true;
	}
	return false;
}

// has the button gone from off -> on
bool __Button::pressed()
{
	return (read() == PRESSED && has_changed());
}

// has the button gone from on -> off
bool __Button::released()
{
	return (read() == RELEASED && has_changed());
}

// Checks whether a key is held down for a specified time
bool __Button::held(uint16_t hold_time)
{
	// Make sure to update the debounce status and edge timestamp first
	read();
	if (_state == PRESSED) {
		// _down_millis is set on the down edge of read(); if not set, it falls back to the current time
		uint32_t t = _down_millis ? _down_millis : millis();
		return (millis() - t) >= hold_time;
	}
	return false;
}

// ---- New: Advanced event implementation ----
void __Button::tick()
{
	// Update debounce and status edges
	bool was_pressed_edge = pressed();
	bool was_released_edge = released();

	// Long press processing
	if (_state == PRESSED) {
		// No long press has been triggered and the threshold has been reached
		if (!_long_fired && held(_longpress_ms)) {
			_long_fired = true;
			_flag_long = true;
			if (_cb_long_start) _cb_long_start();
			_last_long_repeat_millis = millis();
		}
		// Repeat callback during long press
		if (_long_fired && _longpress_repeat_ms > 0) {
			if (millis() - _last_long_repeat_millis >= _longpress_repeat_ms) {
				_last_long_repeat_millis = millis();
				if (_cb_long_during) _cb_long_during();
			}
		}
	}

	// Edge event: Press
	if (was_pressed_edge) {
		_down_millis = millis();
	}

	// Edge event: Lift
	if (was_released_edge) {
		_up_millis = millis();

		if (_long_fired) {
			// Long press to end
			_long_fired = false;
			if (_cb_long_stop) _cb_long_stop();
			// Long press does not count single/double clicks
			_clicks = 0;
			_last_click_millis = 0;
		} else {
			// Short press: accumulate clicks and determine single/double clicks
			if (_clicks == 0) {
				_clicks = 1;
				_last_click_millis = millis();
			} else {
				if (millis() - _last_click_millis <= _dc_gap) {
					// double click
					_flag_double = true;
					if (_cb_double) _cb_double();
					_clicks = 0;
					_last_click_millis = 0;
				} else {
					// Timeout, the previous click is considered a single click, and the new click restarts the timer.
					_flag_single = true;
					if (_cb_click) _cb_click();
					_clicks = 1;
					_last_click_millis = millis();
				}
			}
		}
	}

	// Click to confirm: If the first click times out and you do not wait for the second click
	if (_clicks == 1 && (millis() - _last_click_millis > _dc_gap)) {
		_flag_single = true;
		if (_cb_click) _cb_click();
		_clicks = 0;
		_last_click_millis = 0;
	}
}

void __Button::setDoubleClickTime(uint16_t gap_ms) { _dc_gap = gap_ms; }
void __Button::setLongPressTime(uint16_t long_ms) { _longpress_ms = long_ms; }
void __Button::setLongPressRepeat(uint16_t repeat_ms) { _longpress_repeat_ms = repeat_ms; }

bool __Button::isSingleClick() { bool f = _flag_single; _flag_single = false; return f; }
bool __Button::isDoubleClick() { bool f = _flag_double; _flag_double = false; return f; }
bool __Button::isLongPress()   { bool f = _flag_long;   _flag_long   = false; return f; }

void __Button::attachClick(void (*cb)()) { _cb_click = cb; }
void __Button::attachDoubleClick(void (*cb)()) { _cb_double = cb; }
void __Button::attachLongPressStart(void (*cb)()) { _cb_long_start = cb; }
void __Button::attachLongPressStop(void (*cb)()) { _cb_long_stop = cb; }
void __Button::attachDuringLongPress(void (*cb)()) { _cb_long_during = cb; }