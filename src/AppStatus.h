#pragma once

namespace AppStatus {
void setPirState(bool rawHigh, bool stableHigh);
bool pirRawHigh();
bool pirStableHigh();
}  // namespace AppStatus
