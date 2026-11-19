#pragma once
#include "rocnet.h"

typedef void (*RnRxCallback)(const RnPacket &);

void netBegin(RnRxCallback cb);  // WiFi indítás, a callback minden fogadott (nem saját) csomagra hívódik
void netLoop();                  // hívd a loop()-ból
bool netReady();
bool netSend(RnPacket &p);       // a sndr mezőt a híd ID-jára állítja