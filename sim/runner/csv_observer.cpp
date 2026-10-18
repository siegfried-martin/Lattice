#include "csv_observer.hpp"

#include <algorithm>
#include <format>
#include <stdexcept>

namespace lattice {

namespace {

std::string day_text(Time t) {
    if (t % kDay == 0) return std::to_string(t / kDay);
    return std::format("{}", static_cast<double>(t) / static_cast<double>(kDay));
}

// Quotes a field if it holds a comma, quote or newline.
std::string csv_field(std::string_view s) {
    if (s.find_first_of(",\"\n") == std::string_view::npos) return std::string(s);
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += '"';
        out += c;
    }
    return out + "\"";
}

}  // namespace

CsvObserver::CsvObserver(const std::string& dir, std::int64_t every_days)
    : series_(dir + "/series.csv"), events_(dir + "/events.csv"), every_days_(every_days < 1 ? 1 : every_days) {
    if (!series_ || !events_) throw std::runtime_error("cannot write metrics in " + dir);
    series_ << "day,series,id,value\n";
    events_ << "day,kind,id,detail\n";
}

void CsvObserver::sample(Time t, std::string_view series, std::int64_t id, double value) {
    if (day_of(t) % every_days_ != 0) return;
    series_ << day_text(t) << ',' << series << ',' << id << ',' << std::format("{}", value) << '\n';
}

void CsvObserver::log(Time t, std::string_view kind, std::int64_t id, std::string_view detail) {
    events_ << day_text(t) << ',' << kind << ',' << id << ',' << csv_field(detail) << '\n';
}

void CsvObserver::flush() {
    series_.flush();
    events_.flush();
}

void write_map(const World& w, const std::string& dir) {
    std::ofstream sectors(dir + "/sectors.csv"), planets(dir + "/planets.csv");
    std::ofstream nodes(dir + "/lattice_nodes.csv"), edges(dir + "/lattice_edges.csv");
    if (!sectors || !planets || !nodes || !edges) throw std::runtime_error("cannot write the map in " + dir);

    sectors << "id,col,row,x,y,region,system\n";
    for (std::size_t i = 0; i < w.map.sectors.size(); ++i) {
        const Sector& s = w.map.sectors[i];
        sectors << i << ',' << s.col << ',' << s.row << ',' << s.x << ',' << s.y << ',' << s.region << ',' << s.system << '\n';
    }

    planets << "id,sector,region,system,group,moon_of,x,y,fringe,holder,seat,good1,tier1,good2,tier2,good3,tier3,fuel_price,population\n";
    constexpr const char* tiers[] = {"high", "medium", "low"};
    for (std::size_t i = 0; i < w.planets.size(); ++i) {
        const Planet& p = w.planets[i];
        const auto id = static_cast<Id>(i);
        const bool holder = w.map.groups[static_cast<std::size_t>(p.group)].holder == id;
        const bool seat = std::find(w.map.seats.begin(), w.map.seats.end(), id) != w.map.seats.end();
        planets << i << ',' << p.sector << ',' << p.region << ',' << p.system << ',' << p.group << ',' << p.moon_of << ','
                << p.x << ',' << p.y << ',' << p.fringe << ',' << holder << ',' << seat;
        for (int s = 0; s < 3; ++s) {
            planets << ',' << good_name(p.makes[static_cast<std::size_t>(s)]) << ',' << tiers[p.tiers[static_cast<std::size_t>(s)]];
        }
        planets << ',' << p.fuel_price << ',' << p.population << '\n';
    }

    nodes << "id,kind,x,y,group\n";
    for (std::size_t i = 0; i < w.map.nodes.size(); ++i) {
        const LatticeNode& n = w.map.nodes[i];
        nodes << i << ',' << (n.kind == NodeKind::Junction ? "junction" : "interchange") << ',' << n.x << ',' << n.y << ','
              << n.group << '\n';
    }
    edges << "id,a,b,length\n";
    for (std::size_t i = 0; i < w.map.edges.size(); ++i) {
        const LatticeEdge& e = w.map.edges[i];
        edges << i << ',' << e.a << ',' << e.b << ',' << e.length << '\n';
    }
}

}  // namespace lattice
