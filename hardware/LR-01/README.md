# FT02-LR01 MeshtasticNative A2

A2 is a backward-compatible extension of the A1 firmware that was already
verified on real hardware for Core UART, broadcast messaging and PKI private messaging.

Identity:
- owner: LanternBox FT-02
- owner_short: FT02
- node: !4c423002

Host:
- Core TX GPIO13 -> LR01 RX GPIO18
- Core RX GPIO7 <- LR01 TX GPIO17
- 115200 8N1, ASCII/UTF-8 + LF

## A2 commands

CORE_PING_x
CORE_STATUS?
CORE_STATUS? id=42
MESH_NODEINFO
MESH_NODES?
MESH_TX hello
MESH_TX id=123 hello
MESH_PRIVATE !4c423001 hello
MESH_PRIVATE id=124 !4c423001 hello

## A2 events

LR01_BOOT version=MESH_NATIVE_A2 protocol=2 node=FT02 id=!4c423002
LR01_READY protocol=2

MESH_TX_ACCEPTED id=123
MESH_TX_SENT id=123
MESH_TX_FAILED id=123 reason=...
MESH_TX_RESULT id=123 type=TEXT|PRIVATE ok=0|1 ...
MESH_DELIVERY id=124 node=4C423001 status=ACK|TIMEOUT

MESH_NODE ...
MESH_NODE_END count=N

MESH_RX id=<air_packet_id> from=<node> to=<node> ...

STATUS_END
STATUS_END id=42

## NAV additions
- fix_type=0/2/3
- sat_used=
- sat_visible=
- speed=<cm/s>
- unix=<UTC Unix seconds>
- time_valid=0/1
- compass_q=0..3

The legacy `sat=` field remains and equals sat_used for A1 compatibility.

## SYSTEM additions
- heap=
- psram=
- rx_errors=
- uart_errors=
- radio_resets=

## RADIO additions
- tx_queue=0

The current radio TX path is synchronous, so queue depth is 0. Private messages
are tracked separately for delivery ACK/TIMEOUT.

## limits
- user message payload: <=120 UTF-8 bytes
- Host UART line: 256 bytes


## HOST_PROTOCOL_A2 Final lock

The following three state frames are frozen for Core A2 integration.

NAV_STATE:
NAV_STATE fix=<0|1> fix_type=<0|2|3> lat=<e7> lon=<e7> alt=<dm> sat=<n> sat_used=<n> sat_visible=<n> hdop=<x100> speed=<cm/s> heading=<x10deg> compass=<0|1> compass_q=<0..3> unix=<sec> time_valid=<0|1>

RADIO_STATE:
RADIO_STATE ready=<0|1> profile=<name> freq=<MHz> rx=<n> nodes=<n> pki=<n> dup=<n> tx_queue=<n>

SYSTEM_STATE:
SYSTEM_STATE gnss=<0|1> compass=<0|1> lora=<0|1> uptime=<sec> gnss_bytes=<n> heap=<bytes> psram=<bytes> rx_errors=<n> uart_errors=<n> radio_resets=<n>

Identity:
owner=LanternBox FT-02
owner_short=FT02
node=!4c423002

MESH_DELIVERY status=ACK means a matching Meshtastic ROUTING_APP delivery ACK was received from the mesh, not merely local radio transmission completion.

Core-provided id=<uint32> remains the Host-side lifecycle key through ACCEPTED / SENT / FAILED / RESULT / DELIVERY.


# QMC5883L Compass Calibration Extension

This build adds a non-blocking, persistent compass calibration flow owned entirely by LR01.
Core never receives raw magnetometer samples and never accesses QMC5883L I2C.

Commands:
- COMPASS_CAL_START
- COMPASS_CAL_STATUS?
- COMPASS_CAL_SAVE
- COMPASS_CAL_CANCEL
- COMPASS_CAL_RESET

Status:
COMPASS_CAL_STATE state=<IDLE|RUNNING|READY|SAVED|CANCELED|FAILED> progress=<0..100> quality=<0..3> samples=<n> calibrated=<0|1> min_x=<n> max_x=<n> min_y=<n> max_y=<n> min_z=<n> max_z=<n>

Persistence:
- ESP32 Preferences / NVS namespace `qmc_cal`
- two-slot generation + CRC32 storage
- old valid slot remains a rollback copy
- SAVE is the only operation that activates a new calibration
- START never deletes the saved calibration

Heading behavior:
- During calibration, heading continues using the previously saved calibration.
- If no saved calibration exists, raw magnetic heading is used and compass_q=1.
- After SAVE, the new offsets/scales take effect immediately.
- X/Y/Z are all corrected. Because LR01 has no accelerometer, heading remains a 2D magnetic azimuth from corrected X/Y; corrected Z is used in calibration coverage/quality and retained for future tilt compensation.
