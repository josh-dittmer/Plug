#include "plug.h"

#include <thread>

bool Plug::init(const std::shared_ptr<Driver>& driver) {
    get_logger().log("Initialization started!");

    Result<Driver::Model> model_res =
        Driver::str_to_model(m_config.m_model_str);
    if (!model_res.is_ok()) {
        get_logger().error("Failed to get device model: " +
                           model_res.unwrap_err());
        return false;
    }

    Result<std::shared_ptr<Driver::HardwareInterface>> interface_res =
        driver->get_interface(model_res.unwrap());
    if (!interface_res.is_ok()) {
        get_logger().error("Failed to get hardware interface: " +
                           interface_res.unwrap_err());
        return false;
    }

    get_logger().verbose("Using hardware interface for " +
                         m_config.m_model_str);

    m_interface = interface_res.unwrap();
    m_interface->set_pin(m_config.m_gpio_pin);

    // create initial state
    hc::api::plug::State state;
    state.m_power_state = hc::api::plug::State::PowerState::OFF;
    state.m_lock_duration = m_config.m_lock_duration;

    // device opts
    Device::StartParams params;
    params.m_gateway = {m_config.m_gateway_url, m_config.m_gateway_namespace};
    params.m_device_id = m_config.m_device_id;
    params.m_secret = m_config.m_secret;
    params.m_initial_state = state;
    params.m_reconn_delay = m_config.m_reconn_delay;
    params.m_reconn_attempts = m_config.m_reconn_attempts;

    // start main device loop
    start(params);

    std::thread loop_thread;
    if (is_client_running()) {
        get_logger().verbose("Starting loop thread...");
        loop_thread = std::thread(&Plug::loop, this);
    }

    // blocks until device loop exits
    await_finish_and_cleanup();

    if (loop_thread.joinable()) {
        get_logger().verbose("Waiting for loop thread to exit...");
        m_cv.notify_all();
        loop_thread.join();
    }

    return true;
}
void Plug::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    stop();
}

void Plug::loop() {
    while (is_client_running()) {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_cv.wait(lock);
        if (!is_client_running()) {
            return;
        }

        get_logger().verbose("loop(): Locking power state change");

        lock.unlock();
        std::this_thread::sleep_for(
            std::chrono::milliseconds(m_config.m_lock_duration));
        lock.lock();

        get_logger().verbose("loop(): Unlocking power state change");

        hc::api::plug::State new_state = get_state();

        if (get_state().m_power_state ==
            hc::api::plug::State::PowerState::ON_LOCKED) {
            new_state.m_power_state = hc::api::plug::State::PowerState::ON;
        } else if (get_state().m_power_state ==
                   hc::api::plug::State::PowerState::OFF_LOCKED) {
            new_state.m_power_state = hc::api::plug::State::PowerState::OFF;
        }

        update_state(new_state);
    }
}

void Plug::on_command_received(
    std::map<std::string, ::sio::message::ptr>& data) {
    std::lock_guard<std::mutex> lock(m_mutex);

    get_logger().verbose("on_command_received(): Reading command...");

    std::string cmd_name = data["command"]->get_string();

    get_logger().verbose("on_command_received(): Command name is \"" +
                         cmd_name + "\"");

    Result<hc::api::plug::Command> cmd_res =
        hc::api::plug::string_to_command(cmd_name);

    if (!cmd_res.is_ok()) {
        get_logger().verbose("on_command_received(): Unimplemented command");
        return;
    }

    hc::api::plug::State new_state = get_state();
    bool needs_update = false;

    switch (cmd_res.unwrap()) {
    case hc::api::plug::Command::PowerOn:
        get_logger().verbose(
            "on_command_received(): Executing power on handler");
        handle_power_on(new_state);
        needs_update = true;
        break;
    case hc::api::plug::Command::PowerOff:
        get_logger().verbose(
            "on_command_received(): Executing power off handler");
        handle_power_off(new_state);
        needs_update = true;
        break;
    }

    if (needs_update) {
        update_state(new_state);
    }
}

::sio::message::ptr Plug::serialize_state() const {
    ::sio::message::ptr state_msg = ::sio::object_message::create();
    state_msg->get_map()["powerState"] = ::sio::string_message::create(
        hc::api::plug::power_state_to_string(get_state().m_power_state));
    state_msg->get_map()["lockDuration"] =
        ::sio::int_message::create(get_state().m_lock_duration);

    return state_msg;
}

void Plug::handle_power_on(hc::api::plug::State& state) {
    if (state.m_power_state == hc::api::plug::State::PowerState::ON ||
        state.m_power_state == hc::api::plug::State::PowerState::ON_LOCKED) {
        get_logger().debug("handle_power_on(): Power already on!");
        return;
    }

    if (state.m_power_state == hc::api::plug::State::PowerState::OFF_LOCKED) {
        get_logger().debug(
            "handle_power_on(): Power is already switching off!");
        return;
    }

    m_interface->on();

    state.m_power_state = hc::api::plug::State::PowerState::ON_LOCKED;
    m_cv.notify_all();

    get_logger().log("Power switched ON");
}

void Plug::handle_power_off(hc::api::plug::State& state) {
    if (state.m_power_state == hc::api::plug::State::PowerState::OFF ||
        state.m_power_state == hc::api::plug::State::PowerState::OFF_LOCKED) {
        get_logger().debug("handle_power_off(): Power already off!");
        return;
    }

    if (state.m_power_state == hc::api::plug::State::PowerState::ON_LOCKED) {
        get_logger().debug(
            "handle_power_off(): Power is already switching on!");
        return;
    }

    m_interface->off();

    state.m_power_state = hc::api::plug::State::PowerState::OFF_LOCKED;
    m_cv.notify_all();

    get_logger().log("Power switched OFF");
}