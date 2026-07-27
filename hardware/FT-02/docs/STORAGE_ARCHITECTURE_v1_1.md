# FT-02 Storage Architecture v1.1

## 定位

`FT02Storage` 是 FT-02 唯一的 SD 存储底层。地图、日志、录音和未来知识资源模块只通过公共接口访问文件，不直接配置 SDMMC。

## 当前生产 profile

```text
SDMMC 1-bit @ 5 MHz
CLK = GPIO35
CMD = GPIO2
D0  = GPIO1
```

配置集中在：

```text
lib/FT02Storage/src/FT02_StorageConfig.h
```

## 模块边界

`FT02Storage` 负责：

- SDMMC host 与 slot 配置
- FAT 文件系统挂载
- 卡类型与容量统计
- 文件存在性检查
- 只读文件打开
- 追加文本行
- 存储状态与错误状态

业务模块负责：

- 地图格式解析与图块绘制
- 日志内容组织
- 录音文件格式
- UI 状态显示

业务模块不得：

- 再次初始化或卸载 SDMMC
- 自行定义 SD GPIO
- 自行改变 SD 时钟
- 在启动阶段执行存储压测
- 在挂载失败后静默切换 SPI

## 测试代码隔离原则

生产固件不包含大文件压测、CRC 探针、原始扇区测试或排列测试。这些测试应保存在独立诊断工程中，验证通过后只把最终配置和必要修复合回主线。

## 未来 4-bit

4-bit 必须作为独立实验 profile 开发。只有完成冷启动、原始扇区、FAT 大文件、CRC、地图读取和主固件集成验收后，才能替换当前生产 profile。
