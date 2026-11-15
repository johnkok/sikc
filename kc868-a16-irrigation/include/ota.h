#pragma once

#include "controller.h"

class OtaService
{
public:
  explicit OtaService(Controller &controller) : controller_(controller) {}
  void update();

private:
  void begin();
  Controller &controller_;
  bool started_ = false;
  bool warned_ = false;
};
