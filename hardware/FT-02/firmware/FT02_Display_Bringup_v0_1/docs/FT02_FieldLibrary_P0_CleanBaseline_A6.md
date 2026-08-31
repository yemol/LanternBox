# FT-02 Field Library P0 Clean Baseline A6

本基线故意不包含任何书籍正文、候选资料、审核参考资料或测试书。

## A6 P0 Admission Gate

只有同时满足以下条件的资料才能进入 FT-02 `/library`：

1. 中文正文；
2. `priority=P0`；
3. `source_priority=P0`；
4. `library_role=CORE_CONTENT`；
5. `installable=true`；
6. `source_status=APPROVED`；
7. `content_audit_status=APPROVED`；
8. `practical_value_status=APPROVED`；
9. `admission_status=APPROVED`。

任何缺失字段都视为不通过，不再兼容 A2/A3/A4/A5 的旧元数据。

## 明确排除

- 候选资料；
- 审核参考资料；
- 培训大纲、考核标准；
- 职业技能标准；
- 政策、法规、管理文件；
- 主要讲网络、机构协作、核验流程的材料；
- “可能有用”但不能直接指导行动的泛化资料；
- 未完成内容审核或来源审核的书；
- 测试 EPUB/TXT/PDF。

## 重新开始流程

先运行：

```bash
python3 tools/reset_p0_library.py --project-root "$PWD"
```

验证空库：

```bash
python3 tools/validate_ft02_field_library.py "$PWD/SD_ROOT/library"
```

正确结果：

```text
PASS books=0 chapters=0 P0-only=YES A6-gate=YES CLEAN-EMPTY=YES
```

以后每一本书都先审核，再写入 `p0_sources_A6.json`，最后才允许 Fetch/Build。
