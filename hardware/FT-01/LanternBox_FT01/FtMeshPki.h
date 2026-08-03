#pragma once

#include <Arduino.h>

class FtMeshPki {
public:
  FtMeshPki();

  bool begin();
  bool isReady() const;
  const uint8_t* publicKey() const;

  bool rememberPeer(uint32_t node, const uint8_t publicKey[32]);
  const uint8_t* peerPublicKey(uint32_t node) const;
  uint8_t peerCount() const;
  bool peerNodeAt(size_t index, uint32_t& node) const;

  // Meshtastic PKI payload layout:
  // ciphertext || 8-byte CCM authentication tag || 4-byte extra nonce.
  bool encryptForPeer(const uint8_t peerPublicKey[32],
                      uint32_t packetId,
                      uint32_t fromNode,
                      const uint8_t* plain,
                      size_t plainLen,
                      uint8_t* encrypted,
                      size_t encryptedCapacity,
                      size_t& encryptedLen) const;

  bool decryptFromPeer(const uint8_t peerPublicKey[32],
                       uint32_t packetId,
                       uint32_t fromNode,
                       const uint8_t* encrypted,
                       size_t encryptedLen,
                       uint8_t* plain,
                       size_t plainCapacity,
                       size_t& plainLen) const;

private:
  static const size_t PEER_MAX = 64;

  struct PeerEntry {
    uint32_t node;
    uint8_t publicKey[32];
    bool valid;
  };

  bool savePeers() const;

  uint8_t privateKeyBytes[32];
  uint8_t publicKeyBytes[32];
  PeerEntry peers[PEER_MAX];
  size_t peerStored;
  size_t peerReplaceIndex;
  bool ready;
};
