# 构建、烧录与故障恢复

## 环境

- Windows 10/11 PowerShell
- CMake 3.25或更新版本
- Ninja
- Arm GNU Toolchain (`arm-none-eabi-gcc`)
- STM32CubeProgrammer 2.x
- ST-Link，连接 SWDIO、SWCLK、GND；目标板必须供电

已冻结的1.0.0构建环境记录在固件包的 `build-manifest.json` 中。

## 可重复构建

工程根目录执行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Configuration release -Clean
```

`VERSION`确定固件版本，`SOURCE_DATE_EPOCH`固定编译日期宏。输出文件位于 `build/release`，SHA256写入 `build-manifest.json`。

Debug构建：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Configuration debug -Clean
```

## 烧录

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\flash.ps1 -Configuration release
```

脚本会依次尝试1000、400和100 kHz SWD，并执行写入、校验和软件复位。普通烧录保留Bank 2配置日志。

如果板上NRST也连接到了ST-Link，可在普通连接失败时使用：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\flash.ps1 -Configuration release -ConnectionMode UnderReset
```

## 发布包

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-release.ps1
```

输出：

- `dist/watch_u575_astra-v1.0.0/`
- `dist/watch_u575_astra-v1.0.0.zip`
- ZIP的SHA256文件

## 上机检查

1. 开机动画结束并进入主表盘。
2. 四个按键功能正确。
3. 系统设置底部显示 `WDG OK`。
4. 设置15秒息屏并等待进入STOP2。
5. 使用四个按键分别唤醒，检查 `STOP2`计数增加。
6. 确认RTC、温湿度和MAX30102在唤醒后正常。
7. 验证闹钟能够从息屏状态唤醒。

## 常见故障

### No STM32 target found

重新插拔ST-Link和目标板电源，检查GND、SWDIO和SWCLK，降低SWD频率；连接NRST时使用`UnderReset`。

### 烧录成功但黑屏

先等待至少10秒，随后按任意键排除OLED处于Power Save。仍黑屏时重新上电并再次烧录Release固件。

### 周期性重启

进入系统设置检查是否显示`WDG RST`或`WDG ERR`。记录复位周期和`S:`后的最小栈余量，再停止继续测试。

### 清除全部设置

整片擦除会删除固件和Bank 2配置日志，仅在确认需要恢复出厂状态时通过STM32CubeProgrammer执行；擦除后必须重新烧录Release固件。
