# FT-02 v2.58 优化 20 px 字体正式版

本版基于 v2.57 实机对比结果，正式采用对比页中的 B 方案，并删除临时字体测试页。

## 字体调整

- 全局 20 px 常规中文改为优化栅格版本。
- 使用 Noto Sans CJK SC 20 px 灰度渲染，再以阈值 170 转换为 1-bit 位图。
- 完整覆盖 22,367 个字符，不是测试字符子集。
- 主要应用于知识库 Card 说明、分类摘要、定位记录辅助文字和相关小字号区域。
- 24 px 常规正文与 24 px 粗体标题保持不变。

## 定位记录操作

```text
ENTER：开始或停止 Session
P：手动记录当前位置
A：开启或关闭 30 秒自动轨迹
R：重新连接 GNSS
B：返回首页，记录继续在后台运行
```

## 保持不变

- GNSS：RX=GPIO39、TX=GPIO38、38400 baud。
- 定位记录退出页面后继续后台运行。
- 定位记录中间区域局部刷新。
- 知识库双向单次刷新和防变淡流程。
- 现场行动手册 SD 数据格式与第一阶段 42 张行动卡。
- 地图 PBF UI A3.13。
- SD FSPI Mode 3 @ 40 MHz。
- CardKB2：SDA=GPIO47、SCL=GPIO21。

## 重新生成优化字库

开发环境具备 Pillow、fontTools 和 Noto Sans CJK SC 时，可运行：

```bash
python3 tools/generate_ft02_global_cjk20_optimized_font.py
```

生成工具只输出嵌入式 1-bit 字形数据，不复制或分发源字体文件。
