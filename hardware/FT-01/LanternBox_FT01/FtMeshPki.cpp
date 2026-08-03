#include "FtMeshPki.h"

#include <Preferences.h>
#include <esp_system.h>
#include <mbedtls/ccm.h>
#include <mbedtls/sha256.h>
#include <string.h>

namespace {
  typedef int64_t FieldElement[16];

  const FieldElement CURVE_121665 = {0xDB41, 1};
  const uint8_t CURVE_BASEPOINT[32] = {9};
  const size_t PRIVATE_KEY_BYTES = 32;
  const size_t PUBLIC_KEY_BYTES = 32;
  const size_t CCM_TAG_BYTES = 8;
  const size_t EXTRA_NONCE_BYTES = 4;
  const size_t PKI_OVERHEAD_BYTES = CCM_TAG_BYTES + EXTRA_NONCE_BYTES;
  const size_t CCM_NONCE_BYTES = 13;
  const char* NVS_NAMESPACE = "ft01pki";
  const char* NVS_PRIVATE_KEY = "private";
  const char* NVS_PEER_KEYS = "peers";
  const size_t STORED_PEER_BYTES = 36;

  void carry(FieldElement value) {
    for (int i = 0; i < 16; i++) {
      value[i] += (int64_t)1 << 16;
      int64_t carryValue = value[i] >> 16;
      value[(i + 1) * (i < 15)] += carryValue - 1 + 37 * (carryValue - 1) * (i == 15);
      value[i] -= carryValue << 16;
    }
  }

  void conditionalSwap(FieldElement left, FieldElement right, int swap) {
    int64_t mask = ~(int64_t)(swap - 1);
    for (int i = 0; i < 16; i++) {
      int64_t delta = mask & (left[i] ^ right[i]);
      left[i] ^= delta;
      right[i] ^= delta;
    }
  }

  void packField(uint8_t out[32], const FieldElement value) {
    FieldElement t;
    FieldElement m;
    for (int i = 0; i < 16; i++) t[i] = value[i];
    carry(t);
    carry(t);
    carry(t);

    for (int round = 0; round < 2; round++) {
      m[0] = t[0] - 0xFFED;
      for (int i = 1; i < 15; i++) {
        m[i] = t[i] - 0xFFFF - ((m[i - 1] >> 16) & 1);
        m[i - 1] &= 0xFFFF;
      }
      m[15] = t[15] - 0x7FFF - ((m[14] >> 16) & 1);
      int borrow = (int)((m[15] >> 16) & 1);
      m[14] &= 0xFFFF;
      conditionalSwap(t, m, 1 - borrow);
    }

    for (int i = 0; i < 16; i++) {
      out[2 * i] = (uint8_t)(t[i] & 0xFF);
      out[2 * i + 1] = (uint8_t)(t[i] >> 8);
    }
  }

  void unpackField(FieldElement out, const uint8_t in[32]) {
    for (int i = 0; i < 16; i++) out[i] = in[2 * i] + ((int64_t)in[2 * i + 1] << 8);
    out[15] &= 0x7FFF;
  }

  void addField(FieldElement out, const FieldElement left, const FieldElement right) {
    for (int i = 0; i < 16; i++) out[i] = left[i] + right[i];
  }

  void subtractField(FieldElement out, const FieldElement left, const FieldElement right) {
    for (int i = 0; i < 16; i++) out[i] = left[i] - right[i];
  }

  void multiplyField(FieldElement out, const FieldElement left, const FieldElement right) {
    int64_t product[31] = {0};
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 16; j++) product[i + j] += left[i] * right[j];
    }
    for (int i = 0; i < 15; i++) product[i] += 38 * product[i + 16];
    for (int i = 0; i < 16; i++) out[i] = product[i];
    carry(out);
    carry(out);
  }

  void squareField(FieldElement out, const FieldElement value) {
    multiplyField(out, value, value);
  }

  void invertField(FieldElement out, const FieldElement value) {
    FieldElement c;
    for (int i = 0; i < 16; i++) c[i] = value[i];
    for (int exponent = 253; exponent >= 0; exponent--) {
      squareField(c, c);
      if (exponent != 2 && exponent != 4) multiplyField(c, c, value);
    }
    for (int i = 0; i < 16; i++) out[i] = c[i];
  }

  bool isAllZero(const uint8_t* data, size_t len) {
    uint8_t combined = 0;
    for (size_t i = 0; i < len; i++) combined |= data[i];
    return combined == 0;
  }

  bool x25519(uint8_t out[32], const uint8_t scalar[32], const uint8_t point[32]) {
    uint8_t clamped[32];
    memcpy(clamped, scalar, sizeof(clamped));
    clamped[31] = (uint8_t)((clamped[31] & 0x7F) | 0x40);
    clamped[0] &= 0xF8;

    FieldElement x;
    FieldElement a;
    FieldElement b;
    FieldElement c;
    FieldElement d;
    FieldElement e;
    FieldElement f;
    unpackField(x, point);
    for (int i = 0; i < 16; i++) {
      b[i] = x[i];
      d[i] = a[i] = c[i] = 0;
    }
    a[0] = d[0] = 1;

    for (int bit = 254; bit >= 0; bit--) {
      int swap = (clamped[bit >> 3] >> (bit & 7)) & 1;
      conditionalSwap(a, b, swap);
      conditionalSwap(c, d, swap);
      addField(e, a, c);
      subtractField(a, a, c);
      addField(c, b, d);
      subtractField(b, b, d);
      squareField(d, e);
      squareField(f, a);
      multiplyField(a, c, a);
      multiplyField(c, b, e);
      addField(e, a, c);
      subtractField(a, a, c);
      squareField(b, a);
      subtractField(c, d, f);
      multiplyField(a, c, CURVE_121665);
      addField(a, a, d);
      multiplyField(c, c, a);
      multiplyField(a, d, f);
      multiplyField(d, b, x);
      squareField(b, e);
      conditionalSwap(a, b, swap);
      conditionalSwap(c, d, swap);
    }

    invertField(c, c);
    multiplyField(a, a, c);
    packField(out, a);
    memset(clamped, 0, sizeof(clamped));
    return !isAllZero(out, 32);
  }

  void writeLe32(uint8_t* out, uint32_t value) {
    out[0] = (uint8_t)(value & 0xFF);
    out[1] = (uint8_t)((value >> 8) & 0xFF);
    out[2] = (uint8_t)((value >> 16) & 0xFF);
    out[3] = (uint8_t)((value >> 24) & 0xFF);
  }

  void writeLe64(uint8_t* out, uint64_t value) {
    for (uint8_t i = 0; i < 8; i++) out[i] = (uint8_t)((value >> (8 * i)) & 0xFF);
  }

  void buildPkiNonce(uint8_t nonce[16], uint32_t packetId, uint32_t fromNode, uint32_t extraNonce) {
    memset(nonce, 0, 16);
    writeLe64(nonce, packetId);
    writeLe32(nonce + 8, fromNode);
    // Meshtastic overlays the random extra nonce on bytes 4..7.
    writeLe32(nonce + 4, extraNonce);
  }

  uint32_t readLe32(const uint8_t* data) {
    return ((uint32_t)data[0]) |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
  }
}

FtMeshPki::FtMeshPki() : peerStored(0), peerReplaceIndex(0), ready(false) {
  memset(privateKeyBytes, 0, sizeof(privateKeyBytes));
  memset(publicKeyBytes, 0, sizeof(publicKeyBytes));
  for (size_t i = 0; i < PEER_MAX; i++) {
    peers[i].node = 0;
    memset(peers[i].publicKey, 0, sizeof(peers[i].publicKey));
    peers[i].valid = false;
  }
}

bool FtMeshPki::begin() {
  if (ready) return true;

  Preferences preferences;
  if (!preferences.begin(NVS_NAMESPACE, false)) return false;

  bool loaded = preferences.getBytesLength(NVS_PRIVATE_KEY) == PRIVATE_KEY_BYTES &&
                preferences.getBytes(NVS_PRIVATE_KEY, privateKeyBytes, PRIVATE_KEY_BYTES) == PRIVATE_KEY_BYTES;

  if (!loaded || isAllZero(privateKeyBytes, sizeof(privateKeyBytes))) {
    for (size_t offset = 0; offset < PRIVATE_KEY_BYTES; offset += sizeof(uint32_t)) {
      uint32_t randomWord = esp_random();
      memcpy(privateKeyBytes + offset, &randomWord, sizeof(randomWord));
    }
    if (isAllZero(privateKeyBytes, sizeof(privateKeyBytes)) ||
        preferences.putBytes(NVS_PRIVATE_KEY, privateKeyBytes, PRIVATE_KEY_BYTES) != PRIVATE_KEY_BYTES) {
      preferences.end();
      memset(privateKeyBytes, 0, sizeof(privateKeyBytes));
      return false;
    }
  }
  size_t storedPeerBytes = preferences.getBytesLength(NVS_PEER_KEYS);
  if (storedPeerBytes > 0 && storedPeerBytes <= PEER_MAX * STORED_PEER_BYTES && storedPeerBytes % STORED_PEER_BYTES == 0) {
    uint8_t storedPeers[PEER_MAX * STORED_PEER_BYTES];
    if (preferences.getBytes(NVS_PEER_KEYS, storedPeers, storedPeerBytes) == storedPeerBytes) {
      size_t records = storedPeerBytes / STORED_PEER_BYTES;
      for (size_t i = 0; i < records; i++) {
        const uint8_t* record = storedPeers + i * STORED_PEER_BYTES;
        uint32_t node = readLe32(record);
        if (node != 0 && !isAllZero(record + 4, 32)) {
          peers[peerStored].node = node;
          memcpy(peers[peerStored].publicKey, record + 4, 32);
          peers[peerStored].valid = true;
          peerStored++;
        }
      }
    }
  }
  preferences.end();

  ready = x25519(publicKeyBytes, privateKeyBytes, CURVE_BASEPOINT);
  if (!ready) {
    memset(privateKeyBytes, 0, sizeof(privateKeyBytes));
    memset(publicKeyBytes, 0, sizeof(publicKeyBytes));
  }
  return ready;
}

bool FtMeshPki::isReady() const {
  return ready;
}

const uint8_t* FtMeshPki::publicKey() const {
  return ready ? publicKeyBytes : nullptr;
}

bool FtMeshPki::rememberPeer(uint32_t node, const uint8_t peerKey[32]) {
  if (node == 0 || !peerKey || isAllZero(peerKey, 32)) return false;

  for (size_t i = 0; i < PEER_MAX; i++) {
    if (peers[i].valid && peers[i].node == node) {
      if (memcmp(peers[i].publicKey, peerKey, 32) == 0) return true;
      memcpy(peers[i].publicKey, peerKey, 32);
      return savePeers();
    }
  }

  size_t slot = PEER_MAX;
  for (size_t i = 0; i < PEER_MAX; i++) {
    if (!peers[i].valid) {
      slot = i;
      break;
    }
  }
  if (slot == PEER_MAX) {
    slot = peerReplaceIndex;
    peerReplaceIndex = (peerReplaceIndex + 1) % PEER_MAX;
  } else {
    peerStored++;
  }

  peers[slot].node = node;
  memcpy(peers[slot].publicKey, peerKey, 32);
  peers[slot].valid = true;
  return savePeers();
}

const uint8_t* FtMeshPki::peerPublicKey(uint32_t node) const {
  for (size_t i = 0; i < PEER_MAX; i++) {
    if (peers[i].valid && peers[i].node == node) return peers[i].publicKey;
  }
  return nullptr;
}

uint8_t FtMeshPki::peerCount() const {
  return (uint8_t)peerStored;
}

bool FtMeshPki::peerNodeAt(size_t index, uint32_t& node) const {
  size_t found = 0;
  for (size_t i = 0; i < PEER_MAX; i++) {
    if (!peers[i].valid) continue;
    if (found == index) {
      node = peers[i].node;
      return true;
    }
    found++;
  }
  node = 0;
  return false;
}

bool FtMeshPki::savePeers() const {
  uint8_t storedPeers[PEER_MAX * STORED_PEER_BYTES];
  size_t storedLen = 0;
  for (size_t i = 0; i < PEER_MAX; i++) {
    if (!peers[i].valid) continue;
    writeLe32(storedPeers + storedLen, peers[i].node);
    memcpy(storedPeers + storedLen + 4, peers[i].publicKey, 32);
    storedLen += STORED_PEER_BYTES;
  }

  Preferences preferences;
  if (!preferences.begin(NVS_NAMESPACE, false)) return false;
  bool saved = preferences.putBytes(NVS_PEER_KEYS, storedPeers, storedLen) == storedLen;
  preferences.end();
  return saved;
}

bool FtMeshPki::encryptForPeer(const uint8_t peerPublicKey[32],
                               uint32_t packetId,
                               uint32_t fromNode,
                               const uint8_t* plain,
                               size_t plainLen,
                               uint8_t* encrypted,
                               size_t encryptedCapacity,
                               size_t& encryptedLen) const {
  encryptedLen = 0;
  if (!ready || !peerPublicKey || !plain || !encrypted || plainLen == 0) return false;
  if (plainLen + PKI_OVERHEAD_BYTES > encryptedCapacity) return false;

  uint8_t sharedSecret[32];
  uint8_t aesKey[32];
  if (!x25519(sharedSecret, privateKeyBytes, peerPublicKey)) return false;
  if (mbedtls_sha256(sharedSecret, sizeof(sharedSecret), aesKey, 0) != 0) {
    memset(sharedSecret, 0, sizeof(sharedSecret));
    return false;
  }

  uint32_t extraNonce = esp_random();
  if (extraNonce == 0) extraNonce = 1;
  uint8_t nonce[16];
  buildPkiNonce(nonce, packetId, fromNode, extraNonce);

  uint8_t* tag = encrypted + plainLen;
  mbedtls_ccm_context context;
  mbedtls_ccm_init(&context);
  int result = mbedtls_ccm_setkey(&context, MBEDTLS_CIPHER_ID_AES, aesKey, 256);
  if (result == 0) {
    result = mbedtls_ccm_encrypt_and_tag(&context,
                                         plainLen,
                                         nonce,
                                         CCM_NONCE_BYTES,
                                         nullptr,
                                         0,
                                         plain,
                                         encrypted,
                                         tag,
                                         CCM_TAG_BYTES);
  }
  mbedtls_ccm_free(&context);
  memset(sharedSecret, 0, sizeof(sharedSecret));
  memset(aesKey, 0, sizeof(aesKey));

  if (result != 0) {
    memset(encrypted, 0, plainLen + PKI_OVERHEAD_BYTES);
    return false;
  }
  writeLe32(encrypted + plainLen + CCM_TAG_BYTES, extraNonce);
  encryptedLen = plainLen + PKI_OVERHEAD_BYTES;
  return true;
}

bool FtMeshPki::decryptFromPeer(const uint8_t peerPublicKey[32],
                                uint32_t packetId,
                                uint32_t fromNode,
                                const uint8_t* encrypted,
                                size_t encryptedLen,
                                uint8_t* plain,
                                size_t plainCapacity,
                                size_t& plainLen) const {
  plainLen = 0;
  if (!ready || !peerPublicKey || !encrypted || !plain) return false;
  if (encryptedLen <= PKI_OVERHEAD_BYTES) return false;

  const size_t cipherLen = encryptedLen - PKI_OVERHEAD_BYTES;
  if (cipherLen > plainCapacity) return false;

  uint8_t sharedSecret[32];
  uint8_t aesKey[32];
  if (!x25519(sharedSecret, privateKeyBytes, peerPublicKey)) return false;
  if (mbedtls_sha256(sharedSecret, sizeof(sharedSecret), aesKey, 0) != 0) {
    memset(sharedSecret, 0, sizeof(sharedSecret));
    return false;
  }

  const uint8_t* tag = encrypted + cipherLen;
  uint32_t extraNonce = readLe32(encrypted + cipherLen + CCM_TAG_BYTES);
  uint8_t nonce[16];
  buildPkiNonce(nonce, packetId, fromNode, extraNonce);

  mbedtls_ccm_context context;
  mbedtls_ccm_init(&context);
  int result = mbedtls_ccm_setkey(&context, MBEDTLS_CIPHER_ID_AES, aesKey, 256);
  if (result == 0) {
    result = mbedtls_ccm_auth_decrypt(&context,
                                      cipherLen,
                                      nonce,
                                      CCM_NONCE_BYTES,
                                      nullptr,
                                      0,
                                      encrypted,
                                      plain,
                                      tag,
                                      CCM_TAG_BYTES);
  }
  mbedtls_ccm_free(&context);
  memset(sharedSecret, 0, sizeof(sharedSecret));
  memset(aesKey, 0, sizeof(aesKey));

  if (result != 0) {
    memset(plain, 0, cipherLen);
    return false;
  }
  plainLen = cipherLen;
  return true;
}
