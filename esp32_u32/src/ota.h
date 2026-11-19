#pragma once

// OTA (WiFi-s) firmware frissítés, ArduinoOTA.
void otaBegin(const char *hostname);  // WiFi csatlakozás UTÁN hívd (egyszer)
void otaLoop();                       // hívd a loop()-ból
bool otaActive();                     // true, amíg frissítés folyik