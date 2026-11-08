# GitHub 上传清单

## 首次上传

先在 GitHub 创建一个空仓库，名称使用
`AstraPulse--Low-Power-Watch-Based-on-FreeRTOS`（GitHub 仓库名不允许空格），不要勾选自动创建
README、`.gitignore` 或 License。然后在本项目根目录执行：

```powershell
git init -b main
git add .
git status
git commit -m "release: AstraPulse 1.0.0"
git remote add origin https://github.com/<your-account>/AstraPulse--Low-Power-Watch-Based-on-FreeRTOS.git
git push -u origin main
```

## 发布 1.0.0

```powershell
git tag -a v1.0.0 -m "AstraPulse 1.0.0"
git push origin v1.0.0
```

在 GitHub Releases 中以 `v1.0.0` 创建发布，说明内容可来自
`docs/RELEASE_NOTES.md`，并上传：

- `dist/watch_u575_astra-v1.0.0.zip`
- `dist/watch_u575_astra-v1.0.0.zip.sha256.txt`

`build/`、`dist/` 和未使用的 Astra 历史副本已被 `.gitignore` 排除，不会进入源码仓库。

## 建议的仓库信息

- Description: `AstraPulse -- Low-Power Watch Based on FreeRTOS`
- Topics: `stm32`, `stm32u5`, `freertos`, `embedded`, `smartwatch`, `ssd1306`,
  `max30102`, `low-power`, `stop2`, `astra-ui`
- License: `GPL-3.0`

首次推送后请在 GitHub 页面检查 README 中的 PCB/原理图、License 识别和
Releases 附件是否正常显示。
