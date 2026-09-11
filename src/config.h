#pragma once

// Compile-time developer extras (not persisted, not a user setting).
// Set to 0 for release builds. Override with -DDEV_MODE=0 if needed.
#ifndef DEV_MODE
#define DEV_MODE 1
#endif
