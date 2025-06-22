#include "rpi_z_driver.h"

#include <pigpio.h>

bool RPiZDriver::init() {
    if (m_init) {
        m_logger.error("init(): GPIO already initialized!");
        return false;
    }

    if (gpioInitialise() < 0) {
        m_logger.error("Failed to initialize GPIO!");
        return false;
    }

    m_logger.log("GPIO initialized!");

    m_init = true;
    return true;
}

void RPiZDriver::shutdown() {
    if (!m_init) {
        m_logger.error("shutdown(): GPIO not initialized!");
        return;
    }

    gpioTerminate();

    m_logger.log("GPIO stopped");
}

Result<std::shared_ptr<Driver::HardwareInterface>>
RPiZDriver::get_interface(const Model& model) {
    switch (model) {
    case Model::PLUG_V1:
        return Result<std::shared_ptr<HardwareInterface>>::Ok(
            PlugV1Interface::create(get_ptr()));
    default:
        return Result<std::shared_ptr<HardwareInterface>>::Err(
            Error(__func__, "unsupported model"));
    }
}

void RPiZDriver::write() {
    if (!m_init) {
        m_logger.error("write(): GPIO not initialized!");
        return;
    }

    if (gpioWrite(m_pin, m_value ? 1 : 0) != 0) {
        m_logger.error("write(): Write failed");
    }
}

void RPiZDriver::PlugV1Interface::on() {
    m_driver->set_pin(m_pin);
    m_driver->set_value(true);
    m_driver->write();
}

void RPiZDriver::PlugV1Interface::off() {
    m_driver->set_pin(m_pin);
    m_driver->set_value(false);
    m_driver->write();
}
