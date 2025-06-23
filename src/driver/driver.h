#pragma once

#include <homecontroller/util/logger.h>
#include <homecontroller/util/result.h>

#include <memory>

class Driver {
  public:
    class HardwareInterface;

    enum class Model { PLUG_V1 };

    static Result<Model> str_to_model(const std::string& str);

  protected:
    Driver(const std::string& log_context)
        : m_logger(log_context), m_init(false) {}

  public:
    ~Driver() {}

    virtual bool init() = 0;
    virtual void shutdown() = 0;

    virtual Result<std::shared_ptr<HardwareInterface>>
    get_interface(const Model& model) = 0;

  protected:
    hc::util::Logger m_logger;

    bool m_init;

  private:
    virtual void write() = 0;
};

class Driver::HardwareInterface {
  protected:
    HardwareInterface() : m_pin(0) {}

  public:
    ~HardwareInterface() {}

    virtual void on() = 0;
    virtual void off() = 0;

    void set_pin(unsigned int pin) { m_pin = pin; }

  protected:
    unsigned int m_pin;
};