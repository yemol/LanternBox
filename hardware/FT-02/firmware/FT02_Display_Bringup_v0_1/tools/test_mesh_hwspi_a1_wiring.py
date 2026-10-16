#!/usr/bin/env python3
from pathlib import Path
p=Path(__file__).resolve().parents[1]
main=(p/'src/main.cpp').read_text(); back=(p/'src/FT02_MeshRadioBackend.cpp').read_text(); hw=(p/'src/FT02_LoRaHwProbe.cpp').read_text(); spi=(p/'src/FT02_LoRaHardwareSpi.cpp').read_text(); spih=(p/'src/FT02_LoRaHardwareSpi.h').read_text(); pio=(p/'platformio.ini').read_text(); svc=(p/'src/FT02_MeshService.cpp').read_text()
checks={
 'epd_fast_swspi':'FAST-GPIO-SW-SPI' in main and 'g_ft02EpdSpi' not in main,
 'hw_spi_class':'SPIClass g_spi(HSPI)' in spi,
 'hw_spi_pins': all(x in spih for x in ['SCK = 38','MOSI = 39','MISO = 40','NSS = 7','BUSY = 13','DIO1 = 48']),
 'hw_spi_begin':'g_spi.begin(SCK, MISO, MOSI, NSS)' in spi,
 'radio_uses_extended_module':'new Module(FT02_LoRaHardwareSpi::NSS' in back and 'FT02_LoRaHardwareSpi::bus()' in back and 'FT02_LoRaHardwareSpi::settings()' in back,
 'no_soft_hal_in_backend':'FT02RadioLibSoftHal' not in back and 'FT02_LoRaSoftSpi' not in back,
 'profile_4mhz_mode0':'CLOCK_HZ = 4000000u' in spih and 'SPI_MODE0' in spi,
 'probe_no_spi_transaction':'FT02_LoRaSoftSpi' not in hw and 'readRadioStatus' not in hw and 'SPI identity check delegated to RadioLib HW-SPI begin' in hw,
 'meshtastic_profile': all(x in back for x in ['478.875f','250.0f','MESH_SF = 11','MESH_CR = 5','MESH_SYNC_WORD = 0x2B','MESH_PREAMBLE = 16']),
 'rx_patch_retained':'SPIsetRegValue(0x08B5, 0x01, 0, 0)' in back,
 'irq_poll_retained':'MESH_IRQ_POLL_INTERVAL_MS = 20' in back and 'getIrqFlags()' in back,
 'tx_enabled':'g_radio->transmit(data, len)' in back and 'FT02_LoRaHwProbeSetRfTransmit()' in back and 'FT02_LoRaHwProbeSetRfReceive()' in back,
 'service_log':'[MESH] ready SX1262/HW-SPI LongFast' in svc and '[MESH-TX] broadcast text=' in svc and ' PKI %s' in svc,
 'radiolib_pinned':'6d8934836678d8894e3d556550475b37dce3e2b6' in pio,
}
for k,v in checks.items(): print(f'{"PASS" if v else "FAIL"} {k}')
raise SystemExit(0 if all(checks.values()) else 1)
