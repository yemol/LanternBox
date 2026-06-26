from api.context.engine import analyze_context

text = "水只剩一桶了，而且今天很热。"

ctx = analyze_context(text)

print(ctx.model_dump_json(indent=2, ensure_ascii=False))