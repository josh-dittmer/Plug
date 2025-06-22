#pragma once

#include "driver.h"

class TestDriver : public Driver,
                   public std::enable_shared_from_this<TestDriver> {
    struct Private {
        explicit Private() = default;
    };

  public:
    class PlugV1Interface;

    TestDriver(Private) : Driver("TestDriver") {}
    ~TestDriver() {}

    static std::shared_ptr<TestDriver> create() {
        return std::make_shared<TestDriver>(Private());
    }

    bool init() override;
    void shutdown() override;

    Result<std::shared_ptr<HardwareInterface>>
    get_interface(const Model& model) override;

    std::shared_ptr<TestDriver> get_ptr() { return shared_from_this(); }

  private:
    void write() override;
};

class TestDriver::PlugV1Interface : public Driver::HardwareInterface {
    struct Private {
        explicit Private() = default;
    };

  public:
    PlugV1Interface(Private, const std::shared_ptr<TestDriver>& driver)
        : HardwareInterface(), m_driver(driver) {}
    ~PlugV1Interface() {}

    static std::shared_ptr<PlugV1Interface>
    create(const std::shared_ptr<TestDriver>& driver) {
        return std::make_shared<PlugV1Interface>(Private(), driver);
    }

    void on() override;
    void off() override;

  private:
    std::shared_ptr<TestDriver> m_driver;
};