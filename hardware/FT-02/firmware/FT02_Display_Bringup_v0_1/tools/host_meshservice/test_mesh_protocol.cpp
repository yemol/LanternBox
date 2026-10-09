#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include "../../src/FT02_MeshProtocol.h"

static void le32(uint8_t *p, uint32_t v) {
    p[0]=v&0xff; p[1]=(v>>8)&0xff; p[2]=(v>>16)&0xff; p[3]=(v>>24)&0xff;
}

int main() {
    uint8_t frame[32] = {};
    le32(frame+0, 0xFFFFFFFFu);
    le32(frame+4, 0x4C423001u);
    le32(frame+8, 0x12345678u);
    frame[12] = 0x63;
    frame[13] = 0xA4;
    frame[14] = 0x00;
    frame[15] = 0x01;
    FT02MeshHeader h{};
    assert(FT02_MeshParseHeader(frame, sizeof(frame), h));
    assert(h.to == 0xFFFFFFFFu);
    assert(h.from == 0x4C423001u);
    assert(h.id == 0x12345678u);
    assert(h.channel == 0xA4u);

    // Data protobuf: portnum=1, payload="t123", want_response=true,
    // request_id=0xAABBCCDD (fixed32 field 6).
    const uint8_t proto[] = {
        0x08, 0x01,
        0x12, 0x04, 't','1','2','3',
        0x18, 0x01,
        0x35, 0xDD, 0xCC, 0xBB, 0xAA
    };
    FT02MeshDataView d{};
    assert(FT02_MeshParseDataProto(proto, sizeof(proto), d));
    assert(d.portnum == 1u);
    assert(d.payloadLen == 4u);
    assert(std::memcmp(d.payload, "t123", 4) == 0);
    assert(d.wantResponse);
    assert(d.requestId == 0xAABBCCDDu);
    assert(std::strcmp(FT02_MeshPortName(1), "TEXT_MESSAGE_APP") == 0);
    assert(std::strcmp(FT02_MeshPortName(4), "NODEINFO_APP") == 0);
    assert(std::strcmp(FT02_MeshPortName(5), "ROUTING_APP") == 0);
    uint8_t built[64] = {};
    size_t builtLen = 0;
    assert(FT02_MeshBuildDataProto(FT02_MESH_PORT_TEXT_MESSAGE_APP,
                                   reinterpret_cast<const uint8_t*>("123"), 3,
                                   false, 0, built, sizeof(built), builtLen));
    const uint8_t expectedBuilt[] = {0x08,0x01,0x12,0x03,'1','2','3'};
    assert(builtLen == sizeof(expectedBuilt));
    assert(std::memcmp(built, expectedBuilt, sizeof(expectedBuilt)) == 0);
    FT02MeshDataView builtView{};
    assert(FT02_MeshParseDataProto(built, builtLen, builtView));
    assert(builtView.portnum == FT02_MESH_PORT_TEXT_MESSAGE_APP);
    assert(builtView.payloadLen == 3);
    assert(std::memcmp(builtView.payload, "123", 3) == 0);


    std::cout << "MeshProtocol host tests PASS\n";
    return 0;
}
