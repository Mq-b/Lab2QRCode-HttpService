#pragma once

#include <string>
#include <string_view>
#include <optional>

/**
 * @brief 使用 Gzip 格式压缩数据。
 *
 * @param data 要压缩的原始数据 (string_view 避免拷贝)。
 * @param level 压缩级别 (0-9)，默认为 -1 (Z_DEFAULT_COMPRESSION)。
 * 1 为最快压缩，9 为最佳压缩。
 * @return std::optional<std::string> 成功时返回包含压缩数据的字符串，失败返回 std::nullopt。
 */
[[nodiscard]]
std::optional<std::string> gzip_compress(std::string_view data, int level = -1) noexcept;

/**
 * @brief 解压 Gzip 格式的数据。
 *
 * @param compressed_data Gzip 格式的压缩数据 (string_view 避免拷贝)。
 * @return std::optional<std::string> 成功时返回解压后的原始字符串，失败返回 std::nullopt。
 */
[[nodiscard]]
std::optional<std::string> gzip_decompress(std::string_view compressed_data) noexcept;
