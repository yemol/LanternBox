# 构建与发布

## 固定 Arduino 配置

- Board：已验证的 M5Stack Cardputer-Adv
- ESP32 Core：M5Stack 3.2.2 基线
- Flash Size：`8MB`
- Partition Scheme：`8M with spiffs (3MB APP/1.5MB SPIFFS)`

如果编译输出仍显示：

```text
Maximum is 1310720 bytes
```

说明分区方案回到了旧的 1.25MiB APP 配置，本次编译无效。

## 容量门禁

每个正式版本必须保存真实 Arduino IDE 输出：

```text
Sketch uses <实际字节> bytes (...) of program storage space. Maximum is <上限> bytes.
Global variables use <实际字节> bytes (...) of dynamic memory...
```

发布要求：

1. 编译成功。
2. 完整链接成功。
3. `Maximum` 不是 1,310,720 字节。
4. Sketch 小于实际 Maximum。
5. 建议 Sketch 不超过实际 Maximum 的 80%。
6. RAM 保留足够余量，特别关注 50 条消息与未来中文输入法缓存。
7. `tools/check_build_log.py` 和 `tools/check_size.py` 只作为辅助，不能替代真实板卡编译。

## 密钥准备

```bash
cp Ft01Secrets.example.h Ft01Secrets.h
```

随后填写真实频道材料。`Ft01Secrets.h` 只存在于开发机，不进入 Git、发布 ZIP 或日志。

## 发布步骤

1. 更新 `VERSION` 为新版本号。
2. 完成 Arduino 真实编译并保存输出。
3. 运行 [测试清单](TESTING.md)。
4. 确认 `.gitignore` 生效，仓库没有真实密钥。
5. 检查不包含 `.DS_Store`、`._*`、构建缓存和临时文件。
6. 更新 `CHANGELOG.md`，只写最终变化和验收结果。
7. 使用中文 Git 提交信息，Git Tag 使用英文版本号。

推荐提交信息：

```text
整理 FT-01 v0.5.2e 文档并固定后台通信基线
```

推荐 Tag：

```text
ft01-v0.5.2e
```

## 发布记录模板

```text
版本：
日期：
Board/Core：
Flash Size：
Partition Scheme：
Sketch uses：
Maximum：
Global variables：
实机测试：PASS / FAIL
已知问题：
```
