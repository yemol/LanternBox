#pragma once

#include "FT02_FontPackRenderer.h"
#include "FT02_InputManager.h"

enum FT02CommunicationInputResult : uint8_t
{
    FT02_COMM_INPUT_NONE = 0,
    FT02_COMM_INPUT_REDRAW,
    FT02_COMM_INPUT_EXIT_HOME,
    FT02_COMM_INPUT_OPEN_HELP
};

void FT02_CommunicationUIOpen();
void FT02_DrawCommunicationNodeScreen(FT02Display& display);
FT02CommunicationInputResult FT02_CommunicationUIHandleInput(const FT02InputEvent& event);
bool FT02_CommunicationUITakeDeferredRedraw(uint32_t nowMs);
bool FT02_CommunicationUIIsInbox();
bool FT02_CommunicationUIIsCompose();
bool FT02_CommunicationUIIsNodes();
void FT02_CommunicationUIOnNodeListUpdated(uint16_t count);
void FT02_CommunicationUIOnSyncStarted(const char* noticeText);
void FT02_CommunicationUIOnSyncReady();
