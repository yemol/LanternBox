# FT-02 提交 / 发布检查清单

提交 Git 或打包新版本前至少确认：

- [ ] `src/FT02_BuildInfo.h` 版本号已更新
- [ ] `VERSION.txt` 与固件版本一致
- [ ] `python3 tools/check_firmware_version.py` PASS
- [ ] `pio run` 编译通过
- [ ] `boards/ft02-esp32-s3-n16r8.json` 仍存在
- [ ] 没有重新引入 Core 直接 GNSS / Compass / SX126x 硬件访问
- [ ] LR01 Host Protocol 变更已同步到 `docs/LR01_HOST_PROTOCOL_A2.md`
- [ ] 新增用户操作已同步到 `docs/USER_GUIDE.md`
- [ ] 设备内帮助页与实际快捷键一致
- [ ] 地图、定位、通讯、系统自检没有明显回归
- [ ] 如果修改罗盘校准，至少测试 START / READY / SAVE / CANCEL / RESET 二次确认
- [ ] 重新生成工程 checksum 文件

建议提交信息格式：

```text
FT02 vX.XX: <主要功能/修复>
```
