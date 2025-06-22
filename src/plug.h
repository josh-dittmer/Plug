#pragma once

#include "driver/driver.h"

#include <homecontroller/api/device.h>
#include <homecontroller/api/device_data/plug.h>

#include <condition_variable>
#include <mutex>

class Plug : public hc::api::Device<hc::api::plug::State> {
  public:
    struct Config {
        std::string m_model_str;
        int m_gpio_pin;
        int m_lock_duration;

        std::string m_device_id;
        std::string m_secret;

        std::string m_gateway_url;
        std::string m_gateway_namespace;

        int m_reconn_delay;
        int m_reconn_attempts;
    };

    Plug(const Config& config)
        : Device("Plug@" + std::to_string(config.m_gpio_pin)),
          m_config(config) {}
    ~Plug() {}

    bool init(const std::shared_ptr<Driver>& driver);
    void shutdown();

  private:
    void loop();

    void on_command_received(
        std::map<std::string, ::sio::message::ptr>& data) override;

    ::sio::message::ptr serialize_state() const override;

    void handle_power_on(hc::api::plug::State& state);
    void handle_power_off(hc::api::plug::State& state);

    Config m_config;

    std::shared_ptr<Driver::HardwareInterface> m_interface;

    std::mutex m_mutex;
    std::condition_variable m_cv;
};