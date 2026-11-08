# Third-party software

## oled-ui-astra

- Upstream: https://github.com/AstraThreshold/oled-ui-astra
- Baseline: `v0.0.2-alpha`
- License: GPL-3.0 (the upstream license is retained at
  `ThirdParty/oled-ui-astra-0.0.2-alpha/LICENSE`; the project-level copy is
  `LICENSE`)
- Local integration: only the Astra UI core, HAL interface, custom font, and
  u8g2 sources are compiled. The STM32F103 example, startup code, and demo
  application are excluded.
- Local changes: owned launcher objects, explicit four-key navigation, removal
  of demo autoplay/blocking page transitions, and STM32U575 display HAL.

STM32 HAL/CMSIS and FreeRTOS keep their upstream license files inside their
respective directories.

The Astra snapshot contains u8g2 sources under their BSD-style license. The
copyright, redistribution conditions and disclaimer remain in the source
headers.

## WenQuanYi Bitmap Song

The menu-specific 12 x 12 Chinese glyph subset is generated from WenQuanYi
Bitmap Song 9pt. The source BDF identifies the font as GPL-2.0 with a font
embedding exception. Only glyphs listed in `Assets/fonts/watch_chinese.map`
are embedded in the firmware.
