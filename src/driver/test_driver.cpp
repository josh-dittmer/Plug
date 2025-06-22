#include "test_driver.h"

bool TestDriver::init() {
    if (m_init) {
        m_logger.error("init(): Already initialized!");
        return false;
    }

    m_logger.log("Initialized!");

    m_init = true;
    return true;
}

void TestDriver::shutdown() {
    if (!m_init) {
        m_logger.error("shutdown(): Not initialized!");
        return;
    }

    m_logger.log("Stopped");
}

Result<std::shared_ptr<Driver::HardwareInterface>>
TestDriver::get_interface(const Model& model) {
    switch (model) {
    case Model::PLUG_V1:
        return Result<std::shared_ptr<HardwareInterface>>::Ok(
            PlugV1Interface::create(get_ptr()));
    default:
        return Result<std::shared_ptr<HardwareInterface>>::Err(
            Error(__func__, "unsupported model"));
    }
}

void TestDriver::write() {
    if (!m_init) {
        m_logger.error("write(): Not initialized!");
        return;
    }

    m_logger.verbose("write(): Write performed");
}

void TestDriver::PlugV1Interface::on() { m_driver->write(); }

void TestDriver::PlugV1Interface::off() { m_driver->write(); }
