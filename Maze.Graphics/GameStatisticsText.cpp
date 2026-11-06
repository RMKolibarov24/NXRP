#include "GameStatisticsText.h"

#include <iomanip>
#include <sstream>

std::string gameStatisticsText(const GameStatistics& statistics)
{
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(statistics.elapsed).count();
    std::ostringstream text;
    text << "Time " << std::setfill('0') << std::setw(2) << seconds / 60
        << ':' << std::setw(2) << seconds % 60 << "  |  Moves " << statistics.successfulMoves;
    return text.str();
}
