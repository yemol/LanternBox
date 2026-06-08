import argparse
import os
from pathlib import Path

os.environ["PYTORCH_ENABLE_MPS_FALLBACK"] = "1"
os.environ["CUDA_VISIBLE_DEVICES"] = ""

import torch

# Mac M 系列上强制禁用 MPS，避免 Placeholder storage 报错
torch.backends.mps.is_available = lambda: False
torch.backends.mps.is_built = lambda: False

from melo.api import TTS


def get_speaker_id(model: TTS) -> int:
    spk2id = model.hps.data.spk2id

    if isinstance(spk2id, dict):
        return spk2id.get("ZH", list(spk2id.values())[0])

    if hasattr(spk2id, "ZH"):
        return spk2id.ZH

    return 0


def synthesize(text: str, output_path: str, speed: float = 1.0) -> None:
    text = text.strip()

    if not text:
        raise ValueError("Text is empty.")

    output = Path(output_path)
    output.parent.mkdir(parents=True, exist_ok=True)

    model = TTS(language="ZH", device="cpu")
    speaker_id = get_speaker_id(model)

    model.tts_to_file(
        text,
        speaker_id,
        str(output),
        speed=speed,
    )


def main():
    parser = argparse.ArgumentParser(description="LanternBox MeloTTS speaker")
    parser.add_argument("--text", required=True, help="Text to synthesize")
    parser.add_argument("--output", required=True, help="Output wav file path")
    parser.add_argument("--speed", type=float, default=1.0, help="Speech speed")

    args = parser.parse_args()

    synthesize(
        text=args.text,
        output_path=args.output,
        speed=args.speed,
    )


if __name__ == "__main__":
    main()