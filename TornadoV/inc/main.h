#pragma once

#include <windows.h>
#include "natives.h"
#include "types.h"
#include "enums.h"

// Define WAIT if not defined
#ifndef WAIT
#define WAIT(ms) scriptWait(ms)
#endif