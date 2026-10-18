#include "csv_observer.hpp"

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

CsvObserver::CsvObserver(const std::string& dir)
    : series_(dir + "/series.csv"), events_(dir + "/events.csv") {
    if (!series_ || !events_) throw std::runtime_error("cannot write metrics in " + dir);
    series_ << "day,series,id,value\n";
    events_ << "day,kind,id,detail\n";
}

void CsvObserver::sample(Time t, std::string_view series, std::int64_t id, double value) {
    series_ << day_text(t) << ',' << series << ',' << id << ',' << std::format("{}", value) << '\n';
}

void CsvObserver::log(Time t, std::string_view kind, std::int64_t id, std::string_view detail) {
    events_ << day_text(t) << ',' << kind << ',' << id << ',' << csv_field(detail) << '\n';
}

void CsvObserver::flush() {
    series_.flush();
    events_.flush();
}

}  // namespace lattice
