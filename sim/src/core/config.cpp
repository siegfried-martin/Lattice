#include "lattice/config.hpp"

#include <fstream>
#include <set>
#include <sstream>
#include <type_traits>

#include <toml.hpp>

namespace lattice {

namespace {

std::string where(std::string_view source, const toml::node& node) {
    std::ostringstream out;
    out << source << ":" << node.source().begin.line;
    return out.str();
}

// Collects every leaf key, dotted, so unknown ones can be reported.
void collect_keys(const toml::table& table, const std::string& prefix,
                  std::set<std::string>& out) {
    for (auto&& [key, node] : table) {
        std::string path = prefix.empty() ? std::string(key.str()) : prefix + "." + std::string(key.str());
        if (const toml::table* sub = node.as_table()) {
            collect_keys(*sub, path, out);
        } else {
            out.insert(path);
        }
    }
}

// Inserts value at a dotted path, creating the tables on the way.
template <class T>
void insert_path(toml::table& root, std::string_view path, T value) {
    toml::table* table = &root;
    std::size_t start = 0;
    for (std::size_t dot; (dot = path.find('.', start)) != std::string_view::npos; start = dot + 1) {
        auto [it, _] = table->insert(path.substr(start, dot - start), toml::table{});
        table = it->second.as_table();
    }
    table->insert_or_assign(path.substr(start), value);
}

}  // namespace

Config parse_config(std::string_view toml_text, std::string_view source) {
    toml::table table;
    try {
        table = toml::parse(toml_text, source);
    } catch (const toml::parse_error& e) {
        std::ostringstream out;
        out << source << ":" << e.source().begin.line << ": " << e.description();
        throw ConfigError(out.str());
    }

    Config config;
    std::set<std::string> known;
    for_each_field(config, [&](std::string_view key, auto& field) {
        known.insert(std::string(key));
        toml::node_view node = table.at_path(key);
        if (!node) return;
        using T = std::remove_reference_t<decltype(field)>;
        if constexpr (std::is_same_v<T, double>) {
            if (auto f = node.as_floating_point()) {
                field = f->get();
            } else if (auto i = node.as_integer()) {
                field = static_cast<double>(i->get());
            } else {
                throw ConfigError(where(source, *node.node()) + ": " + std::string(key) + " must be a number");
            }
        } else if constexpr (std::is_same_v<T, bool>) {
            auto b = node.as_boolean();
            if (!b) throw ConfigError(where(source, *node.node()) + ": " + std::string(key) + " must be true or false");
            field = b->get();
        } else {
            static_assert(std::is_same_v<T, std::int64_t>, "add parsing for this field type");
            auto i = node.as_integer();
            if (!i) throw ConfigError(where(source, *node.node()) + ": " + std::string(key) + " must be an integer");
            field = i->get();
        }
    });

    std::set<std::string> present;
    collect_keys(table, "", present);
    for (const std::string& key : present) {
        if (!known.contains(key)) {
            throw ConfigError(where(source, *table.at_path(key).node()) + ": unknown key " + key);
        }
    }

    validate(config);
    return config;
}

Config load_config_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw ConfigError("cannot open config " + path);
    std::ostringstream text;
    text << in.rdbuf();
    return parse_config(text.str(), path);
}

void validate(const Config& c) {
    auto require = [](bool ok, const char* message) {
        if (!ok) throw ConfigError(message);
    };
    require(c.time.real_seconds_per_day > 0, "time.real_seconds_per_day must be above 0");
    require(c.time.space_step_minutes > 0, "time.space_step_minutes must be above 0");
    require(c.time.battle_round_hours > 0, "time.battle_round_hours must be above 0");
    require(c.time.labour_days > 0, "time.labour_days must be above 0");
    require(c.time.budget_days > 0, "time.budget_days must be above 0");
    require(c.time.diplomacy_days > 0, "time.diplomacy_days must be above 0");
    require(c.time.payment_days > 0, "time.payment_days must be above 0");
    require(c.debug.pings >= 0 && c.debug.pings <= 1'000'000, "debug.pings must be 0 to 1,000,000");
}

std::string to_toml(const Config& config) {
    toml::table table;
    for_each_field(config, [&](std::string_view key, const auto& field) { insert_path(table, key, field); });
    std::ostringstream out;
    out << table << "\n";
    return out.str();
}

}  // namespace lattice
