#pragma once
#include <cstdint>

void DriverKeyboardSend(HANDLE device, INPUT inputs[], uint32_t n);
void DriverSyncKeyStates();
SHORT DriverGetAsyncKeyState(int vKey, decltype(GetAsyncKeyState) f);