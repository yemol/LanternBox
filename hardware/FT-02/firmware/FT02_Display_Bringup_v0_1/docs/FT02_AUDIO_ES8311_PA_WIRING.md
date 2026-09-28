# FT-02 ES8311 / NS4150B 音频接线

唯一新增线：

```text
小声卡 SD / CTRL  -> ESP32-S3 GPIO4
```

建议增加：

```text
SD / CTRL -- 10 kΩ -- GND
```

目的：ESP32-S3 复位和启动早期 GPIO 尚未接管前，也把 NS4150B 保持在硬件关断状态。

逻辑：
- GPIO4 LOW：NS4150B shutdown，待机/录音
- GPIO4 HIGH：NS4150B enable，仅播放

不要把 `SD` 与 I2S 的 `SDIN/SDOUT` 混淆；这里的 `SD` 是功放 shutdown/control。
