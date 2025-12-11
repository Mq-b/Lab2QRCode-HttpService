#include <asio.hpp>

#include <spdlog/sinks/stdout_color_sinks.h> // 引入控制台彩色输出 sink
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include "src/http_server_wrapper.hpp"


void setup_logging() {
    if (std::filesystem::exists("Log")) {
        std::filesystem::create_directory("Log");
    }
    auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("log/server.log", 0, 0);
    file_sink->set_level(spdlog::level::debug);
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [thread %t] [%oms] [%l] %v");

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    console_sink->set_pattern("%^[%Y-%m-%d %H:%M:%S] [thread %t] [%oms] [%l] %v%$");

    auto logger = std::make_shared<spdlog::logger>("multi_sink", spdlog::sinks_init_list{file_sink, console_sink});
    spdlog::register_logger(logger);

    spdlog::set_default_logger(logger);
    spdlog::flush_on(spdlog::level::debug);
}
// --- 用户业务代码示例 ---

int main(int argc, char* argv[]) {
    setup_logging();
    try {
        std::uint16_t port = 10000;

        if (argc >= 2) {
            std::string_view port_arg = argv[1];
            uint16_t parsed_port = 0;

            auto [ptr, ec] = std::from_chars(
                port_arg.data(),
                port_arg.data() + port_arg.size(),
                parsed_port
            );

            if (ec == std::errc() && ptr == port_arg.data() + port_arg.size()) {
                port = parsed_port;
            } else {
                spdlog::warn("invalid port argument '{}', using default port {}", port_arg, port);
            }
        }

        asio::io_context io_context(1); // 单线程模型

        asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto){
            spdlog::info("shutdown signal received");
            io_context.stop();
        });

        l2q_http::http_server server(io_context, port);
        server.route("/api/def", [](l2q_http::request_args&& args){
            auto v = args.body;
            return l2q_http::request_result{};
        });
        server.start();

        spdlog::info("running on: {} ...", port);
        io_context.run();

    } catch (const std::exception& e) {
        spdlog::critical("unhandled exception in main: {}", e.what());
        return 1;
    }
}