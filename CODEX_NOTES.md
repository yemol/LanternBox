# LanternBox / 壳中灯 项目协作规则

1. 未经确认，不要删除文件。
2. 修改前先说明计划修改哪些文件。
3. 优先小步修改，不要一次性大重构。
4. 项目运行前需要启用 venv：
   source venv/bin/activate
5. api 和 data 目录同级。
6. main.py 位于 api 目录下。
7. 项目默认本地离线优先，不要引入强依赖云服务。
8. AI 助手回答应避免依赖外部支援，例如“联系供水公司”等建议。
9. 成员信息、医疗、证件等真实隐私数据不要读取、上传或写入测试样例。
10. 所有 Core、Field Terminal、Study Terminal、Sensor Node 通信设计默认端到端加密。