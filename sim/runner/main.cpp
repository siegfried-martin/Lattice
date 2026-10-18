// The headless runner (ARC-3): builds a world from a config and seed, or loads a save,
// runs it for some days and writes metrics.

#include <charconv>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "csv_observer.hpp"
#include "lattice/config.hpp"
#include "lattice/save.hpp"
#include "lattice/world.hpp"

namespace {

constexpr std::string_view kUsage = R"(usage: lattice-run [options]

  --config FILE   config to build a new world from (defaults otherwise)
  --seed N        world seed (default 1)
  --load FILE     continue a saved world instead (it carries its config and seed)
  --days N        game days to run (default 365)
  --out DIR       write config.toml, run.toml, series.csv and events.csv here
  --save FILE     save the world when the run ends
  --dump-config   print the full config, defaults included, and exit
)";

struct Options {
    std::optional<std::string> config, load, out, save;
    std::optional<std::uint64_t> seed;
    std::int64_t days = 365;
    bool dump_config = false;
};

template <class T>
T parse_number(std::string_view flag, std::string_view text) {
    T value{};
    auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc() || end != text.data() + text.size()) {
        throw std::runtime_error(std::string(flag) + " needs a number, got '" + std::string(text) + "'");
    }
    return value;
}

Options parse_args(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) throw std::runtime_error(std::string(arg) + " needs a value");
            return argv[++i];
        };
        if (arg == "--config") o.config = value();
        else if (arg == "--seed") o.seed = parse_number<std::uint64_t>(arg, value());
        else if (arg == "--load") o.load = value();
        else if (arg == "--days") o.days = parse_number<std::int64_t>(arg, value());
        else if (arg == "--out") o.out = value();
        else if (arg == "--save") o.save = value();
        else if (arg == "--dump-config") o.dump_config = true;
        else if (arg == "--help" || arg == "-h") {
            std::cout << kUsage;
            std::exit(0);
        } else {
            throw std::runtime_error("unknown option " + std::string(arg));
        }
    }
    if (o.load && (o.config || o.seed)) throw std::runtime_error("--load can't be combined with --config or --seed");
    if (o.days < 0) throw std::runtime_error("--days can't be negative");
    return o;
}

int run(const Options& o) {
    using namespace lattice;

    if (o.dump_config) {
        std::cout << to_toml(o.config ? load_config_file(*o.config) : Config{});
        return 0;
    }

    World world = o.load ? load_world_file(*o.load) : new_world(o.config ? load_config_file(*o.config) : Config{}, o.seed.value_or(1));
    const std::int64_t start_day = day_of(world.now);

    std::optional<CsvObserver> csv;
    if (o.out) {
        std::filesystem::create_directories(*o.out);
        std::ofstream(*o.out + "/config.toml") << to_toml(world.config);
        std::ofstream(*o.out + "/run.toml") << "seed = " << world.seed << "\n"
                                            << "start_day = " << start_day << "\n"
                                            << "days = " << o.days << "\n"
                                            << "save_version = " << kSaveVersion << "\n"
                                            << "loaded_from = \"" << o.load.value_or("") << "\"\n";
        csv.emplace(*o.out);
    }

    const auto t0 = std::chrono::steady_clock::now();
    advance(world, o.days * kDay, csv ? &*csv : nullptr);
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    if (csv) csv->flush();

    if (o.save) save_world_file(world, *o.save);

    std::printf("ran days %lld to %lld (seed %llu) in %.3f s, %.0f days/s, %llu events; state %016llx\n",
                static_cast<long long>(start_day), static_cast<long long>(day_of(world.now)),
                static_cast<unsigned long long>(world.seed), seconds,
                seconds > 0 ? static_cast<double>(o.days) / seconds : 0.0,
                static_cast<unsigned long long>(world.events_processed),
                static_cast<unsigned long long>(state_hash(world)));
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(parse_args(argc, argv));
    } catch (const std::exception& e) {
        std::cerr << "lattice-run: " << e.what() << "\n";
        return 1;
    }
}
