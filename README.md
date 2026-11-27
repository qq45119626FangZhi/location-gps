# lacation-gps

GPS 农场畜牧定位项目。使用两款 STM32 芯片分别承担定位终端的数据采集与网关的数据传输，仓库包含两个子工程：

- `location-gps-control`：定位终端（采集端）
- `location-gps-lora-gateway`：LoRa 网关（转发端）

## 系统架构

终端采集芯片 UID、步数、经纬度等数据，打包为 JSON 后上报服务器。提供两种上报方案，通过编译期宏二选一：

- 方案①（当前启用）：4G 直传
  `control --USART3(AT)--> QS100 4G 模组 --TCP--> 115.120.239.161:23399`
- 方案②（预留）：LoRa + 以太网网关
  `control --SPI--> LLCC68(480MHz) --> gateway LLCC68 --SPI--> W5500 --TCP--> 192.168.45.36:666`

## 功能特性

### 定位终端 control（STM32F103C8T6，HAL 库）

- **GPS 定位**：USART2（9600bps）驱动 AT6558R，解析 `$GNRMC` NMEA 帧获取时间、有效位、经纬度与日期；`ddmm.mmmmm` 转换为度，UTC 时间 +8h 得北京时间；PB3 控制模块电源。
- **计步采集**：I2C1（100kHz）读取 DS3553 计步传感器（器件地址 `0x4F`，步数寄存器 `0xC4~0xC6`）。
- **低功耗**：LSE 32768Hz 驱动 RTC，采集完成后设置闹钟并进入 Standby 模式，定时唤醒后重新采集；唤醒时恢复 GPS 与 4G 模组供电。
- **设备标识**：读取 96 位芯片 UID，转为 24 位十六进制字符串作为 `uid`。
- **数据上报**：
  - ① USART3（9600bps）以 AT 指令驱动 QS100 4G 模组，通过 `AT+NSOCR / AT+NSOCO / AT+NSOSD` 建立 TCP 连接并上传 JSON（payload 以 ASCII 十六进制发送）。
  - ② SPI1（18MHz）驱动 LLCC68 LoRa 模块（SF9 / BW125kHz / CR4/5 / 480MHz / +17dBm），向网关发送 JSON 原始字节。
- **调试输出**：USART1（115200bps）printf 重定向，带文件名/行号的调试打印。

### 网关板 gateway（STM32F103ZET6，HAL 库）

- **LoRa 接收**：SPI1 驱动 LLCC68（参数与终端一致），上电后进入连续接收模式，接收终端上报的 JSON。
- **以太网转发**：SPI2 驱动 W5500，静态 IP `192.168.45.136`，以 TCP 客户端（socket0）将收到的数据透传至 `192.168.45.36:666`，发送后关闭连接。
- **调试输出**：USART1（115200bps）。
- 工程内已编译 W5500 的 HTTP Server / DHCP / DNS 组件，但当前应用未启用 HTTP 服务，仅使用裸 Socket 转发。

## 数据格式

上报数据为 cJSON 生成的无格式 JSON 字符串：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `uid` | string | 96 位芯片 UID 转换的 24 位十六进制 |
| `date` | string | 北京时间日期时间 |
| `step` | number | 计步传感器步数 |
| `lon` | number | 经度（度） |
| `lat` | number | 纬度（度） |

LoRa 链路直接发送 JSON 原始字节，无额外应用层帧头；LoRa 层启用 CRC 与显式头。

## 工程信息

- **MCU**：control 为 STM32F103C8T6（LQFP48），gateway 为 STM32F103ZET6（LQFP144）。
- **开发工具**：STM32CubeMX 6.10.0 生成初始化代码，Keil MDK-ARM V5（AC5 编译器）工程。
- **HAL 库**：STM32Cube FW_F1（`.ioc` 标注 V1.8.7）。
- **源码分层**：`Core`（CubeMX 生成）、`Drivers`（HAL 驱动）、`MDK-ARM/Source` 下的 `App`（应用）、`Com`（公共组件，如 cJSON、调试打印）、`Int`（模块驱动：AT6558R、DS3553、QS100、LLCC68、W5500）。

## 编译

使用 Keil uVision 分别打开以下工程编译：

- `location-gps-control/MDK-ARM/location-gps.uvprojx`
- `location-gps-lora-gateway/MDK-ARM/location-gps-lora-gateway.uvprojx`

## 当前状态

- 终端当前启用 4G 直传方案（`APP_WORK_NET_4G=1`），LoRa 上报路径为预留实现。
- GPS 采集目前使用写死的 `$GNRMC` 模拟串，真实 GPS 读取逻辑暂被注释（见 `location-gps-control/MDK-ARM/Source/App/App_Work.c`）。
- 网关 HTTP Server 代码已编入工程但未接入主流程，当前仅做 TCP 透传。

## 许可证

见 [LICENSE](LICENSE)。
