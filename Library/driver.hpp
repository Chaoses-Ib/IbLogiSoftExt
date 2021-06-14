#pragma once
#include <cstdint>

extern HANDLE device;
extern bool has_acceleration;

void DriverSend(INPUT inputs[], uint32_t n);

void DriverKeyboardSend(INPUT inputs[], uint32_t n);
void DriverSyncKeyStates();
SHORT DriverGetKeyState(int vKey, decltype(GetAsyncKeyState) f);

void DriverMouseSend(INPUT inputs[], uint32_t n);