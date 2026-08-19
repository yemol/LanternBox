# FT02 ↔ LR01 Host Protocol A2

**Protocol version:** 2  
**Status:** FT-02 Core production interface  
**Hardware authority:** LR01 owns GNSS, QMC5883L compass, Meshtastic/LoRa.

Core must not directly access GNSS UART, QMC5883L I2C, SX126x, or calculate/store compass calibration parameters.

## 1. Physical UART

```text
FT02 Core GPIO13 TX -> LR01 GPIO18 RX
FT02 Core GPIO7  RX <- LR01 GPIO17 TX
115200 8N1
ASCII / UTF-8 + LF
```

Identity:

```text
owner=LanternBox FT-02
owner_short=FT02
node=!4c423002
```

Startup:

```text
LR01_BOOT version=MESH_NATIVE_A2 protocol=2 node=FT02 id=!4c423002
LR01_READY protocol=2
```

## 2. Navigation state

```text
NAV_STATE fix=<0|1> fix_type=<0|2|3> lat=<e7> lon=<e7> alt=<dm> sat=<n> sat_used=<n> sat_visible=<n> hdop=<x100> speed=<cm/s> heading=<x10deg> compass=<0|1> compass_q=<0..3> unix=<sec> time_valid=<0|1>
```

Units:

- `lat/lon`: degree × 1e7
- `alt`: 0.1 m
- `hdop`: HDOP × 100
- `speed`: cm/s
- `heading`: degree × 10
- `unix`: UTC Unix seconds
- `sat`: A1 compatibility field, equal to `sat_used`

`compass_q` current meaning:

- `0`: sensor / heading invalid
- `1`: QMC5883L normal, but no saved calibration
- `2`: saved usable calibration
- `3`: saved high-quality calibration

Core consumes `heading`, `compass`, and `compass_q`; it does not calculate heading itself.

## 3. Radio state

```text
RADIO_STATE ready=<0|1> profile=<name> freq=<MHz> rx=<n> nodes=<n> pki=<n> dup=<n> tx_queue=<n>
```

## 4. System state

```text
SYSTEM_STATE gnss=<0|1> compass=<0|1> lora=<0|1> uptime=<sec> gnss_bytes=<n> heap=<bytes> psram=<bytes> rx_errors=<n> uart_errors=<n> radio_resets=<n>
```

## 5. Core → LR01 commands

General:

```text
CORE_PING_<seq>
CORE_STATUS?
CORE_STATUS? id=<uint32>
MESH_NODEINFO
MESH_NODES?
MESH_TX <text>
MESH_TX id=<uint32> <text>
MESH_PRIVATE <node> <text>
MESH_PRIVATE id=<uint32> <node> <text>
```

Compass calibration:

```text
COMPASS_CAL_START
COMPASS_CAL_STATUS?
COMPASS_CAL_SAVE
COMPASS_CAL_CANCEL
COMPASS_CAL_RESET
```

## 6. LR01 → Core messages

General:

```text
LR01_PONG_<seq>
MESH_TX_ACCEPTED id=<uint32>
MESH_TX_SENT id=<uint32>
MESH_TX_FAILED id=<uint32> reason=<reason>
MESH_TX_RESULT id=<uint32> type=TEXT|PRIVATE ok=0|1 [reason=...]
MESH_DELIVERY id=<uint32> node=<hex> status=ACK|TIMEOUT
MESH_RX id=<air_packet_id> from=<hex> to=<hex> name="..." kind=... rssi=... snr=... text="..."
MESH_NODE id=!<hex> long="..." short="..." online=0|1 hops=<n|-1> rssi=... snr=... pki=0|1 last=<seconds>
MESH_NODE_END count=<n>
STATUS_END
STATUS_END id=<uint32>
LR01_ERR code=<n> message="<text>"
```

Compass calibration state:

```text
COMPASS_CAL_STATE state=<STATE> progress=<0..100> quality=<0..3> samples=<n> calibrated=<0|1> min_x=<n> max_x=<n> min_y=<n> max_y=<n> min_z=<n> max_z=<n>
```

`STATE`:

```text
IDLE
RUNNING
READY
SAVED
CANCELED
FAILED
```

`calibrated=1` means LR01 currently has a valid calibration persisted in NVS. During a new `RUNNING` session it may still be `1`, because the previous saved calibration remains active until a new `COMPASS_CAL_SAVE` succeeds.

LR01 sends `COMPASS_CAL_STATE` approximately once per second while `RUNNING` or `READY`. Core should not high-frequency poll.

## 7. Compass calibration flow

```text
Enter Core calibration page
        ↓
COMPASS_CAL_STATUS?
        ↓
COMPASS_CAL_START
        ↓
RUNNING
        ↓
user rotates device / performs 3D figure-eight motion
        ↓
READY
        ↓
COMPASS_CAL_SAVE
        ↓
SAVED
```

Cancel:

```text
COMPASS_CAL_CANCEL
```

Cancelling does not modify the previously persisted calibration.

Reset:

```text
COMPASS_CAL_RESET
```

Reset deletes the LR01 persisted calibration and therefore requires a second confirmation in Core UI.

Calibration errors:

```text
20 compass_cal_busy
21 compass_cal_not_running
22 compass_cal_quality_low
23 compass_cal_save_failed
24 compass_cal_reset_failed
25 compass_not_ready
```

LR01 calibration is non-blocking. GNSS, Meshtastic, Host PING and status traffic continue during calibration.

## 8. Delivery semantics

`MESH_TX_ACCEPTED` means LR01 accepted the host request.

`MESH_TX_SENT` means local RF transmission completed. It does **not** prove delivery.

`MESH_DELIVERY status=ACK` means a matching Meshtastic delivery ACK was received and Core may mark the reliable message delivered.

`MESH_DELIVERY status=TIMEOUT` means no matching ACK was received before timeout.

The Core-supplied `id=<uint32>` is the host-side lifecycle correlation key. Meshtastic air packet IDs remain LR01-internal.

## 9. Limits

- User message payload: `<= 120 UTF-8 bytes`
- Host UART input line: `<= 256 bytes`
- Overlong line is dropped until LF:

```text
LR01_ERR code=8 message="line_too_long"
```
