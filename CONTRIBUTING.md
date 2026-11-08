# 贡献指南

## 开发基线

- 不修改 `docs/HARDWARE.md` 中的引脚契约，除非硬件也同时改版。
- UI 不直接访问 HAL；按 `UI -> Services -> BSP/HAL` 的依赖方向开发。
- 任务中不使用长时间忙等待，优先使用队列、任务通知或状态机。
- 禁止在任务运行期间动态创建 FreeRTOS 对象。
- 保留 GPL-3.0 及第三方版权声明。

## 提交前检查

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Configuration debug -Clean
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Configuration release -Clean
```

硬件相关变更还应完成对应功能页、息屏唤醒与看门狗状态的实机验证。
