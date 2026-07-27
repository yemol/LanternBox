# FT-02 Storage Architecture v1.0

## 模块位置

```text
lib/FT02Storage/src/
├── FT02_Storage.h
├── FT02_Storage.cpp
└── FT02_StorageConfig.h
```

## 职责

`FT02Storage` 独占以下职责：

- SDMMC 引脚和频率配置；
- SD 卡初始化和 FAT 挂载；
- 卡类型、容量、已用和剩余空间统计；
- 启动读写门槛；
- 文件存在检查和只读文件打开；
- 存储状态与错误状态输出。

当前地图模块只调用：

```cpp
FT02_StorageIsReady();
FT02_StorageFileExists(path);
FT02_StorageOpenRead(path);
```

主程序和地图模块中不允许直接出现 `SD_MMC` 调用。

## 当前 profile

```text
SDMMC 1-bit @ 10 MHz
CLK=GPIO15
CMD=GPIO16
D0=GPIO14
```

未来 4-bit 必须新增独立实验 profile，不得修改当前生产候选 profile。
