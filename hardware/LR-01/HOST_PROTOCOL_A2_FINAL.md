# FT02-LR01 HOST_PROTOCOL_A2 Final

Protocol version: 2
Status: FROZEN for FT02 Core LR01HostRuntime A2 integration.

## Identity

owner=LanternBox FT-02
owner_short=FT02
node=!4c423002

Startup:

LR01_BOOT version=MESH_NATIVE_A2 protocol=2 node=FT02 id=!4c423002
LR01_READY protocol=2

## Physical Host UART

Core GPIO13 TX -> LR01 GPIO18 RX
Core GPIO7 RX  <- LR01 GPIO17 TX
115200 8N1
ASCII/UTF-8 + LF

## NAV_STATE

NAV_STATE fix=<0|1> fix_type=<0|2|3> lat=<e7> lon=<e7> alt=<dm> sat=<n> sat_used=<n> sat_visible=<n> hdop=<x100> speed=<cm/s> heading=<x10deg> compass=<0|1> compass_q=<0..3> unix=<sec> time_valid=<0|1>

Example:
NAV_STATE fix=1 fix_type=3 lat=312138835 lon=1214956818 alt=286 sat=14 sat_used=14 sat_visible=27 hdop=98 speed=135 heading=968 compass=1 compass_q=2 unix=1787053355 time_valid=1

Units:
lat/lon      degrees * 1e7
alt          0.1 m
hdop         HDOP * 100
speed        cm/s
heading      degree * 10
unix         UTC Unix seconds

sat is retained for A1 compatibility and equals sat_used.

compass_q:
0 invalid
1 poor
2 usable
3 good/calibrated

## RADIO_STATE

RADIO_STATE ready=<0|1> profile=<name> freq=<MHz> rx=<n> nodes=<n> pki=<n> dup=<n> tx_queue=<n>

Example:
RADIO_STATE ready=1 profile=CN35 freq=478.875 rx=25 nodes=3 pki=2 dup=1 tx_queue=0

## SYSTEM_STATE

SYSTEM_STATE gnss=<0|1> compass=<0|1> lora=<0|1> uptime=<sec> gnss_bytes=<n> heap=<bytes> psram=<bytes> rx_errors=<n> uart_errors=<n> radio_resets=<n>

Example:
SYSTEM_STATE gnss=1 compass=1 lora=1 uptime=1234 gnss_bytes=974815 heap=185000 psram=8123456 rx_errors=0 uart_errors=0 radio_resets=0

## Core -> LR01

CORE_PING_<seq>
CORE_STATUS?
CORE_STATUS? id=<uint32>
MESH_NODEINFO
MESH_NODES?
MESH_TX <text>
MESH_TX id=<uint32> <text>
MESH_PRIVATE <node> <text>
MESH_PRIVATE id=<uint32> <node> <text>

## LR01 -> Core

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

## Delivery semantics

MESH_TX_ACCEPTED:
LR01 accepted the Host request.

MESH_TX_SENT:
The LR01 completed local Meshtastic encoding and RF transmission.

MESH_DELIVERY status=ACK:
A matching Meshtastic ROUTING_APP delivery ACK was received and correlated using the transmitted MeshPacket.id / Data.request_id path.

MESH_DELIVERY status=TIMEOUT:
No matching delivery ACK was observed before the LR01 delivery timer expired.

Therefore Core may treat ACK as the reliable-message delivered state. SENT alone must not be treated as delivered.

## Message ID lifecycle

The id=<uint32> supplied by Core is the unique Host-side lifecycle correlation key.

Example:

MESH_PRIVATE id=12346 !4c423001 hello

MESH_TX_ACCEPTED id=12346
MESH_TX_SENT id=12346
MESH_TX_RESULT id=12346 type=PRIVATE ok=1
MESH_DELIVERY id=12346 node=4C423001 status=ACK

The Meshtastic air packet ID is separate and remains internal to LR01. LR01 maintains the mapping between Core id and MeshPacket.id.

## Limits

User message payload: <= 120 UTF-8 bytes.
Host UART input line: <= 256 bytes.
Overlong lines are discarded until LF and reported as:
LR01_ERR code=8 message="line_too_long"
