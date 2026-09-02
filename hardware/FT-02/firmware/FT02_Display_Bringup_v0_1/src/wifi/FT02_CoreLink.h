#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

static constexpr size_t FT02_CORE_LINK_ADDRESS_LEN = 64;

enum FT02CoreLinkState : uint8_t
{
    FT02_CORE_LINK_IDLE = 0,
    FT02_CORE_LINK_DISCOVERING,
    FT02_CORE_LINK_CANDIDATE,
    FT02_CORE_LINK_READY,
    FT02_CORE_LINK_ERROR
};

void FT02_CoreLinkBegin();
void FT02_CoreLinkReset();
void FT02_CoreLinkStartDiscovery();
bool FT02_CoreLinkPoll(uint32_t nowMs);
FT02CoreLinkState FT02_CoreLinkStateCurrent();
const char* FT02_CoreLinkStatusText();
const char* FT02_CoreLinkAddressText();
const char* FT02_CoreLinkConfiguredAddressText();
bool FT02_CoreLinkHasConfiguredAddress();
bool FT02_CoreLinkSetConfiguredAddress(const char* address);
const char* FT02_CoreLinkApiVersionText();
const char* FT02_CoreLinkCoreVersionText();
