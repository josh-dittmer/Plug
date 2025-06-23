#pragma once

#include "driver.h"

class RPiZDriver : public Driver,
                   public std::enable_shared_from_this<RPiZDriver> {
    struct Private {
        explicit Private() = default;
    };

  public:
    class PlugV1Interface;

    RPiZDriver(Private) : Driver("RPiZDriver") {}
    ~RPiZDriver() {}

    static std::shared_ptr<RPiZDriver> create() {
        return std::make_shared<RPiZDriver>(Private());
    }

    bool init() override;
    void shutdown() override;

    Result<std::shared_ptr<HardwareInterface>>
    get_interface(const Model& model) override;

    std::shared_ptr<RPiZDriver> get_ptr() { return shared_from_this(); }

  private:
    void write() override;

    void set_pin(unsigned int pin) { m_pin = pin; }
    void set_value(bool value) { m_value = value; }

    unsigned int m_pin;
    bool m_value;
};

class RPiZDriver::PlugV1Interface : public Driver::HardwareInterface {
    struct Private {
        explicit Private() = default;
    };

  public:
    PlugV1Interface(Private, const std::shared_ptr<RPiZDriver>& driver)
        : HardwareInterface(), m_driver(driver) {}
    ~PlugV1Interface() {}

    static std::shared_ptr<PlugV1Interface>
    create(const std::shared_ptr<RPiZDriver>& driver) {
        return std::make_shared<PlugV1Interface>(Private(), driver);
    }

    void on() override;
    void off() override;

  private:
    std::shared_ptr<RPiZDriver> m_driver;
};