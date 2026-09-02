#include "FT02_CorePortalClient.h"

#include <stdio.h>
#include <string.h>

namespace
{
static FT02CorePortalPage g_page = {};
static char g_message[80] = "";

static void setItem(uint8_t index, const char* id, const char* label, FT02CorePortalItemType type)
{
    if(index >= FT02_CORE_PORTAL_MAX_ITEMS) return;
    snprintf(g_page.items[index].id, sizeof(g_page.items[index].id), "%s", id != nullptr ? id : "");
    snprintf(g_page.items[index].label, sizeof(g_page.items[index].label), "%s", label != nullptr ? label : "");
    g_page.items[index].type = type;
    g_page.items[index].value = 0;
}
}

void FT02_CorePortalClientBegin()
{
    FT02_CorePortalClientReset();
}

void FT02_CorePortalClientReset()
{
    memset(&g_page, 0, sizeof(g_page));
    snprintf(g_page.page, sizeof(g_page.page), "home");
    snprintf(g_page.title, sizeof(g_page.title), "壳中灯");
    snprintf(g_page.status, sizeof(g_page.status), "未连接Core");
    snprintf(g_message, sizeof(g_message), "");
}

void FT02_CorePortalClientOpenHome()
{
    FT02_CorePortalClientReset();
    setItem(0, "wiki", "Wiki百科", FT02_CORE_PORTAL_ITEM_PAGE);
    setItem(1, "ai", "AI助手", FT02_CORE_PORTAL_ITEM_PAGE);
    setItem(2, "guide", "应急指南", FT02_CORE_PORTAL_ITEM_PAGE);
    setItem(3, "sync", "终端同步", FT02_CORE_PORTAL_ITEM_PAGE);
    g_page.itemCount = 4;
    g_page.selected = 0;
    FT02_CorePortalClientRefreshFromCoreLink();
}

void FT02_CorePortalClientRefreshFromCoreLink()
{
    const FT02CoreLinkState state = FT02_CoreLinkStateCurrent();
    if(state == FT02_CORE_LINK_CANDIDATE || state == FT02_CORE_LINK_READY)
    {
        snprintf(g_page.status, sizeof(g_page.status), "Core地址 %s", FT02_CoreLinkAddressText());
    }
    else if(state == FT02_CORE_LINK_DISCOVERING)
    {
        snprintf(g_page.status, sizeof(g_page.status), "正在连接Core");
    }
    else
    {
        snprintf(g_page.status, sizeof(g_page.status), "%s", FT02_CoreLinkStatusText());
    }
}

bool FT02_CorePortalClientMove(int8_t delta)
{
    if(g_page.itemCount == 0) return false;
    if(delta < 0)
    {
        g_page.selected = g_page.selected == 0 ? static_cast<uint8_t>(g_page.itemCount - 1) : static_cast<uint8_t>(g_page.selected - 1);
    }
    else if(delta > 0)
    {
        g_page.selected = static_cast<uint8_t>((g_page.selected + 1) % g_page.itemCount);
    }
    else
    {
        return false;
    }
    return true;
}

bool FT02_CorePortalClientMoveGrid(int8_t deltaRow, int8_t deltaCol)
{
    if(g_page.itemCount == 0) return false;
    const uint8_t cols = 2;
    const uint8_t current = g_page.selected;
    const int row = current / cols;
    const int col = current % cols;
    const int nextRow = row + deltaRow;
    const int nextCol = col + deltaCol;
    if(nextRow < 0 || nextCol < 0 || nextCol >= cols) return false;
    const int next = nextRow * cols + nextCol;
    if(next < 0 || next >= g_page.itemCount || next == current) return false;
    g_page.selected = static_cast<uint8_t>(next);
    return true;
}

bool FT02_CorePortalClientSelectCurrent()
{
    if(g_page.selected >= g_page.itemCount) return false;
    const FT02CorePortalItem& item = g_page.items[g_page.selected];
    snprintf(g_message, sizeof(g_message), "%s：A2仅显示入口，等待Core API接入", item.label);
    snprintf(g_page.status, sizeof(g_page.status), "%s", g_message);
    return true;
}

const FT02CorePortalPage& FT02_CorePortalClientPage()
{
    return g_page;
}

const char* FT02_CorePortalClientMessage()
{
    return g_message;
}
