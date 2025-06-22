#pragma once

#include <homecontroller/util/logger.h>

class PWM {
  public:
    static bool init();
    static void write(unsigned int pin, uint8_t val);
    static void shutdown();

  private:
    static hc::util::Logger m_logger;
};