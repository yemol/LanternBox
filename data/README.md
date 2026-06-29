# LanternBox Data 目录说明

`data/` 存放 LanternBox Core 运行所需的本地数据、配置、测试集与自动生成文件。

## 目录结构

```text
data/
├── context_profiles/          # Context Profile 源文件，一条规则一个 JSON
├── context_profiles.json      # 自动生成文件，由 tools/build_context_profiles.py 生成
│
├── retrieval_benchmark/       # Retrieval Benchmark 源文件，一条测试用例一个 JSON
├── retrieval_benchmark.json   # 自动生成文件，由 tools/build_benchmark.py 生成
│
├── emergency_guides.json      # 应急指南库，当前仍为主运行文件
│
├── guide_taxonomy.json        # Guide 检索用分类词表
├── runtime_settings.json      # 运行时设置
└── README.md                  # 本说明文件