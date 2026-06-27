# LanternBox AI Services Layer

Version: v0.1  
Status: Draft  
Last Updated: 2026-06-28

---

## 一、定位

Services Layer 是 LanternBox AI 的本地业务数据访问层。

它不负责 AI 推理，也不负责 Pipeline 调度。

它负责从本地数据源中读取、整理和提供业务数据，例如：

- 应急指南
- 精选 Wiki
- Kiwix / ZIM
- 物资库存
- 成员信息
- 地图资源点
- 传感器数据
- 日志记录

Services 是 Pipeline 和底层数据之间的缓冲层。

---

## 二、为什么需要 Services Layer

当前部分数据准备逻辑仍散落在 routes.py 或 Pipeline Preload 中。

例如：

- 匹配触发规则
- 查找关联指南
- 查找关联 Wiki
- 读取物资库存
- 读取成员信息
- 读取地图或传感器数据

如果这些逻辑直接写在 routes.py 或 pipeline/preload.py 中，未来会导致：

- routes.py 再次变胖
- Pipeline 直接依赖数据细节
- Study / Knowledge / Emergency 模式重复读取同一类数据
- 后续更换数据源时需要到处修改

因此，数据访问必须收敛到 Services Layer。

---

## 三、总体关系

```text
routes.py
   ↓
Pipeline Builder
   ↓
Pipeline Preload
   ↓
Services Layer
   ↓
Local Data Sources