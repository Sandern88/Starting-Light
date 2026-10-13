#include "AppStatus.h"

namespace AppStatus {
namespace {
bool gPirRawHigh = false;
bool gPirStableHigh = false;
}  // namespace

void setPirState(bool rawHigh, bool stableHigh) {
  gPirRawHigh = rawHigh;
  gPirStableHigh = stableHigh;
}

bool pirRawHigh() { return gPirRawHigh; }

bool pirStableHigh() { return gPirStableHigh; }
}  // namespace AppStatus
