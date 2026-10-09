from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
inp = (ROOT / "src" / "FT02_InputManager.cpp").read_text()
main = (ROOT / "src" / "main.cpp").read_text()

checks = {
    "A1.8 protocol": "FT02_KB_RAM_PROTOCOL = 0x17" in inp and "FT02_KB_RAM_SIZE = 8" in inp,
    "RAM snapshot": "FT02_ReadCardKbRamSnapshot" in inp,
    "armed status": "FT02_KB_STATUS_SLEEP_ARMED = 0x02" in inp,
    "request commit pulse": "FT02_KB_HANDSHAKE_SIGNAL_MS = 120" in inp,
    "cancel pulse": "FT02_KB_HANDSHAKE_CANCEL_MS = 260" in inp,
    "RTC open drain": "RTC_GPIO_MODE_INPUT_OUTPUT_OD" in inp,
    "wire released": "Wire.end();" in inp,
    "SCL EXT1 wake": "esp_sleep_enable_ext1_wakeup_io" in main and "ESP_EXT1_WAKEUP_ANY_LOW" in main,
    "no legacy SCL digital wake": "gpio_wakeup_enable" not in main and "GPIO_INTR_LOW_LEVEL" not in main,
    "no timer wake": "esp_sleep_enable_timer_wakeup" not in main,
    "no RAM command": "FT02_KB_CMD_" not in inp,
    "no FIFO-era command": "FT02_CARDKB_HOST_CMD_PREFIX" not in inp,
}
for name, ok in checks.items():
    print(f"{name}: {'PASS' if ok else 'FAIL'}")
    if not ok:
        raise SystemExit(1)
