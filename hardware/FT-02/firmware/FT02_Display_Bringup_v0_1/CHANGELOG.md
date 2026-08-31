# Production lineage

## v2.75k57

- 通用 EPD partial hot-idle 从 2500 ms 缩短到 350 ms。
- Input stable-release 保持 48 ms；same-key repeat 保持 180 ms。
- 不改变音频、Reader clean-slate、音量安全刷新和 A18 音量曲线。

## v2.75k56

- 1/10 用户播放增益从 0.20x 调整到 0.10x。
- 后续每档增量保持不变，最终表：`0.10, 0.18, 0.30, 0.46, 0.60, 0.75, 0.90, 1.10, 1.35, 1.60`。

## v2.75k55

- 用户主动调音量时恢复屏幕音量显示。
- 快速连续音量键合并 220 ms；刷新期间冻结 PCM、关闭 PA，刷新后从同一 PCM 位置恢复。

## v2.75k54

- Field Library Reader 每页清理并重绘 `x=0,y=86,w=800,h=394`，解决旧画面残留。
- SELECT/BACK/HELP 采用稳定释放后重新武装的一次性输入策略。

## v2.75k50

- 播放页固定显示总时长，不再刷新播放已用时间。
- 播放期间无自动 EPD timer refresh。
- 保留 PSRAM clean playback、Full-TX、RX suspend、46.4 ms stop fade 和 DAC mute → PA shutdown → DAC power-down。
