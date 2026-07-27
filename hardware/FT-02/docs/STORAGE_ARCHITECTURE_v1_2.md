# FT-02 Storage Architecture v1.2

## 职责边界

底层 SD 操作集中在：

```text
lib/FT02Storage/src/
├── FT02_Storage.h
├── FT02_Storage.cpp
└── FT02_StorageConfig.h
```

地图、UI、日志和后续录音模块不得直接调用 `esp_vfs_fat_sdmmc_mount()`、`sdmmc_host_*` 或另行初始化 SD。

## 当前后端

- 原生 ESP-IDF VFS SDMMC
- 1-bit
- 5 MHz
- CLK GPIO35
- CMD GPIO2
- D0 GPIO1

## 正式启动流程

1. 输入模块初始化。
2. `FT02_StorageBegin()` 执行一次挂载。
3. 墨水屏初始化。
4. 首页读取存储状态与剩余空间。
5. 进入地图页时按需读取地图配置和图块。

正式启动流程不执行大文件压测、CRC 探针、重复挂载或测试文件写入。

## 未来扩展

4-bit 必须通过独立实验 profile 和完整稳定性验收后才能替换当前基线。
