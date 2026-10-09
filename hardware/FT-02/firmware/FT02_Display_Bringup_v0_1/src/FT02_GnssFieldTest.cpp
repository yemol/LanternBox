#include "FT02_GnssFieldTest.h"
#include <stdio.h>
#include <string.h>
#include "FT02_BuildInfo.h"
#include "FT02_Gnss.h"
#include "FT02_CompassCore.h"
#include "FT02_Storage.h"
namespace {
constexpr uint32_t SNAP_MS=1000UL, SYNC_MS=5000UL;
FT02GnssFieldTestSnapshot g_test={}; FILE* g_file=nullptr; uint32_t g_lastSnap=0,g_lastSync=0;
void changed(){++g_test.uiGeneration;}
void action(const char* t){snprintf(g_test.lastAction,sizeof(g_test.lastAction),"%s",t?t:"");changed();}
bool writeLine(const char* l){if(!g_test.active||!g_file||!l)return false; bool ok=fwrite(l,1,strlen(l),g_file)==strlen(l); ok=fwrite("\n",1,1,g_file)==1&&ok; if(!ok){++g_test.writeErrorCount;action("导航日志写入失败");} return ok;}
void buildPath(){snprintf(g_test.path,sizeof(g_test.path),"/lanternbox/debug/nav/core_uart_nav_U%010lu.log",(unsigned long)millis());}
void snapshot(){
 if(!g_test.active||!g_file)return; const FT02GnssSnapshot g=FT02_GnssSnapshotCurrent(); const FT02CoreCompassSnapshot c=FT02_CompassCoreSnapshotCurrent(); char line[768];
 snprintf(line,sizeof(line),"NAV ms=%lu gnss_uart=%u fix=%u type=%u used=%u pdop=%.2f lat=%.7f lon=%.7f alt=%.1f speed=%.2f course=%.1f nmea_bytes=%lu compass_core=%u compass_q=%u",
 (unsigned long)millis(),g.communicationActive?1u:0u,g.fixValid?1u:0u,(unsigned)g.fixType,(unsigned)g.satellites,g.hdop,g.latitude,g.longitude,g.altitudeMeters,g.speedKmh,g.courseDegrees,(unsigned long)g.bytesReceived,c.ready?1u:0u,(unsigned)c.quality);
 if(writeLine(line))++g_test.snapshotCount;
}
}
void FT02_GnssFieldTestBegin(){memset(&g_test,0,sizeof(g_test));g_test.storageReady=FT02_StorageIsReady();snprintf(g_test.path,sizeof(g_test.path),"--");snprintf(g_test.lastAction,sizeof(g_test.lastAction),"T 开始Core GNSS记录");changed();}
bool FT02_GnssFieldTestStart(){if(g_test.active)return true;g_test.storageReady=FT02_StorageIsReady();if(!g_test.storageReady){action("无法记录：SD未就绪");return false;}buildPath();g_file=FT02_StorageOpenWriteFile(g_test.path,true);if(!g_file){++g_test.writeErrorCount;action("无法创建导航日志");return false;}g_test.active=true;g_test.startedMs=millis();g_test.durationSeconds=0;g_test.rawSentenceCount=0;g_test.snapshotCount=0;g_test.invalidSentenceCount=0;g_lastSnap=0;g_lastSync=millis();writeLine("# FT02 CORE UART GNSS + CORE I2C COMPASS STATE LOG A2");snapshot();fflush(g_file);action("Core GNSS记录中");return true;}
void FT02_GnssFieldTestStop(){if(!g_test.active)return;if(g_file){FT02_StorageSyncFile(g_file);fclose(g_file);g_file=nullptr;}g_test.active=false;action("Core GNSS记录已保存");}
void FT02_GnssFieldTestToggle(){if(g_test.active)FT02_GnssFieldTestStop();else FT02_GnssFieldTestStart();}
void FT02_GnssFieldTestPoll(){g_test.storageReady=FT02_StorageIsReady();if(!g_test.active)return;const uint32_t now=millis();g_test.durationSeconds=(now-g_test.startedMs)/1000UL;if(!g_lastSnap||now-g_lastSnap>=SNAP_MS){g_lastSnap=now;snapshot();changed();}if(g_file&&now-g_lastSync>=SYNC_MS){if(!FT02_StorageSyncFile(g_file)){++g_test.writeErrorCount;action("导航日志同步失败");}g_lastSync=now;}}
FT02GnssFieldTestSnapshot FT02_GnssFieldTestSnapshotCurrent(){auto x=g_test;if(x.active)x.durationSeconds=(millis()-x.startedMs)/1000UL;return x;}
