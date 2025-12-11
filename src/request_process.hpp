#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <map>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "heterogeneous.hpp"

namespace l2q_http{
// 状态码枚举
enum struct status_code : std::uint16_t{
	ok = 200,
	bad_request = 400,
	unauthorized = 401,
	forbidden = 403,
	not_found = 404,
	method_not_allowed = 405,
	not_acceptable = 406,
	internal_server_error = 500,
};

enum struct http_method {
	get,
	post,
	put,
	del, // delete 是 C++ 关键字，改用 del 或 delete_method
	head,
	options,
	patch,
	unknown
};

// 请求处理结果
struct request_result{
	nlohmann::json data{};
	status_code code{status_code::ok};
};


struct request_args{
	http_method method{};
	nlohmann::json body{};
};

class request_handler{
	using logic_func = std::function<request_result(request_args&&)>;

public:
	request_handler() = default;

	/**
	* @brief 注册路由
	* @param path: API路径 (e.g., "/api/login")
	* @param handler: 处理逻辑
	*/
	template <std::invocable<request_args&&> Fn>
		requires (std::constructible_from<logic_func, Fn&&>)
	bool route(std::string_view path, Fn&& handler){
		return routes_.try_emplace(path, std::forward<Fn>(handler)).second;
	}

	/**
	 * @brief 核心处理函数
	 * @param path 请求路径
	 * @param body_json 请求体 (已解析为 json 对象)
	 * @return 状态码和响应数据
	 */
	[[nodiscard]] request_result process(std::string_view path, request_args&& request) const{
		if(auto it = routes_.find(path); it != routes_.end()){
			try{
				spdlog::debug("processing logic for path: {}", path);
				return it->second(std::move(request));
			} catch(const std::exception& e){
				spdlog::error("logic error at {}: {}", path, e.what());
				return request_result{e.what(), status_code::internal_server_error};
			}
		}

		spdlog::warn("path not found: {}", path);
		return request_result{"resource not found", status_code::not_found};
	}

private:
	string_hash_map<logic_func> routes_;
};
} // namespace l2q_http
