#pragma once

#include <cstdint>

typedef struct {
  int16_t Type;
  int16_t SFXIndex;
  double Pan;
  double SleepDuration;
  wchar_t *Text;
} Command;
