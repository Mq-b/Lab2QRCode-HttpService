#include "compress.h"

#include <vector>
#include <limits>
#include <limits>
#include <utility>
#include <algorithm>
#include <zlib.h>

#if defined(__has_cpp_attribute) && __has_cpp_attribute(indeterminate)
#  define BUFFER_INDETERMINATE [[indeterminate]]
#else
#  define BUFFER_INDETERMINATE
#endif

namespace {
    // 内部辅助：自动管理 z_stream 资源的 RAII 包装器
    struct ZStreamGuard {
        z_stream& zs;
        bool is_compress; // true for deflate, false for inflate

        ZStreamGuard(z_stream& stream, bool compress) : zs(stream), is_compress(compress) {}
        ~ZStreamGuard() {
            if (is_compress) {
                deflateEnd(&zs);
            } else {
                inflateEnd(&zs);
            }
        }
    };
}

std::optional<std::string> gzip_compress(std::string_view data, int level) noexcept try{
    if (data.empty()) {
        return std::string{};
    }

    // 限制压缩级别范围
    if (level != -1) {
        level = std::clamp(level, 0, 9);
    }

    z_stream zs{};

    // 15 + 16 启用 Gzip 头部处理
    if (deflateInit2(&zs, level, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return std::nullopt;
    }

    ZStreamGuard guard(zs, true);

    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
    // zlib 使用 uInt，确保 size 不溢出
    if (data.size() > std::numeric_limits<uInt>::max()) {
         return std::nullopt;
    }
    zs.avail_in = static_cast<uInt>(data.size());

    int ret;
    char buffer[32 * 1024] BUFFER_INDETERMINATE;
    std::string compressed_data;

    // 预估大小：通常压缩后会变小，但对于极小数据或随机数据可能会变大。
    // 保守估计预分配一部分，避免初期多次 realloc
    compressed_data.reserve(data.size() / 2);

    do {
        zs.next_out = reinterpret_cast<Bytef*>(buffer);
        zs.avail_out = sizeof(buffer);

        ret = deflate(&zs, Z_FINISH);

        if (compressed_data.size() < zs.total_out) {
            // 计算本次输出产生的数据量
            // total_out 是累计值，减去当前字符串长度即为新增量
            // 或者使用 (sizeof(buffer) - zs.avail_out)
            std::size_t size_produced = sizeof(buffer) - zs.avail_out;
            compressed_data.append(buffer, size_produced);
        }
    } while (ret == Z_OK);

    if (ret != Z_STREAM_END) {
        return std::nullopt;
    }

    return compressed_data;
} catch(...){
    return std::nullopt;
}

std::optional<std::string> gzip_decompress(std::string_view compressed_data) noexcept try{
    if (compressed_data.empty()) {
        return std::string{};
    }

    z_stream zs{};
    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed_data.data()));

    if (compressed_data.size() > std::numeric_limits<uInt>::max()) {
        return std::nullopt;
    }
    zs.avail_in = static_cast<uInt>(compressed_data.size());

    // 15 + 16 自动检测 zlib 或 gzip 头部
    if (inflateInit2(&zs, 15 | 16) != Z_OK) {
        return std::nullopt;
    }

    ZStreamGuard guard(zs, false); // RAII 自动调用 inflateEnd

    int ret;
    char buffer[32 * 1024] BUFFER_INDETERMINATE;
    std::string out_string;

    // 启发式预分配：假设压缩比为 2:1 到 3:1 之间
    // 如果文件很大，这可以显著减少 realloc
    // 注意检查 max_size 以防溢出
    try {
        out_string.reserve(std::min<size_t>(compressed_data.size() * 3, 1024 * 1024 * 100));
    } catch (...) {
        // 内存不足时不强求 reserve，继续尝试
    }

    do {
        zs.next_out = reinterpret_cast<Bytef*>(buffer);
        zs.avail_out = sizeof(buffer);

        ret = inflate(&zs, Z_NO_FLUSH);

        switch (ret) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                return std::nullopt;
            default: break;
        }

        std::size_t size_produced = sizeof(buffer) - zs.avail_out;
        out_string.append(buffer, size_produced);

    } while (ret != Z_STREAM_END);

    return out_string;
} catch(...){
    return std::nullopt;
}
