#include "config.h"
#include "driver/rpi_z_driver.h"
#include "driver/test_driver.h"
#include "plug.h"

#include <homecontroller/util/string.h>

#include <csignal>
#include <thread>

std::vector<std::unique_ptr<Plug>> g_plugs;

struct CommandLineArgs {
    std::string m_config_path;
};

CommandLineArgs read_args(const hc::util::Logger& main_logger, int argc,
                          char* argv[]) {
    CommandLineArgs args;
    args.m_config_path = "conf/conf.json";

    static std::map<std::string, std::function<void(const std::string&)>>
        parse_map = {{"--conf-path", [&](const std::string& val) {
                          args.m_config_path = val;
                      }}};

    for (int i = 1; i < argc; i++) {
        std::string arg_str(argv[i]);

        std::vector<std::string> arg_str_split =
            hc::util::str::split(arg_str, '=');
        std::string arg = arg_str_split[0];
        std::string val = arg_str_split.size() > 1 ? arg_str_split[1] : "";

        auto mit = parse_map.find(arg);
        if (mit == parse_map.end()) {
            main_logger.warn("unknown argument \"" + arg + "\"");
            continue;
        }

        mit->second(val);
    }

    std::string args_str = "\n\tconf-path=" + args.m_config_path;

    main_logger.log("Starting with arguments: " + args_str);

    return args;
}

Result<std::shared_ptr<Driver>> get_driver(const std::string& name) {
    static std::map<std::string, std::function<std::shared_ptr<Driver>()>>
        str_to_driver_map = {{"RPI_Z", []() { return RPiZDriver::create(); }},
                             {"TEST", []() { return TestDriver::create(); }}};

    auto mit = str_to_driver_map.find(name);
    if (mit == str_to_driver_map.end()) {
        return Result<std::shared_ptr<Driver>>::Err(
            Error(__func__, "invalid driver name"));
    }

    return Result<std::shared_ptr<Driver>>::Ok(mit->second());
}

int main(int argc, char* argv[]) {
    hc::util::Logger main_logger = hc::util::Logger("Main");

    CommandLineArgs args = read_args(main_logger, argc, argv);

    main_logger.log("RGBLights for HomeController v1.0.0");
    main_logger.log("Created by Josh Dittmer");

    // load config
    Config config = Config(args.m_config_path);
    Result<Config::Values> config_res = config.load();

    if (!config_res.is_ok()) {
        main_logger.fatal("Failed to load config: " + config_res.unwrap_err());
        main_logger.fatal("Plug exited with non-zero status code");
        return -1;
    }

    Config::Values config_values = config_res.unwrap();

    hc::util::Logger::set_log_level(hc::util::Logger::string_to_log_level(
        main_logger, config_values.m_log_level_str));

    Result<std::shared_ptr<Driver>> driver_res =
        get_driver(config_values.m_driver_str);
    if (!driver_res.is_ok()) {
        main_logger.error("Failed to get driver: " + driver_res.unwrap_err());
        main_logger.fatal("Plug exited with non-zero status code");
        return -1;
    }

    std::shared_ptr<Driver> driver = driver_res.unwrap();
    if (!driver->init()) {
        main_logger.error("Failed to start driver!");
        main_logger.fatal("Plug exited with non-zero status code");
        return -1;
    }

    std::vector<std::thread> threads;
    for (const Plug::Config& pc : config_values.m_plugs) {
        std::unique_ptr<Plug>& plug_ptr =
            g_plugs.emplace_back(std::make_unique<Plug>(pc));

        threads.emplace_back(&Plug::init, plug_ptr.get(), driver);
    }

    std::signal(SIGINT, [](int s) {
        for (const auto& p : g_plugs) {
            p->shutdown();
        }
    });

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    driver->shutdown();

    main_logger.log("Plug stopped, exiting gracefully");

    return 0;
}