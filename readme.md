# Lab2QRCode-HttpService

## 依赖项

* [nlohmann-json](https://github.com/nlohmann/json)
* [zlib](https://github.com/madler/zlib)
* [asio](https://github.com/chriskohlhoff/asio)
* [spdlog](https://github.com/gabime/spdlog)

## 服务器API

### POST 获取最新版本号

`POST /update/check_version`

传入本地信息{当前版本，平台，频道}，获取最新版本号和相关标志

> Body 请求参数

```json
{
  "version": "1.0",
  "os-arch": "windows-x64",
  "channel": "stable"
}
```

#### 请求参数

| 名称        | 位置   | 类型     | 必选 | 说明                     |
|-----------|------|--------|----|------------------------|
| body      | body | object | 是  | none                   |
| » version | body | string | 是  | 本地版本号                  |
| » os-arch | body | string | 是  | 操作系统-架构                |
| » channel | body | string | 否  | 用户订阅的通道，留空或无法识别为stable |

---

> 返回示例

> 200 Response

```json
{
  "version": "1.0",
  "flags": 3
}
```

> 400 Response

```json
{
  "reason": "unknown arch"
}
```

#### 返回结果

| 状态码 | 状态码含义                                                            |
|-----|------------------------------------------------------------------|
| 200 | [OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)          |
| 400 | [Bad Request](https://tools.ietf.org/html/rfc7231#section-6.5.1) |

#### 返回数据结构

状态码 **200**

| 名称        | 类型      | 必选   | 约束   | 说明                                  |
|-----------|---------|------|------|-------------------------------------|
| » version | string  | true | none | 版本号                                 |
| » flags   | integer | true | none | bit flag[是否有可用更新/是否是大更新/是否是紧要的漏洞修复] |

状态码 **400**

| 名称       | 类型     | 必选   | 约束   | 说明     |
|----------|--------|------|------|--------|
| » reason | string | true | none | 请求失败原因 |

### POST 获取更新数据

`POST /update/fetch_latest`

> Body 请求参数

```json
{
  "os-arch": "string",
  "channel": "string"
}
```

#### 请求参数

| 名称        | 位置   | 类型     | 必选 | 说明                     |
|-----------|------|--------|----|------------------------|
| body      | body | object | 是  | none                   |
| » os-arch | body | string | 是  | 操作系统-架构                |
| » channel | body | string | 否  | 用户订阅的通道，留空或无法识别为stable |

---

> 返回示例

> 200 Response

```json
{
  "data": "string",
  "hash": "string"
}
```

> 400 Response

```json
{
  "reason": "unknown arch"
}
```

### 返回结果

| 状态码 | 状态码含义                                                            |
|-----|------------------------------------------------------------------|
| 200 | [OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)          |
| 400 | [Bad Request](https://tools.ietf.org/html/rfc7231#section-6.5.1) |

#### 返回数据结构

状态码 **200**

| 名称     | 类型     | 必选    | 约束   | 说明                |
|--------|--------|-------|------|-------------------|
| » data | string | true  | none | 经压缩和base64后的更新数据包 |
| » hash | string | false | none | 校验码（TODO）         |

状态码 **400**

| 名称       | 类型     | 必选   | 约束   | 说明     |
|----------|--------|------|------|--------|
| » reason | string | true | none | 请求失败原因 |
