#pragma once

#include <fstream>
#include <string>

#include "lattice/world.hpp"

namespace lattice {

// Writes series.csv (day, series, id, value) and events.csv (day, kind, id, detail)
// into a run's output directory (ARC-8).
class CsvObserver : public Observer {
public:
    explicit CsvObserver(const std::string& dir);

    void sample(Time t, std::string_view series, std::int64_t id, double value) override;
    void log(Time t, std::string_view kind, std::int64_t id, std::string_view detail) override;

    void flush();

private:
    std::ofstream series_;
    std::ofstream events_;
};

}  // namespace lattice
