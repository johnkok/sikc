#pragma once

#include "controller.h"
#include "display.h"

class OtaService
{
public:
  OtaService(Controller &controller, StatusDisplay &display) : controller_(controller), display_(display) {}
  void update();

private:
  void begin();
  Controller &controller_;
  StatusDisplay &display_;
  bool started_ = false;
  bool warned_ = false;
};
