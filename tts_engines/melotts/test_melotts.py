import os

os.environ["PYTORCH_ENABLE_MPS_FALLBACK"] = "1"
os.environ["CUDA_VISIBLE_DEVICES"] = ""

import torch

# 强制让 MeloTTS 认为 MPS 不可用
torch.backends.mps.is_available = lambda: False
torch.backends.mps.is_built = lambda: False

from melo.api import TTS

text = "你好，我是壳中灯。今天有点累也没关系，我们可以先慢一点，把事情一件一件整理清楚。"

model = TTS(language="ZH", device="cpu")

print("spk2id 类型：", type(model.hps.data.spk2id))
print("可用 speaker：", model.hps.data.spk2id)

spk2id = model.hps.data.spk2id

if isinstance(spk2id, dict):
    speaker_id = spk2id.get("ZH", list(spk2id.values())[0])
else:
    if hasattr(spk2id, "ZH"):
        speaker_id = spk2id.ZH
    else:
        speaker_id = 0

print("使用 speaker_id：", speaker_id)

model.tts_to_file(
    text,
    speaker_id,
    "test_melotts.wav",
    speed=1.0,
)