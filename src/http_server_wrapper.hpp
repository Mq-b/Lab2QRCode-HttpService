#include <asio.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <string>
#include <string_view>
#include <memory>
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include "request_process.hpp"

// 使用 asio 的命名空间简化代码

namespace l2q_http {
    using asio::ip::tcp;
    using asio::awaitable;
    using asio::co_spawn;
    using asio::detached;
    using asio::use_awaitable;

    //TODO 简化?
    constexpr http_method string_to_method(std::string_view method_str) noexcept {
        if (method_str == "GET") return http_method::get;
        if (method_str == "POST") return http_method::post;
        if (method_str == "PUT") return http_method::put;
        if (method_str == "DELETE") return http_method::del;
        if (method_str == "HEAD") return http_method::head;
        if (method_str == "OPTIONS") return http_method::options;
        if (method_str == "PATCH") return http_method::patch;
        return http_method::unknown;
    }

    /**
     * @brief 简单的 HTTP 会话处理逻辑
     * 注意：为了保持示例简单，这里手动处理了 HTTP 协议字符串。
     * 在生产环境中，建议结合 boost::beast 或其他 http parser。
     */
    class http_session {
    public:
        // 移动构造函数，确保 socket 所有权转移
        explicit http_session(tcp::socket socket, const request_handler& handler) noexcept
            : socket_(std::move(socket)), handler_(std::addressof(handler)) {}

        // 禁止拷贝
        http_session(const http_session&) = delete;
        http_session& operator=(const http_session&) = delete;

        // 默认移动语义
        http_session(http_session&&) noexcept = default;
        http_session& operator=(http_session&&) noexcept = default;

        ~http_session() noexcept {
            // socket 会在析构时自动关闭，但在 log 中记录一下
            // 实际析构时不应抛出异常，所以仅做简单处理或忽略
        }

        /**
         * @brief 异步处理客户端请求的主协程
         */
        awaitable<void> process() {
            try {
                auto remote_ep = socket_.remote_endpoint();
                spdlog::debug("new connection from {}:{}", remote_ep.address().to_string(), remote_ep.port());

                // 1. 读取 HTTP 请求头 (直到 \r\n\r\n)
                asio::streambuf buffer;
                std::size_t bytes_transferred = co_await asio::async_read_until(
                    socket_, 
                    buffer, 
                    "\r\n\r\n", 
                    use_awaitable
                );

                const std::string request_str{
                    std::istreambuf_iterator<char>(&buffer), 
                    std::istreambuf_iterator<char>()
                };

                // 简单的解析第一行 (Method Path Version)
                std::string method, path, version;
                std::istringstream request_stream(request_str);
                request_stream >> method >> path >> version;

                // 2. 简单的 Body 解析 (在 \r\n\r\n 之后的部分)
                // 注意：真实场景需要解析 Content-Length
                std::string body_str;
                std::size_t header_end = request_str.find("\r\n\r\n");
                if (header_end != std::string::npos) {
                    body_str = request_str.substr(header_end + 4);
                }


                request_result result;
                {
                    // 3. 转换为 JSON 并调用 Handler

                    try {
                        nlohmann::json req_json;
                        if (!body_str.empty()) {
                            req_json = nlohmann::json::parse(body_str);
                        }
                        // 调用核心业务逻辑
                        result = handler_->process(path, request_args{
                            .method = string_to_method(method),
                            .body = std::move(req_json)
                        });
                    } catch (const nlohmann::json::parse_error& e) {
                        // JSON 格式错误处理
                        result = request_result(
                            "invalid json format", status_code::bad_request
                        );
                    }
                }

                // 4. 序列化响应
                const auto response_body = result.data.dump();
                const auto status_str = [](status_code code) -> std::string {
                    // 简单的状态码转字符串 (TODO 看需不需要使用magic enum)
                    if (code == status_code::ok) return "200 OK";
                    if (code == status_code::not_found) return "404 Not Found";
                    return std::format("{}", static_cast<int>(code)) + " Error";
                }(result.code);

                const auto response = fmt::format(
                    "HTTP/1.1 {}\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: {}\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "{}",
                    status_str, response_body.size(), response_body
                );

                // 3. 发送响应
                co_await asio::async_write(
                    socket_, 
                    asio::buffer(response), 
                    use_awaitable
                );

                spdlog::debug("response sent to {}", remote_ep.address().to_string());

                // 4. 优雅关闭 socket
                // 注意：为了简化，这里直接关闭，不支持 Keep-Alive
                socket_.shutdown(tcp::socket::shutdown_both);

            } catch (const std::exception& e) {
                spdlog::error("session exception: {}", e.what());
            }
        }

    private:
        const request_handler* handler_;
        tcp::socket socket_;
    };


    /**
     * @brief HTTP 服务器包装器
     */
    class http_server {
    public:
        explicit http_server(asio::io_context& ctx, std::uint16_t port) noexcept
            : io_context_(ctx), port_(port) {}

        /**
         * @brief 启动监听协程
         */
        void start() {
            // 将监听协程放入 io_context 执行
            co_spawn(io_context_, listener(), detached);
        }

        template <typename Fn>
            requires (std::is_invocable_r_v<request_result, Fn, request_args&&>)
        bool route(std::string_view path, Fn&& handler) {
            return handler_.route(path, std::forward<Fn>(handler));
        }

    private:
        /**
         * @brief 监听并接受连接的协程
         */
        awaitable<void> listener() {
            auto executor = co_await asio::this_coro::executor;
            
            try {
                tcp::acceptor acceptor(executor, {tcp::v4(), port_});
                // 设置端口复用
                acceptor.set_option(tcp::acceptor::reuse_address(true));

                spdlog::info("server started listening on port {}", port_);

                while (true) {
                    // 等待新的连接
                    tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
                    
                    // 为每个新连接生成一个新的协程进行处理
                    // 使用 std::move 将 socket 所有权转移给 session
                    auto session = http_session(std::move(socket), handler_);
                    
                    // 使用 co_spawn 启动会话协程，并将其与当前上下文分离(detached)
                    // 注意：这里需要将 session 移动进 lambda 或者由 session 类自行管理生命周期
                    // 简单的做法是利用 C++20 协程传值保存临时对象，或者使用 shared_ptr
                    co_spawn(
                        executor,
                        [sess = std::move(session)]() mutable -> awaitable<void> {
                            co_await sess.process();
                        },
                        detached
                    );
                }
            } catch (const std::exception& e) {
                spdlog::critical("server listener failed: {}", e.what());
            }
        }

        request_handler handler_{};
        asio::io_context& io_context_;
        std::uint16_t port_;
    };

} // namespace simple_web
