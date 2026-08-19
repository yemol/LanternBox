# FT02-LR01 v1.1 本地 Codex + KiCad 制造收口包

**状态：FINAL DESIGN FREEZE / LOCAL FABRICATION HANDOFF。不是可直接下单的 Gerber 包。**

只有 `output/FABRICATION_RELEASE_GATE.md` 最终为 `Overall: PASS`，并且 ERC / DRC / RF / BOM / CPL / Gerber / Drill / 固件门全部通过后，才允许下单。

## 最短运行流程

1. 安装 KiCad 10 stable。
2. 安装 Codex CLI：

```bash
curl -fsSL https://chatgpt.com/codex/install.sh | sh
```

3. 解压本包，终端进入目录，例如：

```bash
cd ~/Downloads/FT02-LR01_v1.1_Local_Codex_Fabrication_Handoff
```

4. 检查环境：

```bash
bash scripts/01_CHECK_ENV.command
```

5. 启动 Codex：

```bash
bash scripts/02_START_CODEX.command
```

该脚本会把首条任务复制到剪贴板并启动 Codex。Codex 打开后按 `Command+V` 粘贴并发送。

首条任务内容：

> Read AGENTS.md and CODEX_MASTER_TASK.md. Execute the FT02-LR01 v1.1 manufacturing closure end-to-end. Do not add or redesign any features. Preserve the Semtech E512V01A RF reference exactly. Do not mark fabrication release PASS unless every mandatory gate passes.

## Codex 权限

允许 Codex：
- 读写当前工程目录；
- 调用 `git`；
- 调用 `kicad-cli`；
- 运行本包 `scripts/` 下的检查/导出脚本。

不要允许：
- 删除 `reference/` 的官方 RF 文件；
- 为了“清零”而忽略真实 ERC/DRC 错误；
- 修改已经冻结的硬件功能或引脚合同；
- 重新设计 E512 RF 区。

## 可能出现的一次人工步骤：官方 Altium RF 导入

如果 Codex 报告 `MANUAL_RF_IMPORT_REQUIRED`，说明当前 KiCad CLI 无法无损自动完成官方 Altium PCB 的导入。

只需要：
1. 用 KiCad GUI 导入 `reference/SX1268MB1xAS_E512V01A_Altium_Package.zip` 中的官方 E512 PCB；
2. 保存成 KiCad PCB；
3. 放入 `reference/e512_imported/`；
4. 回 Codex 输入 `continue`。

不要人工重新画 RF。

## 最终检查

完成后重点看：

- `output/FABRICATION_RELEASE_GATE.md`
- `logs/ERC.txt`
- `logs/DRC.txt`
- `logs/FIRMWARE_BUILD.txt`
- `output/gerber/`
- `output/drill/`
- `output/FT02-LR01_v1.1_BOM.csv`
- `output/FT02-LR01_v1.1_CPL.csv`
- `output/FT02-LR01_v1.1_Fabrication_Release.zip`

**只有 `Overall: PASS` 才可以下单。**
