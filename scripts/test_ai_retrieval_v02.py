"""
LanternBox AI 来源召回测试脚本 v0.2

用法：
1. 后端运行在 http://127.0.0.1:8787
2. 将 ai_retrieval_test_cases_v02.json 放在脚本同目录
3. python test_ai_retrieval_v02.py

说明：
- 本脚本只检查来源召回，不评价 AI 正文。
- 默认使用 /api/ai/advice 的 metadata_only=true。
"""
import json
import urllib.request

API_URL = "http://127.0.0.1:8787/api/ai/advice"
TEST_CASES_PATH = "ai_retrieval_test_cases_v02.json"

def post_json(url, payload):
    data = json.dumps(payload, ensure_ascii=False).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=data,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urllib.request.urlopen(req, timeout=90) as resp:
        return json.loads(resp.read().decode("utf-8"))

def title_blob(items):
    return " ".join(str(item.get("title", "")) for item in items)

def main():
    with open(TEST_CASES_PATH, "r", encoding="utf-8") as f:
        cases = json.load(f)

    passed = 0
    failed = []

    for case in cases:
        result = post_json(API_URL, {
            "message": case["query"],
            "mode": "emergency",
            "metadata_only": True,
            "history": [],
        })

        guides = result.get("related_guides", [])
        wikis = result.get("related_wikis", [])
        blob = title_blob(guides + wikis)

        include_ok = any(word in blob for word in case.get("should_include_any", []))
        exclude_ok = not any(word in blob for word in case.get("should_not_include_any", []))
        ok = include_ok and exclude_ok
        passed += int(ok)

        if not ok:
            failed.append(case["id"])

        print("=" * 72)
        print(f"{case['id']} {case['query']}")
        print("返回指南：", [g.get("title") for g in guides])
        print("返回 Wiki：", [w.get("title") for w in wikis])
        print("命中检查：", "OK" if include_ok else "FAIL")
        print("排除检查：", "OK" if exclude_ok else "FAIL")
        print("结果：", "PASS" if ok else "FAIL")

    print("=" * 72)
    print(f"通过：{passed}/{len(cases)}")
    if failed:
        print("失败用例：", ", ".join(failed))

if __name__ == "__main__":
    main()
