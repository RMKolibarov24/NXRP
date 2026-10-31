#pragma once

#include <string>

#include "GameStatistics.h"

// Total minutes and two-digit seconds. Fractional seconds remain in the model.
std::string gameStatisticsText(const GameStatistics& statistics);
