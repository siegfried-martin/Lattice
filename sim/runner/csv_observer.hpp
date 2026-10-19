#pragma once

#include <fstream>
#include <string>

#include "lattice/world.hpp"

namespace lattice {

// Writes series.csv (day, series, id, value) and events.csv (day, kind, id, detail)
// into a run's output directory (ARC-8). Series are kept every every_days days.
class CsvObserver : public Observer {
public:
    explicit CsvObserver(const std::string& dir, std::int64_t every_days = 1);

    void sample(Time t, std::string_view series, std::int64_t id, double value) override;
    void log(Time t, std::string_view kind, std::int64_t id, std::string_view detail) override;

    void flush();

private:
    std::ofstream series_;
    std::ofstream events_;
    std::int64_t every_days_;
};

// The generated map, for plotting: sectors.csv, planets.csv, lattice_nodes.csv and
// lattice_edges.csv.
void write_map(const World& world, const std::string& dir);

}  // namespace lattice
