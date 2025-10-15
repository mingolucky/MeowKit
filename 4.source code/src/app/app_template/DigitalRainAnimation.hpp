/*
  DigitalRainAnimation.hpp - Library for Digital Rain Animation(MATRIX EFFECT).
  Created by Eric Nam, November 08, 2021.
  Released into the public domain.
*/
#pragma once
#include <Arduino.h>
#include <vector>
#include <string>

#define DEFAULT_FONT_SIZE 1
#define DEFAULT_LINE_WIDTH 12
#define DEFAULT_LETTER_HEIGHT 14
#define KEY_RESET_TIME 60000 // 60 seconds

#define ASCII_RANGE_1_START 33
#define ASCII_RANGE_1_END   65
#define ASCII_RANGE_2_START 91
#define ASCII_RANGE_2_END   126

enum AnimMode { SHOWCASE, MATRIX, TEXT };

template<class T>
class DigitalRainAnimation {
private:
  T* _gfx = nullptr;
  AnimMode _animMode = MATRIX;

  // Display dimensions
  int width = 0, height = 0;

  // Matrix settings
  int line_len_min = 3;
  int line_len_max = 20;
  int line_speed_min = 3;
  int line_speed_max = 15;
  int numOfline = 0;
  int matrixTimeFrame = 100;

  // Text settings
  int textTimeFrame = 400;
  int timeFrame = 100;
  int textStartX = 60, textStartY = 60;
  std::string textMessage;
  size_t textIndex = 0;

  // Font settings
  uint8_t fontSize = DEFAULT_FONT_SIZE;
  uint8_t lineWidth = DEFAULT_LINE_WIDTH;
  uint8_t letterHeight = DEFAULT_LETTER_HEIGHT;

  // Flags
  bool isPlaying = true;
  bool isAlphabetOnly = false;

  // Time tracking
  uint32_t lastDrawTime = 0;
  uint32_t lastUpdatedKeyTime = 0;

  // Colors
  uint16_t headCharColor;
  uint16_t textColor;
  uint16_t bgColor;

  // Data buffers
  std::vector<int> line_length;
  std::vector<int> line_pos;
  std::vector<int> line_speed;

  // Key animation
  std::string keyString;

  // Helper: generate random ASCII character
  String getASCIIChar() {
    return String((char)(random(0, 2) == 0 ? random(ASCII_RANGE_1_START, ASCII_RANGE_1_END) : random(ASCII_RANGE_2_START, ASCII_RANGE_2_END)));
  }

  String getAbcASCIIChar() {
    return String((char)(random(0, 2) == 0 ? random('A', 'Z' + 1) : random('a', 'z' + 1)));
  }

  int setYPos(int lineLen) {
    return lineLen * -20;
  }

  int getRandomNum(int min, int max) {
    return random(min, max + 1);
  }

  void resetKey() {
    keyString.clear();
    lastUpdatedKeyTime = millis();
  }

  std::string getKey(int key_length) {
    resetKey();
    int maxKeyLength = std::min(key_length > 0 ? key_length : numOfline, numOfline);
    for (int i = 0; i < maxKeyLength; ++i)
      keyString += getAbcASCIIChar().c_str();
    return keyString;
  }

  void prepareAnim() {
    if (!_gfx) return;

    setHeadCharColor(255, 255, 255);
    setTextColor(0, 255, 0);
    setBGColor(0, 0, 0);

    width = _gfx->width();
    height = _gfx->height();
    numOfline = (width + lineWidth - 1) / lineWidth;

    _gfx->fillRect(0, 0, width, height, bgColor);
    _gfx->setTextColor(textColor, bgColor);

    line_length.clear();
    line_pos.clear();
    line_speed.clear();

    for (int i = 0; i < numOfline; ++i) {
      line_length.push_back(getRandomNum(line_len_min, line_len_max));
      line_pos.push_back(setYPos(line_length[i]) - letterHeight);
      line_speed.push_back(getRandomNum(line_speed_min, line_speed_max));
    }

    lastDrawTime = millis() - timeFrame;
    lastUpdatedKeyTime = millis() - timeFrame;
    isPlaying = true;
  }

  void lineUpdate(int i) {
    line_length[i] = getRandomNum(line_len_min, line_len_max);
    line_pos[i] = setYPos(line_length[i]);
    line_speed[i] = getRandomNum(line_speed_min, line_speed_max);
  }

  void lineAnimation(int i) {
    int startX = i * lineWidth;
    int currentY = -letterHeight;
    bool isKeyMode = !keyString.empty();

    _gfx->fillRect(startX, 0, lineWidth, height, bgColor);
    _gfx->setTextSize(fontSize);

    for (int j = 0; j < line_length[i]; ++j) {
      int colorVal = ::map(j, 0, line_length[i], 10, 255);  // Use ::map to explicitly specify the global scope
      uint16_t lumColor = luminance(textColor, colorVal);
      _gfx->setTextColor(isKeyMode ? _gfx->color565(colorVal, 0, 0) : lumColor, bgColor);
      _gfx->setCursor(startX, line_pos[i] + currentY);
      _gfx->print(isAlphabetOnly ? getAbcASCIIChar() : getASCIIChar());
      currentY = j * letterHeight;
    }

    _gfx->setTextColor(headCharColor, bgColor);
    _gfx->setCursor(startX, line_pos[i] + currentY);
    _gfx->print(
      isKeyMode && keyString.length() > i ?
      String(keyString[i]) :
      (isAlphabetOnly ? getAbcASCIIChar() : getASCIIChar())
    );

    line_pos[i] += line_speed[i];
    if (line_pos[i] >= height) lineUpdate(i);
  }

  uint16_t luminance(uint16_t color, uint8_t lum) {
    uint16_t r = (color & 0xF800) >> 8;
    r |= (r >> 5);
    uint16_t g = (color & 0x07E0) >> 3;
    g |= (g >> 6);
    uint16_t b = (color & 0x001F) << 3;
    b |= (b >> 5);

    b = ((b * lum + 255) >> 8) & 0x00F8;
    g = ((g * lum + 255) >> 8) & 0x00FC;
    r = ((r * lum + 255) >> 8) & 0x00F8;

    return (r << 8) | (g << 3) | (b >> 3);
  }

  void textAnimation() {
    if (textIndex >= textMessage.length()) {
      _gfx->fillRect(0, 0, width, height, bgColor);
      textIndex = 0;

      if (_animMode == TEXT) {
        _gfx->setCursor(textStartX, textStartY);
      } else {
        _animMode = MATRIX;
        timeFrame = matrixTimeFrame;
      }
      return;
    }

    _gfx->setTextSize(fontSize);
    char letter = textMessage[textIndex++];
    if (letter == '\n') {
      _gfx->fillRect(0, 0, width, height, bgColor);
      _gfx->setCursor(textStartX, textStartY);
    } else {
      _gfx->print(letter);
    }
  }

public:
  DigitalRainAnimation() {}

  void init(T* gfx, bool biggerText = false, bool alphabetOnly = false) {
    _gfx = gfx;
    isAlphabetOnly = alphabetOnly;
    setBigText(biggerText);
    prepareAnim();
  }

  void setup(int len_min, int len_max, int spd_min, int spd_max, int frame) {
    line_len_min = len_min;
    line_len_max = len_max;
    line_speed_min = spd_min;
    line_speed_max = spd_max;
    matrixTimeFrame = frame;
    timeFrame = matrixTimeFrame;
    prepareAnim();
  }

  void setTextAnimMode(AnimMode mode, const std::string& msg, int startX = 60, int startY = 60, int frame = 400) {
    _animMode = mode;
    textMessage = msg;
    textIndex = 0;
    textStartX = startX;
    textStartY = startY;
    textTimeFrame = frame;
    timeFrame = frame;
  }

  void setAnimText(const std::string& msg) {
    textMessage = msg;
    textIndex = 0;
  }

  void setBigText(bool on) {
    fontSize = on ? DEFAULT_FONT_SIZE * 2 : DEFAULT_FONT_SIZE;
    lineWidth = on ? DEFAULT_LINE_WIDTH * 2 : DEFAULT_LINE_WIDTH;
    letterHeight = on ? DEFAULT_LETTER_HEIGHT * 1.6 : DEFAULT_LETTER_HEIGHT;
  }

  void setHeadCharColor(uint8_t r, uint8_t g, uint8_t b) {
    headCharColor = _gfx->color565(r, g, b);
  }

  void setTextColor(uint8_t r, uint8_t g, uint8_t b) {
    textColor = _gfx->color565(r, g, b);
  }

  void setBGColor(uint8_t r, uint8_t g, uint8_t b) {
    bgColor = _gfx->color565(r, g, b);
  }

  void pause() { isPlaying = false; }
  void resume() { isPlaying = true; }

  void loop() {
    if (!_gfx) return;

    uint32_t now = millis();
    if (now - lastUpdatedKeyTime > KEY_RESET_TIME)
      resetKey();

    if (now - lastDrawTime < timeFrame)
      return;

    if (!isPlaying) return;

    switch (_animMode) {
      case MATRIX:
        for (int i = 0; i < numOfline; ++i) lineAnimation(i);
        break;
      case TEXT:
      case SHOWCASE:
        textAnimation();
        break;
    }

    lastDrawTime = now;
  }
};