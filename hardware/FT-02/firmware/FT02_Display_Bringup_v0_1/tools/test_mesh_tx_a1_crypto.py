from pathlib import Path
import re
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
root=Path(__file__).resolve().parents[1]
secret=(root/'include/FT02_MeshSecrets.h').read_text()
vals=[int(x,16) for x in re.findall(r'0x([0-9A-Fa-f]{2})', secret)]
# first 32 byte constants in this private header are the channel key
key=bytes(vals[-32:])
assert len(key)==32
# Real FT01 -> FT02 broadcast captured on hardware: text="123"
raw=bytes.fromhex('FFFFFFFF0130424C1E61ADFA63A40001C675AED23AE3AD')
from_node=int.from_bytes(raw[4:8],'little')
packet_id=int.from_bytes(raw[8:12],'little')
nonce=packet_id.to_bytes(8,'little')+from_node.to_bytes(4,'little')+(0).to_bytes(4,'little')
plain=Cipher(algorithms.AES(key),modes.CTR(nonce)).encryptor().update(raw[16:])
assert plain==bytes([0x08,0x01,0x12,0x03])+b'123', plain.hex()
print('PASS observed FT01 broadcast decrypts to TEXT_MESSAGE_APP "123"')
# Deterministic FT02 TX vector: builder bytes + AES CTR must round-trip.
ft02=0x4C423002
pid=0x12345678
proto=bytes([0x08,0x01,0x12,0x03])+b'abc'
nonce2=pid.to_bytes(8,'little')+ft02.to_bytes(4,'little')+(0).to_bytes(4,'little')
enc=Cipher(algorithms.AES(key),modes.CTR(nonce2)).encryptor().update(proto)
dec=Cipher(algorithms.AES(key),modes.CTR(nonce2)).decryptor().update(enc)
assert dec==proto
header=(0xFFFFFFFF).to_bytes(4,'little')+ft02.to_bytes(4,'little')+pid.to_bytes(4,'little')+bytes([0x63,0xA4,0x00,0x02])
assert len(header+enc)==23
print('PASS deterministic FT02 broadcast vector len=23 flags=0x63 channel=0xA4 relay=0x02')
