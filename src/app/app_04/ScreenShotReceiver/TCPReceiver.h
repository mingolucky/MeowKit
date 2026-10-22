#ifndef _TCPRECEIVER_H_
#define _TCPRECEIVER_H_

#pragma GCC optimize ("O3")

#ifndef LOVYANGFX_HPP_
#define LGFX_USE_V1
#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>  // https://github.com/lovyan03/LovyanGFX/
#endif

#include <WiFi.h>
#include <WiFiServer.h>
#include <esp_heap_caps.h>
#include "tjpgdClass.h"

class TCPReceiver
{
public:
  TCPReceiver() = default;
  TCPReceiver(const TCPReceiver&) = delete;
  TCPReceiver& operator=(const TCPReceiver&) = delete;
  ~TCPReceiver() { stop(); }

  void stop()
  {
    _client.stop();
    _tcp.end();
    if (_lcd && _ready) { _lcd->waitDMA(); }
    for (auto& buf : _dmabufs) {
      if (buf) heap_caps_free(buf);
      buf = nullptr;
    }
    _dmabuf = nullptr;
    _ready = false;
  }

  bool setup(LovyanGFX* Lcd)
  {
    if (_ready) return true;
    if (!Lcd) return false;
    _lcd = Lcd;
    Serial.println("TCPReceiver setup.");

    _lcd_width = Lcd->width();
    _lcd_height = Lcd->height();
    _bytesize = Lcd->getColorConverter()->bytes;
    Serial.printf("display byte per pixel:%d\n", _bytesize);

    Serial.printf("color depth: %02x \n", Lcd->getColorDepth());

    switch (Lcd->getColorDepth())
    {
    default:
    case lgfx::color_depth_t::rgb565_2Byte:
      _fp_write = write565;
      break;
    case lgfx::color_depth_t::rgb888_3Byte:
      _fp_write = write888;
      break;
    case lgfx::color_depth_t::rgb332_1Byte:
      _fp_write = write332;
      break;
    case lgfx::color_depth_t::grayscale_8bit:
      _fp_write = writeGray;
      break;
    }
    for (int i = 0; i < 2; ++i) {
      _dmabufs[i] = (uint8_t*)heap_caps_malloc(_lcd_width * (_rowskip + 1) * 16 * _bytesize, MALLOC_CAP_DMA);
      if (!_dmabufs[i]) {
        Serial.println("[RetroTV] DMA allocation failed");
        stop();
        return false;
      }
    }

    _dmabuf = _dmabufs[0];

    _tcp.setNoDelay(true);
    _tcp.begin(63333);

    // Decode and draw on the app task. The upstream output task shares global
    // state and can outlive a frame (or a deleted receiver).
    _ready = true;

    Serial.println("TCPReceiver setup done.");
    return true;
  }

  bool loop()
  {
    if (!_ready) return false;
    if (!_client.connected() && !_client.available()) {
      resetClient();
      _client = _tcp.available();
      if (!_client) { delay(1); return true; }
      _client.setNoDelay(true);
      Serial.println("[RetroTV] sender connected");
    }
    if (!_recv_requested) {
      if (_client.print("JPG\n") != 4) { resetClient(); return true; }
      _recv_requested = true;
      _requestAt = millis();
    }
    // TCP headers may arrive in fragments: never discard a partial header.
    if (_client.available() < SENDER_PREFIX_SIZE) {
      if (millis() - _requestAt > FRAME_TIMEOUT_MS) resetClient();
      delay(1);
      return true;
    }
    _frameAt = millis();
    if (!readExact(_tcpBuf, SENDER_PREFIX_SIZE)) { resetClient(); return true; }
    const uint32_t size = uint32_t(_tcpBuf[3]) | (uint32_t(_tcpBuf[4]) << 8)
                        | (uint32_t(_tcpBuf[5]) << 16) | (uint32_t(_tcpBuf[6]) << 24);
    if (_tcpBuf[0] != 'J' || _tcpBuf[1] != 'P' || _tcpBuf[2] != 'G'
        || size == 0 || size > MAX_FRAME_BYTES) {
      Serial.println("[RetroTV] invalid frame header; closing sender");
      resetClient();
      return true;
    }
    _recv_remain = size;
    if (drawJpg()) ++_drawCount;
    else { resetClient(); return true; }
    // Consume only the remaining bytes of this frame, never the next header.
    while (_recv_remain > 0) {
      const uint32_t n = _recv_remain < TCP_BUF_LEN ? _recv_remain : TCP_BUF_LEN;
      if (!readExact(_tcpBuf, n)) { resetClient(); return true; }
      _recv_remain -= n;
    }
    _recv_requested = false;

    if (_sec != millis() / 1000) {
      _sec = millis() / 1000;
      Serial.printf("%2d fps", _drawCount);
      if (_delayCount) Serial.printf(" / delay%3d", _delayCount);
      Serial.print("\r\n");
      _drawCount = 0;
      _delayCount = 0;
    }

    return true;
  }
private:
  static constexpr int TCP_BUF_LEN = 512;
  static constexpr int SENDER_PREFIX_SIZE = 7;
  static constexpr uint32_t _rowskip = 2;

  static constexpr uint32_t FRAME_TIMEOUT_MS = 5000;
  static constexpr uint32_t READ_TIMEOUT_MS = 1500;
  static constexpr uint32_t MAX_FRAME_BYTES = 1024 * 1024;
  bool _ready = false;
  uint32_t _requestAt = 0;
  uint32_t _frameAt = 0;
  unsigned _flip = 0;
  LovyanGFX* _lcd = nullptr;
  uint_fast8_t _bytesize;
  uint8_t* _dmabufs[2] = {};
  uint8_t* _dmabuf = nullptr;

  WiFiServer _tcp;
  WiFiClient _client;
  TJpgD _jdec = {};
  int32_t _recv_remain = 0;
  uint32_t _sec = 0;
  int32_t _lcd_width;
  int32_t _lcd_height;
  int32_t _out_width;
  int32_t _out_height;
  int32_t _off_x;
  int32_t _off_y;
  int32_t _jpg_x;
  int32_t _jpg_y;
  uint32_t _drawCount = 0;
  uint32_t _delayCount = 0;
  uint8_t _tcpBuf[TCP_BUF_LEN];
  bool _recv_requested = false;

  void resetClient() {
    _client.stop();
    _recv_requested = false;
    _recv_remain = 0;
  }

  bool readExact(uint8_t* dst, uint32_t len) {
    uint32_t progressAt = millis();
    while (len) {
      const int n = _client.read(dst, len);
      if (n > 0) {
        dst += n;
        len -= n;
        progressAt = millis();
      } else {
        if (!_client.connected() || millis() - progressAt > READ_TIMEOUT_MS
            || millis() - _frameAt > FRAME_TIMEOUT_MS) {
          Serial.println("[RetroTV] frame read interrupted/timeout");
          return false;
        }
        delay(1);
        ++_delayCount;
      }
      if (millis() - _frameAt > FRAME_TIMEOUT_MS) return false;
    }
    return true;
  }

  static uint32_t jpgRead(TJpgD *jdec, uint8_t *buf, uint32_t len) {
    TCPReceiver* me = (TCPReceiver*)jdec->device;
    if (me->_recv_remain <= 0) return 0;
    if (len > uint32_t(me->_recv_remain)) len = me->_recv_remain;
    const uint32_t requested = len;
    // The decoder can skip a segment larger than the scratch buffer.
    while (len) {
      const uint32_t n = !buf && len > TCP_BUF_LEN ? TCP_BUF_LEN : len;
      if (!me->readExact(buf ? buf : me->_tcpBuf, n)) return 0;
      me->_recv_remain -= n;
      len -= n;
      if (buf) buf += n;
    }
    return requested;
  }

  static uint32_t jpgWrite(TJpgD *jdec, void *bitmap, TJpgD::JRECT *rect) {
    TCPReceiver* me = (TCPReceiver*)jdec->device;

    auto dst = me->_dmabuf;

    uint_fast16_t x = rect->left;
    uint_fast16_t y = rect->top;
    uint_fast16_t w = rect->right + 1 - x;
    uint_fast16_t h = rect->bottom + 1 - y;
    uint_fast16_t outWidth = me->_out_width;
    uint_fast16_t outHeight = me->_out_height;
    uint8_t *src = (uint8_t*)bitmap;
    uint_fast16_t oL = 0, oR = 0;

    if (rect->right < me->_off_x)      return 1;
    if (x >= (me->_off_x + outWidth))  return 1;
    if (rect->bottom < me->_off_y)     return 1;
    if (y >= (me->_off_y + outHeight)) return 1;

    int32_t src_idx = 0;
    int32_t dst_idx = 0;

    uint_fast16_t tmpy = y % ((1 + _rowskip) * jdec->msy << 3);
    if (me->_off_y > y) {
      uint_fast16_t linesToSkip = me->_off_y - y;
      src_idx += linesToSkip * w;
      h -= linesToSkip;
      dst_idx -= tmpy * outWidth;
    } else 
    if (me->_off_y > (y - tmpy)) {
      uint_fast16_t linesToSkip = me->_off_y - (y - tmpy);
      dst_idx -= linesToSkip * outWidth;
    }

    if (me->_off_x > x) {
      oL = me->_off_x - x;
    }
    if (rect->right >= (me->_off_x + outWidth)) {
      oR = (rect->right + 1) - (me->_off_x + outWidth);
    }

    int_fast16_t line = (w - ( oL + oR ));
    dst_idx += oL + x - me->_off_x + outWidth * tmpy;
    src_idx += oL;

    me->_fp_write(&dst[dst_idx * me->_bytesize], &src[src_idx * 3], line, outWidth, w, h);

    return 1;
  }

  static void write888(uint8_t* dst, const uint8_t* src, uint32_t line, uint32_t outWidth, uint32_t w, uint32_t h)
  {
    line *= 3;
    outWidth *= 3;
    w *= 3;
    do {
      memcpy(dst, src, line);
      src += w;
      dst += outWidth;
    } while (--h);
  }

  static void write565(uint8_t* dst, const uint8_t* src, uint32_t line, uint32_t outWidth, uint32_t w, uint32_t h)
  {
    do {
      auto s = src;
      auto d = dst;
      uint32_t l = line;
      while (l--) {
        uint32_t r = s[0];
        uint32_t g = s[1];
        uint32_t b = s[2];
        s += 3;
        g = g >> 2;
        r = (r >> 3) & 0x1F;
        r = (r << 3) + (g >> 3);
        b = (b >> 3) + (g << 5);
        d[0] = r;
        d[1] = b;
        d += 2;
      }
      dst += outWidth * 2;
      src += w * 3;
    } while (--h);
  }

  static void write332(uint8_t* dst, const uint8_t* src, uint32_t line, uint32_t outWidth, uint32_t w, uint32_t h)
  {
    do {
      auto s = src;
      auto d = dst;
      uint32_t l = line;
      while (l--) {
        uint32_t r = s[0];
        uint32_t g = s[1];
        uint32_t b = s[2];
        s += 3;
        r >>= 5;
        g >>= 5;
        b >>= 6;
        g += r << 3;
        b += g << 2;
        d[0] = b;
        ++d;
      }
      dst += outWidth;
      src += w * 3;
    } while (--h);
  }

  static void writeGray(uint8_t* dst, const uint8_t* src, uint32_t line, uint32_t outWidth, uint32_t w, uint32_t h)
  {
    --dst;
    do {
      auto s = src;
      auto d = dst;
      uint32_t l = line;
      while (l--) {
        uint32_t r = s[0];
        uint32_t g = s[1];
        uint32_t b = s[2];
        s += 3;
        ++d;
        r += (g << 1);
        r += b;
        d[0] = r >> 2;
      }
      dst += outWidth;
      src += w * 3;
    } while (--h);
  }

  static uint32_t jpgWriteRow(TJpgD *jdec, uint32_t y, uint32_t h) {
    TCPReceiver* me = (TCPReceiver*)jdec->device;

    if (y == 0) {
      me->_lcd->setAddrWindow(me->_jpg_x, me->_jpg_y, me->_out_width, me->_out_height);

      // while (me->_lcd->getScanLine() > 64) { taskYIELD(); }
    }

    int16_t outY = y - me->_off_y;
    if (outY < 0) {
      if (h <= - outY) return 1;
      h += outY;
      outY = 0;
    }
    if (me->_lcd_height <= me->_jpg_y + outY) return 1;
    if (me->_lcd_height < me->_jpg_y + outY + h) {
      h = me->_lcd_height - (me->_jpg_y + outY);
    }

    switch (me->_lcd->getColorDepth())
    {
    default:
    case lgfx::color_depth_t::rgb565_2Byte:   me->_lcd->pushPixelsDMA((lgfx::swap565_t*  )(me->_dmabuf), me->_out_width * h);  break;
    case lgfx::color_depth_t::rgb888_3Byte:   me->_lcd->pushPixelsDMA((lgfx::bgr888_t*   )(me->_dmabuf), me->_out_width * h);  break;
    case lgfx::color_depth_t::rgb332_1Byte:   me->_lcd->pushPixelsDMA((lgfx::rgb332_t*   )(me->_dmabuf), me->_out_width * h);  break;
    case lgfx::color_depth_t::grayscale_8bit: me->_lcd->pushPixelsDMA((lgfx::grayscale_t*)(me->_dmabuf), me->_out_width * h);  break;
    }

    me->_flip ^= 1;
    me->_lcd->waitDMA();
    me->_dmabuf = me->_dmabufs[me->_flip];
    delay(1); // allow idle/WiFi tasks to run during CPU-heavy frames
    return 1;
  }

  bool drawJpg(void) {
    TJpgD::JRESULT jres = _jdec.prepare(jpgRead, this);
    if (jres != TJpgD::JDR_OK) {
      Serial.printf("prepare failed! %d\r\n", jres);
      return false;
    }
    if (_jdec.width <= 0 || _jdec.height <= 0 || _jdec.width > 4096 || _jdec.height > 4096) return false;
    _out_width = _jdec.width < _lcd_width ? _jdec.width : _lcd_width;
    _jpg_x = (_lcd_width - _jdec.width) >> 1;
    if (0 > _jpg_x) {
      _off_x = - _jpg_x;
      _jpg_x = 0;
    } else {
      _off_x = 0;
    }
    _out_height = _jdec.height < _lcd_height ? _jdec.height : _lcd_height;
    _jpg_y = (_lcd_height- _jdec.height) >> 1;
    if (0 > _jpg_y) {
      _off_y = - _jpg_y;
      _jpg_y = 0;
    } else {
      _off_y = 0;
    }

    _flip = 0;
    _dmabuf = _dmabufs[0];
    _lcd->startWrite();
    jres = _jdec.decomp(jpgWrite, jpgWriteRow, _rowskip);
    _lcd->waitDMA();
    _lcd->endWrite();
    if (jres != TJpgD::JDR_OK) {
      Serial.printf("decomp failed! %d\r\n", jres);
      return false;
    }

    return true;
  }

  void (*_fp_write)(uint8_t* dst, const uint8_t* src, uint32_t line, uint32_t outWidth, uint32_t w, uint32_t h);
};

#endif