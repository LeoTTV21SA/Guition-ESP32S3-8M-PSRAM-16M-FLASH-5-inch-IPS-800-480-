/*
 * start rewrite from:
 * https://github.com/lovyan03/LovyanGFX/blob/master/src/lgfx/v0/platforms/LGFX_PARALLEL_ESP32.hpp
 */
#include "Arduino_ESP32LCD16.h"

#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32S3)
#if (!defined(ESP_ARDUINO_VERSION_MAJOR)) || (ESP_ARDUINO_VERSION_MAJOR < 3)

#define WAIT_LCD_NOT_BUSY while (LCD_CAM.lcd_user.val & LCD_CAM_LCD_START)

Arduino_ESP32LCD16::Arduino_ESP32LCD16(
    int8_t dc, int8_t cs, int8_t wr, int8_t rd,
    int8_t d0, int8_t d1, int8_t d2, int8_t d3, int8_t d4, int8_t d5, int8_t d6, int8_t d7,
    int8_t d8, int8_t d9, int8_t d10, int8_t d11, int8_t d12, int8_t d13, int8_t d14, int8_t d15)
    : _dc(dc), _cs(cs), _wr(wr), _rd(rd),
      _d0(d0), _d1(d1), _d2(d2), _d3(d3), _d4(d4), _d5(d5), _d6(d6), _d7(d7),
      _d8(d8), _d9(d9), _d10(d10), _d11(d11), _d12(d12), _d13(d13), _d14(d14), _d15(d15)
{
}

bool Arduino_ESP32LCD16::begin(int32_t speed, int8_t dataMode)
{
  if (speed == GFX_NOT_DEFINED)
  {
    _speed = 8000000UL; // safe frequency
  }
  else
  {
    _speed = speed;
  }

  pinMode(_dc, OUTPUT);
  digitalWrite(_dc, HIGH); // Data mode

  if (_cs != GFX_NOT_DEFINED)
  {
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH); // disable chip select
  }
  if (_cs >= 32)
  {
    _csPinMask = digitalPinToBitMask(_cs);
    _csPortSet = (PORTreg_t)GPIO_OUT1_W1TS_REG;
    _csPortClr = (PORTreg_t)GPIO_OUT1_W1TC_REG;
  }
  else if (_cs != GFX_NOT_DEFINED)
  {
    _csPinMask = digitalPinToBitMask(_cs);
    _csPortSet = (PORTreg_t)GPIO_OUT_W1TS_REG;
    _csPortClr = (PORTreg_t)GPIO_OUT_W1TC_REG;
  }

  pinMode(_wr, OUTPUT);
  digitalWrite(_wr, HIGH); // Set write strobe high (inactive)

  if (_rd != GFX_NOT_DEFINED)
  {
    pinMode(_rd, OUTPUT);
    digitalWrite(_rd, HIGH);
  }

  esp_lcd_i80_bus_config_t bus_config = {
      .dc_gpio_num = _dc,
      .wr_gpio_num = _wr,
      .clk_src = LCD_CLK_SRC_PLL160M,
      .data_gpio_nums = {
          _d0, _d1, _d2, _d3, _d4, _d5, _d6, _d7,
          _d8, _d9, _d10, _d11, _d12, _d13, _d14, _d15},
      .bus_width = 16,
      .max_transfer_bytes = LCD_MAX_PIXELS_AT_ONCE * 2,
      .psram_trans_align = 0,
      .sram_trans_align = 0};
  esp_lcd_new_i80_bus(&bus_config, &_i80_bus);

  uint32_t diff = INT32_MAX;
  uint32_t div_n = 256;
  uint32_t div_a = 63;
  uint32_t div_b = 62;
  uint32_t clkcnt = 64;
  uint32_t start_cnt = std::min<uint32_t>(64u, (F_CPU / (_speed * 2) + 1));
  uint32_t end_cnt = std::max<uint32_t>(2u, F_CPU / 256u / _speed);
  if (start_cnt <= 2)
  {
    end_cnt = 1;
  }
  for (uint32_t cnt = start_cnt; diff && cnt >= end_cnt; --cnt)
  {
    float fdiv = (float)F_CPU / cnt / _speed;
    uint32_t n = std::max<uint32_t>(2u, (uint32_t)fdiv);
    fdiv -= n;

    for (uint32_t a = 63; diff && a > 0; --a)
    {
      uint32_t b = roundf(fdiv * a);
      if (a == b && n == 256)
      {
        break;
      }
      uint32_t freq = F_CPU / ((n * cnt) + (float)(b * cnt) / (float)a);
      uint32_t d = abs(_speed - (int)freq);
      if (diff <= d)
      {
        continue;
      }
      diff = d;
      clkcnt = cnt;
      div_n = n;
      div_b = b;
      div_a = a;
      if (b == 0 || a == b)
      {
        break;
      }
    }
  }
  if (div_a == div_b)
  {
    div_b = 0;
    div_n += 1;
  }
  int wait = 24 - (div_n * clkcnt);
  _fast_wait = (wait < 0) ? 0 : wait;

  lcd_cam_lcd_clock_reg_t lcd_clock;
  lcd_clock.lcd_clkcnt_n = std::max(1u, clkcnt - 1);
  lcd_clock.lcd_clk_equ_sysclk = (clkcnt == 1);
  lcd_clock.lcd_ck_idle_edge = true;
  lcd_clock.lcd_ck_out_edge = false;
  lcd_clock.lcd_clkm_div_num = div_n;
  lcd_clock.lcd_clkm_div_b = div_b;
  lcd_clock.lcd_clkm_div_a = div_a;
  lcd_clock.lcd_clk_sel = 2; // clock_select: 1=XTAL CLOCK / 2=240MHz / 3=160MHz
  lcd_clock.clk_en = true;

  LCD_CAM.lcd_clock.val = lcd_clock.val;

  _dma_chan = _i80_bus->dma_chan;
  _dmadesc = (dma_descriptor_t *)heap_caps_malloc(sizeof(dma_descriptor_t), MALLOC_CAP_DMA);

  _buffer = (uint8_t *)heap_caps_aligned_alloc(16, LCD_MAX_PIXELS_AT_ONCE * 2, MALLOC_CAP_DMA);
  if (!_buffer)
  {
    return false;
  }
  _2nd_buffer = (uint8_t *)heap_caps_aligned_alloc(16, LCD_MAX_PIXELS_AT_ONCE * 2, MALLOC_CAP_DMA);
  if (!_2nd_buffer)
  {
    return false;
  }

  return true;
}

void Arduino_ESP32LCD16::beginWrite()
{
  CS_LOW();

  LCD_CAM.lcd_misc.val = LCD_CAM_LCD_CD_IDLE_EDGE;
  LCD_CAM.lcd_user.val = 0;
  LCD_CAM.lcd_user.val = LCD_CAM_LCD_2BYTE_EN | LCD_CAM_LCD_CMD | LCD_CAM_LCD_UPDATE_REG;
}

void Arduino_ESP32LCD16::endWrite()
{
  WAIT_LCD_NOT_BUSY;

  CS_HIGH();
}

void Arduino_ESP32LCD16::writeCommand(uint8_t c)
{
  WRITECOMMAND16(c);
}

void Arduino_ESP32LCD16::writeCommand16(uint16_t c)
{
  WRITECOMMAND16(c);
}

void Arduino_ESP32LCD16::writeCommandBytes(uint8_t *data, uint32_t len)
{
  LCD_CAM.lcd_misc.val = LCD_CAM_LCD_CD_IDLE_EDGE | LCD_CAM_LCD_CD_CMD_SET;

  while (len--)
  {
    LCD_CAM.lcd_cmd_val.val = *data++;
    WAIT_LCD_NOT_BUSY;
    LCD_CAM.lcd_user.val = LCD_CAM_LCD_2BYTE_EN | LCD_CAM_LCD_CMD | LCD_CAM_LCD_UPDATE_REG | LCD_CAM_LCD_START;
  }
  LCD_CAM.lcd_misc.val = LCD_CAM_LCD_CD_IDLE_EDGE;
}

void Arduino_ESP32LCD16::write(uint8_t d)
{
  WRITE16(d);
}

void Arduino_ESP32LCD16::write16(uint16_t d)
{
  WRITE16(d);
}

void Arduino_ESP32LCD16::writeRepeat(uint16_t p, uint32_t len)
{
  if (len < USE_DMA_THRESHOLD)
  {
    while (len--)
    {
      WRITE16(p);
    }
  }
  else
  {
    uint32_t bufLen = (len < LCD_MAX_PIXELS_AT_ONCE) ? len : LCD_MAX_PIXELS_AT_ONCE;
    uint32_t xferLen, l;
    uint32_t c32 = p * 0x10001;

    l = (bufLen + 1) / 2;
    for (uint32_t i = 0; i < l; i++)
    {
      _buffer32[i] = c32;
    }

    while (len) // While pixels remain
    {
      xferLen = (bufLen <= len) ? bufLen : len; // How many this pass?

      l = (xferLen - 2) * 2;
      *(uint32_t *)_dmadesc = ((l + 3) & (~3)) | l << 12 | 0xC0000000;
      _dmadesc->buffer = _buffer;
      _dmadesc->next = nullptr;
      gdma_start(_dma_chan, (intptr_t)(_dmadesc));
      LCD_CAM.lcd_misc.val = LCD_CAM_LCD_CD_IDLE_EDGE;
      LCD_CAM.lcd_cmd_val.val = c32;

      uint32_t wait = _fast_wait;
      if (wait > 0)
      {
        do
        {
          __asm__ __volatile__("nop");
        } while (--wait);
      }

      len -= xferLen;

      LCD_CAM.lcd_user.val = LCD_CAM_LCD_ALWAYS_OUT_EN | LCD_CAM_LCD_2BYTE_EN | LCD_CAM_LCD_CMD_2_CYCLE_EN | LCD_CAM_LCD_DOUT | LCD_CAM_LCD_CMD | LCD_CAM_LCD_UPDATE_REG | LCD_CAM_LCD_START;

      WAIT_LCD_NOT_BUSY;
    }
  }
}

void Arduino_ESP32LCD16::writePixels(uint16_t *data, uint32_t len)
{
  uint32_t xferLen, l;

  while (len > USE_DMA_THRESHOLD) // While pixels remain
  {
    xferLen = (len < LCD_MAX_PIXELS_AT_ONCE) ? len : LCD_MAX_PIXELS_AT_ONCE; // How many this pass?
    _data32.value16 = *data++;
    _data32.value16_2 = *data++;

    l = xferLen - 2;
    l <<= 1;
    *(uint32_t *)_dmadesc = ((l + 3) & (~3)) | l << 12 | 0xC0000000;
    _dmadesc->buffer = data;
    _dmadesc->next = nullptr;
    gdma_start(_dma_chan, (intptr_t)(_dmadesc));
    LCD_CAM.lcd_misc.val = LCD_CAM_LCD_CD_IDLE_EDGE;
    LCD_CAM.lcd_cmd_val.val = _data32.value;

    uint32_t wait = _fast_wait;
    while (wait--)
    {
      __asm__ __volatile__("nop");
    }

    data += xferLen - 2;
    len -= xferLen;

    LCD_CAM.lcd_user.val = LCD_CAM_LCD_ALWAYS_OUT_EN | LCD_CAM_LCD_2BYTE_EN | LCD_CAM_LCD_CMD_2_CYCLE_EN | LCD_CAM_LCD_DOUT | LCD_CAM_LCD_CMD | LCD_CAM_LCD_UPDATE_REG | LCD_CAM_LCD_START;

    WAIT_LCD_NOT_BUSY;
  }

  while (len--)
  {
    WRITE16(*data++);
  }
}

void Arduino_ESP32LCD16::writeC8D8(uint8_t c, uint8_t d)
{
  WRITECOMMAND16(c);
  WRITE16(d);
}

void Arduino_ESP32LCD16::writeC8D16(uint8_t c, uint16_t d)
{
  WRITECOMMAND16(c);
  WRITE16(d);
}

void Arduino_ESP32LCD16::writeC8D16D16(uint8_t c, uint16_t d1, uint16_t d2)
{
  WRITECOMMAND16(c);
  WRITE16(d1);
  WRITE16(d2);
}

void Arduino_ESP32LCD16::writeC8D16D16Split(uint8_t c, uint16_t d1, uint16_t d2)
{
  WRITECOMMAND16(c);

  _data16.value = d1;
  WRITE16(_data16.msb);
  WRITE16(_data16.lsb);

  _data16.value = d2;
  WRITE16(_data16.msb);
  WRITE16(_data16.lsb);
}

void Arduino_ESP32LCD16::writeBytes(uint8_t *data, uint32_t len)
{
  uint32_t xferLen, l;

  while (len > (USE_DMA_THRESHOLD * 2)) // While pixels remain
  {
    xferLen = (len < (LCD_MAX_PIXELS_AT_ONCE * 2)) ? len : (LCD_MAX_PIXELS_AT_ONCE * 2); // How many this pass?
    _data32.msb = *data++;
    _data32.lsb = *data++;
    _data32.msb_2 = *data++;
    _data32.lsb_2 = *data++;

    l = xferLen - 4;
    l >>= 1;
    for (int i = 0; i < l; ++i)
    {
      _buffer[(i * 2) + 1] = *data++;
      _buffer[i * 2] = *data++;
    }

    l <<= 1;
    *(uint32_t *)_dmadesc = ((l + 3) & (~3)) | l << 12 | 0xC0000000;
    _dmadesc->buffer = _buffer;
    _dmadesc->next = nullptr;
    gdma_start(_dma_chan, (intptr_t)(_dmadesc));
    LCD_CAM.lcd_misc.val = LCD_CAM_LCD_CD_IDLE_EDGE;
    LCD_CAM.lcd_cmd_val.val = _data32.value;

    uint32_t wait = _fast_wait;
    while (wait--)
    {
      __asm__ __volatile__("nop");
    }

    len -= xferLen;

    LCD_CAM.lcd_user.val = LCD_CAM_LCD_ALWAYS_OUT_EN | LCD_CAM_LCD_2BYTE_EN | LCD_CAM_LCD_CMD_2_CYCLE_EN | LCD_CAM_LCD_DOUT | LCD_CAM_LCD_CMD | LCD_CAM_LCD_UPDATE_REG | LCD_CAM_LCD_START;
    WAIT_LCD_NOT_BUSY;
  }

  while (len)
  {
    if (len == 1)
    {
      WRITE16(*data);
      len--;
    }
    else
    {
      _data16.lsb = *data++;
      _data16.msb = *data++;
      WRITE16(_data16.value);
      len -= 2;
    }
  }
}

void Arduino_ESP32LCD16::writeIndexedPixels(uint8_t *data, uint16_t *idx, uint32_t len)
{
  uint32_t xferLen, l;
  uint32_t p;

  while (len > USE_DMA_THRESHOLD) // While pixels remain
  {
    xferLen = (len < LCD_MAX_PIXELS_AT_ONCE) ? len : LCD_MAX_PIXELS_AT_ONCE; // How many this pass?
    l = xferLen - 2;
    p = idx[*data++];
    p <<= 16;
    p |= idx[*data++];

    for (int i = 0; i < l; ++i)
    {
      _buffer16[i] = idx[*data++];
    }

    l <<= 1;
    *(uint32_t *)_dmadesc = ((l + 3) & (~3)) | l << 12 | 0xC0000000;
    _dmadesc->buffer = _buffer;
    _dmadesc->next = nullptr;
    gdma_start(_dma_chan, (intptr_t)(_dmadesc));
    LCD_CAM.lcd_misc.val = LCD_CAM_LCD_CD_IDLE_EDGE;
    LCD_CAM.lcd_cmd_val.val = p;

    uint32_t wait = _fast_wait;
    while (wait--)
    {
      __asm__ __volatile__("nop");
    }

    len -= xferLen;

    LCD_CAM.lcd_user.val = LCD_CAM_LCD_ALWAYS_OUT_EN | LCD_CAM_LCD_2BYTE_EN | LCD_CAM_LCD_CMD_2_CYCLE_EN | LCD_CAM_LCD_DOUT | LCD_CAM_LCD_CMD | LCD_CAM_LCD_UPDATE_REG | LCD_CAM_LCD_START;
    WAIT_LCD_NOT_BUSY;
  }

  while (len--)
  {
    WRITE16(idx[*data++]);
  }
}

void Arduino_ESP32LCD16::writeIndexedPixelsDouble(uint8_t *data, uint16_t *idx, uint32_t len)
{
  len <<= 1; // double length
  uint32_t xferLen, l;
  uint32_t p;

  while (len > USE_DMA_THRESHOLD) // While pixels remain
  {
    xferLen = (len < LCD_MAX_PIXELS_AT_ONCE) ? len : LCD_MAX_PIXELS_AT_ONCE; // How many this pass?
    l = (xferLen - 2) / 2;
    p = idx[*data++] * 0x10001;

    for (int i = 0; i < l; ++i)
    {
      _buffer32[i] = idx[*data++] * 0x10001;
    }

    l <<= 2;
    *(uint32_t *)_dmadesc = ((l + 3) & (~3)) | l << 12 | 0xC0000000;
    _dmadesc->buffer = _buffer;
    _dmadesc->next = nullptr;
    gdma_start(_dma_chan, (intptr_t)(_dmadesc));
    LCD_CAM.lcd_misc.val = LCD_CAM_LCD_CD_IDLE_EDGE;
    LCD_CAM.lcd_cmd_val.val = p;

    uint32_t wait = _fast_wait;
    while (wait--)
    {
      __asm__ __volatile__("nop");
    }

    len -= xferLen;

    LCD_CAM.lcd_user.val = LCD_CAM_LCD_ALWAYS_OUT_EN | LCD_CAM_LCD_2BYTE_EN | LCD_CAM_LCD_CMD_2_CYCLE_EN | LCD_CAM_LCD_DOUT | LCD_CAM_LCD_CMD | LCD_CAM_LCD_UPDATE_REG | LCD_CAM_LCD_START;
    WAIT_LCD_NOT_BUSY;
  }

  len >>= 1;
  while (len--)
  {
    p = idx[*data++] * 0x10001;
    WRITE32(p);
  }
}

void Arduino_ESP32LCD16::writeYCbCrPixels(uint8_t *yData, uint8_t *cbData, uint8_t *crData, uint16_t w, uint16_t h)
{
  if (w > (LCD_MAX_PIXELS_AT_ONCE / 2))
  {
    Arduino_DataBus::writeYCbCrPixels(yData, cbData, crData, w, h);
  }
  else
  {
    int cols = w >> 1;
    int rows = h >> 1;
    uint8_t *yData2 = yData + w;
    uint16_t *dest = _buffer16;
    uint16_t *dest2 = dest + w;

    uint8_t pxCb, pxCr;
    int16_t pxR, pxG, pxB, pxY;
  
    uint16_t l = (w * 4) - 4;
    uint32_t out_dmadesc = ((l + 3) & (~3)) | l << 12 | 0xC0000000;
    bool poll_started = false;
    for (int row = 0; row < rows; ++row)
    {
      for (int col = 0; col < cols; ++col)
      {
        pxCb = *cbData++;
        pxCr = *crData++;
        pxR = CR2R16[pxCr];
        pxG = -CB2G16[pxCb] - CR2G16[pxCr];
        pxB = CB2B16[pxCb];

        pxY = Y2I16[*yData++];
        *dest++ = CLIPR[pxY + pxR] | CLIPG[pxY + pxG] | CLIPB[pxY + pxB];
        pxY = Y2I16[*yData++];
        *dest++ = CLIPR[pxY + pxR] | CLIPG[pxY + pxG] | CLIPB[pxY + pxB];
        pxY = Y2I16[*yData2++];
        *dest2++ = CLIPR[pxY + pxR] | CLIPG[pxY + pxG] | CLIPB[pxY + pxB];
        pxY = Y2I16[*yData2++];
        *dest2++ = CLIPR[pxY + pxR] | CLIPG[pxY + pxG] | CLIPB[pxY + pxB];
      }
      yData += w;
      yData2 += w;

      if (poll_started)
      {
        WAIT_LCD_NOT_BUSY;
      }
      else
      {
        poll_started = true;
      }

      *(uint32_t *)_dmadesc = out_dmadesc;
      if (row & 1)
      {
        _dmadesc->buffer = _2nd_buffer32 + 1;
        _dmadesc->next = nullptr;
        gdma_start(_dma_chan, (intptr_t)(_dmadesc));
        LCD_CAM.lcd_misc.val = LCD_CAM_LCD_CD_IDLE_EDGE;
        LCD_CAM.lcd_cmd_val.val = *_2nd_buffer32;
        dest = _buffer16;
      }
      else
      {
        _dmadesc->buffer = _buffer32 + 1;
        _dmadesc->next = nullptr;
        gdma_start(_dma_chan, (intptr_t)(_dmadesc));
        LCD_CAM.lcd_misc.val = LCD_CAM_LCD_CD_IDLE_EDGE;
        LCD_CAM.lcd_cmd_val.val = *_buffer32;
        dest = _2nd_buffer16;
      }

      uint32_t wait = _fast_wait;
      while (wait--)
      {
        __asm__ __volatile__("nop");
      }
      LCD_CAM.lcd_user.val = LCD_CAM_LCD_ALWAYS_OUT_EN | LCD_CAM_LCD_2BYTE_EN | LCD_CAM_LCD_CMD_2_CYCLE_EN | LCD_CAM_LCD_DOUT | LCD_CAM_LCD_CMD | LCD_CAM_LCD_UPDATE_REG | LCD_CAM_LCD_START;

      dest2 = dest + w;
    }

    WAIT_LCD_NOT_BUSY;
  }
}

GFX_INLINE void Arduino_ESP32LCD16::WRITECOMMAND16(uint16_t c)
{
  LCD_CAM.lcd_misc.val = LCD_CAM_LCD_CD_IDLE_EDGE | LCD_CAM_LCD_CD_CMD_SET;
  LCD_CAM.lcd_cmd_val.val = c;
  WAIT_LCD_NOT_BUSY;
  LCD_CAM.lcd_user.val = LCD_CAM_LCD_2BYTE_EN | LCD_CAM_LCD_CMD | LCD_CAM_LCD_UPDATE_REG | LCD_CAM_LCD_START;
}

GFX_INLINE void Arduino_ESP32LCD16::WRITE16(uint16_t d)
{
  LCD_CAM.lcd_misc.val = LCD_CAM_LCD_CD_IDLE_EDGE;
  LCD_CAM.lcd_cmd_val.val = d;
  WAIT_LCD_NOT_BUSY;
  LCD_CAM.lcd_user.val = LCD_CAM_LCD_2BYTE_EN | LCD_CAM_LCD_CMD | LCD_CAM_LCD_UPDATE_REG | LCD_CAM_LCD_START;
}

GFX_INLINE void Arduino_ESP32LCD16::WRITE32(uint32_t d)
{
  LCD_CAM.lcd_misc.val = LCD_CAM_LCD_CD_IDLE_EDGE;
  LCD_CAM.lcd_cmd_val.val = d;
  WAIT_LCD_NOT_BUSY;
  LCD_CAM.lcd_user.val = LCD_CAM_LCD_CMD_2_CYCLE_EN | LCD_CAM_LCD_2BYTE_EN | LCD_CAM_LCD_CMD | LCD_CAM_LCD_UPDATE_REG | LCD_CAM_LCD_START;
}

/******** low level bit twiddling **********/

GFX_INLINE void Arduino_ESP32LCD16::CS_HIGH(void)
{
  if (_cs != GFX_NOT_DEFINED)
  {
    *_csPortSet = _csPinMask;
  }
}

GFX_INLINE void Arduino_ESP32LCD16::CS_LOW(void)
{
  if (_cs != GFX_NOT_DEFINED)
  {
    *_csPortClr = _csPinMask;
  }
}

#endif // #if (!defined(ESP_ARDUINO_VERSION_MAJOR)) || (ESP_ARDUINO_VERSION_MAJOR < 3)
#endif // #if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32S3)
