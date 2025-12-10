#include <asio.hpp>

#include <spdlog/sinks/stdout_color_sinks.h> // 引入控制台彩色输出 sink
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <iostream>
#include <string>
#include <utility>

using asio::ip::tcp;

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    HttpSession(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        // 获取远程断点的地址和端口，用于日志
        try {
            remote_endpoint_ = socket_.remote_endpoint().address().to_string()
                             + ":" + std::to_string(socket_.remote_endpoint().port());
        } catch (...) {
            remote_endpoint_ = "Unknown";
        }

        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());

        socket_.async_read_some(asio::buffer(data_, max_length),
            [this, self](std::error_code ec, std::size_t length) {
                if (!ec) {
                    // 使用 {} 占位符进行格式化
                    spdlog::info("[{}] 收到请求: {} bytes", remote_endpoint_, length);

                    // (可选) 如果想打印具体的请求头，可以使用 spdlog::debug
                    // spdlog::debug("Payload: {}", std::string(data_, length));

                    do_write();
                } else {
                    // 处理连接关闭或其他错误
                    if (ec != asio::error::eof) {
                        spdlog::warn("[{}] 读取错误: {}", remote_endpoint_, ec.message());
                    }
                }
            });
    }

    void do_write() {
        auto self(shared_from_this());

        std::string response_str =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 13\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Hello, World!";

        cache_ = response_str;
        // std::vector response{response_str.begin(), response_str.end()};

        asio::async_write(socket_, asio::buffer(cache_),
            [this, self/*, buf = std::move(response)*/](std::error_code ec, std::size_t length) {
                if (!ec) {
                    spdlog::info("[{}] 响应发送成功 ({} bytes)", remote_endpoint_, length);
                } else {
                    spdlog::error("[{}] 发送错误: {}", remote_endpoint_, ec.message());
                }
            });
    }

    tcp::socket socket_;
    std::string remote_endpoint_; // 缓存远程地址用于日志
    enum { max_length = 1024 };
    char data_[max_length];
    std::string cache_{};
};

class HttpServer {
public:
    HttpServer(asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {

        spdlog::info("正在启动服务器，监听端口: {}", port);
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](std::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<HttpSession>(std::move(socket))->start();
                } else {
                    spdlog::error("Accept 失败: {}", ec.message());
                }

                do_accept();
            });
    }

    tcp::acceptor acceptor_;
};

void setup_logging() {
    try {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        console_sink->set_pattern("[%H:%M:%S] [%^%l%$] %v"); // 简短格式

        auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("logs/server.log", 0, 0);
        file_sink->set_level(spdlog::level::debug);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v"); // 详细格式

        std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};

        auto logger = std::make_shared<spdlog::logger>("server_logger", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::info);

        spdlog::set_default_logger(logger);

    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "日志初始化失败: " << ex.what() << std::endl;
    }
}

int main() {
    setup_logging();

    try {
        asio::io_context io_context;

        HttpServer server(io_context, 10000);

        spdlog::info("服务器已就绪: on port:10000");

        io_context.run();
    }
    catch (std::exception& e) {
        spdlog::critical("致命异常: {}", e.what());
    }
}
