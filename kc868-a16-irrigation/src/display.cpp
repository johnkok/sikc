#include "display.h"
#include "config.h"
#include <math.h>

namespace
{
constexpr uint8_t DIGITS[10][5] = {
    {0b111, 0b101, 0b101, 0b101, 0b111},
    {0b010, 0b110, 0b010, 0b010, 0b111},
    {0b111, 0b001, 0b111, 0b100, 0b111},
    {0b111, 0b001, 0b111, 0b001, 0b111},
    {0b101, 0b101, 0b111, 0b001, 0b001},
    {0b111, 0b100, 0b111, 0b001, 0b111},
    {0b111, 0b100, 0b111, 0b101, 0b111},
    {0b111, 0b001, 0b010, 0b010, 0b010},
    {0b111, 0b101, 0b111, 0b101, 0b111},
    {0b111, 0b101, 0b111, 0b001, 0b111}};
}

StatusDisplay::StatusDisplay(Controller &controller, Settings &settings)
    : controller_(controller), settings_(settings),
      pixels_(Config::DISPLAY_PIXEL_COUNT, Config::DISPLAY_PIN, NEO_GRB + NEO_KHZ800) {}

void StatusDisplay::begin()
{
  pixels_.begin();
  pixels_.setBrightness(map(Config::DISPLAY_DEFAULT_BRIGHTNESS, 0, 100, 0, 255));
  pixels_.clear();
  pixels_.show();
}

void StatusDisplay::setPixel(uint8_t matrix, int8_t x, int8_t y, uint32_t color)
{
  if (matrix > 1 || x < 0 || x >= Config::DISPLAY_MATRIX_WIDTH || y < 0 || y >= Config::DISPLAY_MATRIX_HEIGHT)
    return;
  if (Config::DISPLAY_FLIP_X[matrix])
    x = Config::DISPLAY_MATRIX_WIDTH - 1 - x;
  if (Config::DISPLAY_FLIP_Y[matrix])
    y = Config::DISPLAY_MATRIX_HEIGHT - 1 - y;
  if (Config::DISPLAY_SERPENTINE_ROWS && (y & 1))
    x = Config::DISPLAY_MATRIX_WIDTH - 1 - x;
  const uint16_t local = y * Config::DISPLAY_MATRIX_WIDTH + x;
  const uint8_t chainMatrix = Config::DISPLAY_REVERSE_CHAIN ? 1 - matrix : matrix;
  pixels_.setPixelColor(chainMatrix * Config::DISPLAY_MATRIX_WIDTH * Config::DISPLAY_MATRIX_HEIGHT + local, color);
}

void StatusDisplay::drawDigit(uint8_t matrix, int8_t x, int8_t y, uint8_t digit, uint32_t color)
{
  if (digit > 9)
    return;
  for (uint8_t row = 0; row < 5; row++)
    for (uint8_t col = 0; col < 3; col++)
      if (DIGITS[digit][row] & (1 << (2 - col)))
        setPixel(matrix, x + col, y + row, color);
}

void StatusDisplay::drawNumber(uint8_t matrix, int value, uint32_t color)
{
  value = constrain(value, -9, 99);
  if (value < 0)
  {
    setPixel(matrix, 0, 3, color);
    setPixel(matrix, 1, 3, color);
    drawDigit(matrix, 4, 1, -value, color);
  }
  else if (value < 10)
    drawDigit(matrix, 3, 1, value, color);
  else
  {
    drawDigit(matrix, 0, 1, value / 10, color);
    drawDigit(matrix, 4, 1, value % 10, color);
  }
}

void StatusDisplay::drawX(uint8_t matrix, uint32_t color)
{
  for (uint8_t i = 1; i < 7; i++)
  {
    setPixel(matrix, i, i, color);
    setPixel(matrix, 7 - i, i, color);
  }
}

void StatusDisplay::drawCheck(uint8_t matrix, uint32_t color)
{
  setPixel(matrix, 1, 4, color); setPixel(matrix, 2, 5, color); setPixel(matrix, 3, 6, color);
  setPixel(matrix, 4, 5, color); setPixel(matrix, 5, 4, color); setPixel(matrix, 6, 3, color);
}

void StatusDisplay::drawWifi(uint8_t matrix, uint32_t color)
{
  for (uint8_t x = 1; x < 7; x++) setPixel(matrix, x, 1, color);
  setPixel(matrix, 1, 2, color); setPixel(matrix, 6, 2, color);
  for (uint8_t x = 2; x < 6; x++) setPixel(matrix, x, 3, color);
  setPixel(matrix, 2, 4, color); setPixel(matrix, 5, 4, color);
  setPixel(matrix, 3, 5, color); setPixel(matrix, 4, 5, color);
  setPixel(matrix, 3, 7, color); setPixel(matrix, 4, 7, color);
}

void StatusDisplay::drawMqtt(uint8_t matrix, uint32_t color)
{
  for (uint8_t y = 1; y < 7; y++)
  {
    setPixel(matrix, 1, y, color);
    setPixel(matrix, 6, y, color);
  }
  setPixel(matrix, 2, 2, color); setPixel(matrix, 5, 2, color);
  setPixel(matrix, 3, 3, color); setPixel(matrix, 4, 3, color);
}

void StatusDisplay::drawRain(uint8_t matrix, uint32_t color)
{
  for (uint8_t x = 2; x < 6; x++) setPixel(matrix, x, 2, color);
  for (uint8_t x = 1; x < 7; x++) setPixel(matrix, x, 3, color);
  setPixel(matrix, 1, 2, color); setPixel(matrix, 6, 3, color);
  setPixel(matrix, 2, 5, color); setPixel(matrix, 4, 6, color); setPixel(matrix, 6, 5, color);
}

void StatusDisplay::drawValve(uint8_t matrix, int8_t activeZone, uint32_t color)
{
  for (uint8_t x = 0; x < 8; x++) setPixel(matrix, x, 4, color);
  for (uint8_t x = 2; x < 6; x++) { setPixel(matrix, x, 2, color); setPixel(matrix, x, 6, color); }
  setPixel(matrix, 2, 3, color); setPixel(matrix, 5, 3, color);
  setPixel(matrix, 2, 5, color); setPixel(matrix, 5, 5, color);
  setPixel(matrix, 3, 0, color); setPixel(matrix, 4, 0, color);
  setPixel(matrix, 3, 1, color); setPixel(matrix, 4, 1, color);
  if (activeZone >= 0)
    drawDigit(matrix, 3, 2, activeZone + 1, color);
}

void StatusDisplay::drawWell(uint8_t matrix, uint32_t color)
{
  for (uint8_t x = 1; x < 7; x++) { setPixel(matrix, x, 2, color); setPixel(matrix, x, 6, color); }
  for (uint8_t y = 2; y < 7; y++) { setPixel(matrix, 1, y, color); setPixel(matrix, 6, y, color); }
  setPixel(matrix, 3, 0, color); setPixel(matrix, 4, 0, color);
  setPixel(matrix, 4, 1, color); setPixel(matrix, 7, 3, color); setPixel(matrix, 7, 4, color);
  if (controller_.wellOn() || controller_.filling())
  {
    const uint8_t level = 3 + ((millis() / 350) % 3);
    for (uint8_t y = level; y < 6; y++) for (uint8_t x = 2; x < 6; x++) setPixel(matrix, x, y, color);
  }
}

void StatusDisplay::drawFlow(uint8_t matrix, uint32_t irrigationColor, uint32_t wellColor)
{
  // Left droplet is irrigation flow; right droplet is well flow.
  setPixel(matrix, 2, 1, irrigationColor); setPixel(matrix, 1, 2, irrigationColor);
  setPixel(matrix, 2, 2, irrigationColor); setPixel(matrix, 3, 2, irrigationColor);
  setPixel(matrix, 1, 3, irrigationColor); setPixel(matrix, 2, 4, irrigationColor); setPixel(matrix, 3, 3, irrigationColor);
  setPixel(matrix, 5, 1, wellColor); setPixel(matrix, 4, 2, wellColor);
  setPixel(matrix, 5, 2, wellColor); setPixel(matrix, 6, 2, wellColor);
  setPixel(matrix, 4, 3, wellColor); setPixel(matrix, 5, 4, wellColor); setPixel(matrix, 6, 3, wellColor);
  for (uint8_t x = 1; x < 7; x++) setPixel(matrix, x, 6, (x < 4) ? irrigationColor : wellColor);
}

void StatusDisplay::renderStatus(bool wifiConnected, bool mqttConnected)
{
  const uint32_t red = pixels_.Color(255, 0, 0);
  const uint32_t green = pixels_.Color(0, 180, 30);
  const uint32_t blue = pixels_.Color(0, 70, 255);
  const uint32_t cyan = pixels_.Color(0, 180, 180);
  const uint32_t amber = pixels_.Color(255, 80, 0);
  const uint32_t dimBlue = pixels_.Color(0, 20, 45);

  if (controller_.faultMask())
    drawX(0, red);
  else if (controller_.maintenanceMode())
  {
    for (uint8_t x = 1; x < 7; x++) setPixel(0, x, 1, amber);
    for (uint8_t y = 1; y < 7; y++) setPixel(0, 1, y, amber);
  }
  else
  {
    int8_t activeZone = -1;
    for (uint8_t i = 0; i < 4; i++) if (controller_.zoneOn(i)) { activeZone = i; break; }
    const uint8_t page = (millis() / Config::DISPLAY_STATUS_PAGE_MS) % 6;
    switch (page)
    {
    case 0:
      drawWifi(0, wifiConnected ? green : red);
      break;
    case 1:
      drawMqtt(0, mqttConnected ? green : red);
      break;
    case 2:
      drawRain(0, controller_.rainActive() ? red : (controller_.weatherBlocked() ? amber : green));
      break;
    case 3:
      drawValve(0, activeZone, activeZone >= 0 ? blue : green);
      break;
    case 4:
      drawWell(0, controller_.wellOn() || controller_.filling() ? cyan : green);
      break;
    default:
      {
        const bool irrigationExpected = activeZone >= 0;
        const bool wellExpected = controller_.wellOn();
        const uint32_t irrigationColor = controller_.irrigationLpm() > 0.05f ? green : (irrigationExpected ? amber : dimBlue);
        const uint32_t wellColor = controller_.wellLpm() > 0.05f ? green : (wellExpected ? amber : dimBlue);
        drawFlow(0, irrigationColor, wellColor);
      }
      break;
    }
  }
}

void StatusDisplay::renderSensor(const SensorReading sensors[2])
{
  const uint8_t page = (millis() / 3000UL) % 4;
  const uint8_t sensor = page / 2;
  const bool moisture = page & 1;
  const uint32_t color = moisture ? pixels_.Color(0, 90, 255) : pixels_.Color(255, 70, 0);
  const uint32_t red = pixels_.Color(255, 0, 0);

  // Top-left marker: orange means temperature, blue means moisture.
  setPixel(1, 0, 0, color);
  // Top-right marker identifies probe 1 or 2.
  setPixel(1, 7, 0, color);
  if (sensor == 1) setPixel(1, 6, 0, color);

  if (!sensors[sensor].valid)
  {
    for (uint8_t x = 1; x < 7; x++) setPixel(1, x, 4, red);
    return;
  }
  const float raw = moisture ? sensors[sensor].moisture : sensors[sensor].temperature;
  drawNumber(1, lroundf(raw), color);
}

void StatusDisplay::renderOta()
{
  const uint32_t green = pixels_.Color(0, 220, 50);
  const uint32_t amber = pixels_.Color(255, 80, 0);
  const uint32_t red = pixels_.Color(255, 0, 0);
  if (otaState_ == OtaState::Error)
  {
    drawX(0, red); drawX(1, red); return;
  }
  if (otaState_ == OtaState::Finished)
  {
    drawCheck(0, green); drawCheck(1, green); return;
  }
  const uint8_t columns = map(otaPercent_, 0, 100, 0, 16);
  for (uint8_t x = 0; x < columns; x++)
    for (uint8_t y = 2; y < 6; y++)
      setPixel(x / 8, x % 8, y, amber);
  drawNumber(1, otaPercent_, green);
}

void StatusDisplay::render(const SensorReading sensors[2], bool wifiConnected, bool mqttConnected)
{
  pixels_.clear();
  if (otaState_ == OtaState::Idle)
  {
    renderStatus(wifiConnected, mqttConnected);
    renderSensor(sensors);
  }
  else
    renderOta();
  pixels_.show();
}

void StatusDisplay::update(const SensorReading sensors[2], bool wifiConnected, bool mqttConnected)
{
  const uint8_t brightness = constrain(settings_.displayBrightness, 0, 100);
  if (brightness != appliedBrightness_)
  {
    appliedBrightness_ = brightness;
    pixels_.setBrightness(map(brightness, 0, 100, 0, 255));
  }
  const uint32_t now = millis();
  if (now - lastFrame_ < Config::DISPLAY_REFRESH_MS)
    return;
  lastFrame_ = now;
  render(sensors, wifiConnected, mqttConnected);
}

void StatusDisplay::otaStarted() { otaState_ = OtaState::Running; otaPercent_ = 0; }
void StatusDisplay::otaProgress(uint8_t percent) { otaState_ = OtaState::Running; otaPercent_ = constrain(percent, 0, 100); }
void StatusDisplay::otaFinished() { otaState_ = OtaState::Finished; otaPercent_ = 100; }
void StatusDisplay::otaError() { otaState_ = OtaState::Error; }
