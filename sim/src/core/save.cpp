#include "lattice/save.hpp"

#include <array>
#include <bit>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <type_traits>

namespace lattice {

namespace {

// Layout: magic, version, payload size, payload, FNV-1a hash of the payload.
// Every integer is little-endian whatever the host, and doubles go as their bits.
constexpr char kMagic[8] = {'L', 'A', 'T', 'T', 'S', 'A', 'V', 'E'};

std::uint64_t fnv1a(std::span<const std::uint8_t> bytes) {
    std::uint64_t h = 0xcbf29ce484222325ULL;
    for (std::uint8_t b : bytes) {
        h ^= b;
        h *= 0x100000001b3ULL;
    }
    return h;
}

template <class T>
struct is_vector : std::false_type {};
template <class T>
struct is_vector<std::vector<T>> : std::true_type {};
template <class T>
struct is_array : std::false_type {};
template <class T, std::size_t N>
struct is_array<std::array<T, N>> : std::true_type {};

class Writer {
public:
    template <class T>
    void put(T v) {
        static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
        auto u = static_cast<std::make_unsigned_t<
            typename std::conditional_t<std::is_enum_v<T>, std::underlying_type<T>, std::type_identity<T>>::type>>(v);
        for (std::size_t i = 0; i < sizeof(T); ++i) bytes.push_back(static_cast<std::uint8_t>(u >> (8 * i)));
    }
    void put_double(double v) { put(std::bit_cast<std::uint64_t>(v)); }
    void put_string(const std::string& s) {
        put(static_cast<std::uint64_t>(s.size()));
        bytes.insert(bytes.end(), s.begin(), s.end());
    }

    // Writes any saved value: numbers, enums, strings, vectors, arrays, and records
    // through their fields() list (model.hpp).
    template <class T>
    void operator()(const T& v) {
        if constexpr (std::is_same_v<T, bool>) {
            put(static_cast<std::uint8_t>(v));
        } else if constexpr (std::is_same_v<T, double>) {
            put_double(v);
        } else if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
            put(v);
        } else if constexpr (std::is_same_v<T, std::string>) {
            put_string(v);
        } else if constexpr (is_vector<T>::value) {
            put(static_cast<std::uint64_t>(v.size()));
            for (const auto& x : v) (*this)(x);
        } else if constexpr (is_array<T>::value) {
            for (const auto& x : v) (*this)(x);
        } else {
            fields(*this, v);
        }
    }
    template <class... T>
        requires(sizeof...(T) > 1)
    void operator()(const T&... v) {
        ((*this)(v), ...);
    }

    std::vector<std::uint8_t> bytes;
};

class Reader {
public:
    explicit Reader(std::span<const std::uint8_t> b) : bytes_(b) {}

    template <class T>
    T get() {
        static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
        using U = std::make_unsigned_t<
            typename std::conditional_t<std::is_enum_v<T>, std::underlying_type<T>, std::type_identity<T>>::type>;
        need(sizeof(T));
        U u = 0;
        for (std::size_t i = 0; i < sizeof(T); ++i) u |= static_cast<U>(static_cast<U>(bytes_[pos_ + i]) << (8 * i));
        pos_ += sizeof(T);
        return static_cast<T>(u);
    }
    double get_double() { return std::bit_cast<double>(get<std::uint64_t>()); }
    std::string get_string() {
        const auto n = get<std::uint64_t>();
        need(n);
        std::string s(reinterpret_cast<const char*>(bytes_.data() + pos_), n);
        pos_ += n;
        return s;
    }
    // A count of items that each take at least min_bytes, checked against what's left
    // so a corrupt count can't trigger a huge allocation.
    std::uint64_t get_count(std::size_t min_bytes) {
        const auto n = get<std::uint64_t>();
        if (n > remaining() / min_bytes) throw SaveError("save is corrupt: count too large");
        return n;
    }
    std::size_t remaining() const { return bytes_.size() - pos_; }

    // Reads what Writer::operator() wrote.
    template <class T>
    void operator()(T& v) {
        if constexpr (std::is_same_v<T, bool>) {
            const auto b = get<std::uint8_t>();
            if (b > 1) throw SaveError("save is corrupt: bad flag");
            v = b != 0;
        } else if constexpr (std::is_same_v<T, double>) {
            v = get_double();
        } else if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
            v = get<T>();
        } else if constexpr (std::is_same_v<T, std::string>) {
            v = get_string();
        } else if constexpr (is_vector<T>::value) {
            v.resize(get_count(1));
            for (auto& x : v) (*this)(x);
        } else if constexpr (is_array<T>::value) {
            for (auto& x : v) (*this)(x);
        } else {
            fields(*this, v);
        }
    }
    template <class... T>
        requires(sizeof...(T) > 1)
    void operator()(T&... v) {
        ((*this)(v), ...);
    }

private:
    void need(std::uint64_t n) const {
        if (n > remaining()) throw SaveError("save is truncated");
    }

    std::span<const std::uint8_t> bytes_;
    std::size_t pos_ = 0;
};

enum class FieldType : std::uint8_t { Integer = 0, Number = 1, Boolean = 2, NumberList = 3 };

template <class T>
constexpr FieldType field_type() {
    if constexpr (std::is_same_v<T, double>) return FieldType::Number;
    else if constexpr (std::is_same_v<T, bool>) return FieldType::Boolean;
    else if constexpr (is_array<T>::value) return FieldType::NumberList;
    else return FieldType::Integer;
}

// Config is stored by key, as (key, type, values), so a tunable added later loads from
// an older save with its default (ARC-9).
void write_config(Writer& w, const Config& config) {
    std::uint64_t count = 0;
    for_each_field(config, [&](std::string_view, const auto&) { ++count; });
    w.put(count);
    for_each_field(config, [&](std::string_view key, const auto& field) {
        using T = std::remove_cvref_t<decltype(field)>;
        w.put_string(std::string(key));
        w.put(field_type<T>());
        if constexpr (is_array<T>::value) {
            w(std::vector<double>(field.begin(), field.end()));
        } else if constexpr (std::is_same_v<T, double>) {
            w(std::vector<std::uint64_t>{std::bit_cast<std::uint64_t>(field)});
        } else {
            static_assert(std::is_same_v<T, bool> || std::is_same_v<T, std::int64_t>);
            w(std::vector<std::uint64_t>{static_cast<std::uint64_t>(field)});
        }
    });
}

Config read_config(Reader& r) {
    struct Stored {
        FieldType type;
        std::vector<std::uint64_t> bits;
    };
    std::map<std::string, Stored> stored;
    const auto count = r.get_count(10);
    for (std::uint64_t i = 0; i < count; ++i) {
        std::string key = r.get_string();
        Stored s;
        s.type = r.get<FieldType>();
        r(s.bits);
        stored[key] = std::move(s);
    }

    Config config;
    for_each_field(config, [&](std::string_view key, auto& field) {
        auto it = stored.find(std::string(key));
        if (it == stored.end()) return;  // newer tunable: keep the default
        using T = std::remove_cvref_t<decltype(field)>;
        const auto& bits = it->second.bits;
        if (it->second.type != field_type<T>()) throw SaveError("save has the wrong type for config " + it->first);
        if constexpr (is_array<T>::value) {
            if (bits.size() != field.size()) throw SaveError("save has the wrong length for config " + it->first);
            for (std::size_t i = 0; i < field.size(); ++i) field[i] = std::bit_cast<double>(bits[i]);
        } else {
            if (bits.size() != 1) throw SaveError("save is corrupt: config " + it->first);
            if constexpr (std::is_same_v<T, double>) {
                field = std::bit_cast<double>(bits[0]);
            } else if constexpr (std::is_same_v<T, bool>) {
                field = bits[0] != 0;
            } else {
                field = static_cast<std::int64_t>(bits[0]);
            }
        }
        stored.erase(it);
    });
    if (!stored.empty()) throw SaveError("save has unknown config " + stored.begin()->first);
    try {
        validate(config);
    } catch (const ConfigError& e) {
        throw SaveError(std::string("save has invalid config: ") + e.what());
    }
    return config;
}

// Indices in a loaded save must point at records that exist, so a save written by a
// different build (or crafted) fails here rather than reading out of bounds.
void check_references(const World& w) {
    auto in = [](auto id, std::size_t n) { return id >= 0 && static_cast<std::size_t>(id) < n; };
    auto opt = [&](auto id, std::size_t n) { return id == kNone || in(id, n); };
    auto require = [](bool ok, const char* what) {
        if (!ok) throw SaveError(std::string("save is inconsistent: ") + what);
    };
    const Map& m = w.map;
    const auto np = w.planets.size();
    require(m.width > 0 && m.height > 0 && m.sectors.size() == static_cast<std::size_t>(m.width) * static_cast<std::size_t>(m.height), "sectors");
    for (const Sector& s : m.sectors) require(in(s.region, m.regions.size()) && opt(s.system, m.systems.size()), "sector");
    for (const System& s : m.systems) {
        require(in(s.sector, m.sectors.size()), "system");
        for (Id p : s.planets) require(in(p, np), "system planet");
    }
    for (const Group& g : m.groups) {
        require(in(g.system, m.systems.size()) && in(g.holder, np) && opt(g.node, m.nodes.size()), "group");
        for (Id p : g.planets) require(in(p, np), "group planet");
    }
    for (const LatticeNode& n : m.nodes) {
        require(opt(n.group, m.groups.size()), "node");
        for (Id e : n.edges) require(in(e, m.edges.size()), "node edge");
    }
    for (const LatticeEdge& e : m.edges) require(in(e.a, m.nodes.size()) && in(e.b, m.nodes.size()), "edge");
    for (const HopLane& h : m.hop_lanes) require(in(h.a, np) && in(h.b, np), "hop lane");
    for (Id s : m.seats) require(in(s, np), "seat");
    for (const Planet& p : w.planets) {
        require(in(p.sector, m.sectors.size()) && in(p.region, m.regions.size()) && in(p.system, m.systems.size()) &&
                    in(p.group, m.groups.size()) && opt(p.moon_of, np),
                "planet");
        for (int s = 0; s < 3; ++s) {
            require(p.makes[static_cast<std::size_t>(s)] < kGoodCount && p.tiers[static_cast<std::size_t>(s)] <= kLow, "planet goods");
        }
    }
    for (const WorldEvent& e : w.world_events) {
        require(e.good < kGoodCount && opt(e.region, m.regions.size()), "world event");
        for (Id p : e.planets) require(in(p, np), "world event planet");
    }
    const auto nc = w.characters.size();
    auto facts_ok = [&](const Facts& f) {
        for (const PriceFact& pf : f.prices) require(in(pf.planet, np), "fact");
        for (const RiskFact& rf : f.risks) require(in(rf.sector, m.sectors.size()), "risk fact");
        for (const DipFact& df : f.dips) require(in(df.a, np) && in(df.b, np) && df.state <= 2, "war fact");
        for (const Sighting& s : f.sightings) {
            require(in(s.army, w.armies.size()) && opt(s.target, np) && opt(s.faction, np), "sighting");
        }
    };
    for (const ShipVariant& v : m.variants) require(in(v.culture, m.regions.size()), "ship variant");
    for (const Planet& p : w.planets) {
        require(opt(p.governor, nc) && opt(p.liege, np), "planet politics");
        for (Id mi : p.missions) require(in(mi, w.missions.size()), "mission board");
    }
    for (const Character& c : w.characters) {
        require(in(c.culture, m.regions.size()) && opt(c.planet, np) && opt(c.last_planet, np) && opt(c.governs, np) &&
                    opt(c.mission, w.missions.size()) && c.cargo_good <= kGoodCount,
                "character");
        for (Id v : c.ships) require(in(v, m.variants.size()), "character ship");
        for (const Opinion& o : c.opinions) {
            require(in(o.of, nc), "opinion");
            for (const Deed& d : o.recent) require(static_cast<int>(d.kind) < kDeedKindCount, "deed");
        }
        for (Id k : c.contracts) require(in(k, w.contracts.size()), "character contract");
        for (const TripStop& t : c.trip) require(in(t.planet, np) && t.buy <= kGoodCount, "trip");
        for (const Leg& l : c.route) require(opt(l.to_planet, np) && opt(l.to_node, m.nodes.size()), "route");
        require(c.route.empty() || in(c.leg, c.route.size()), "route leg");
        facts_ok(c.facts);
    }
    for (const Contract& k : w.contracts) {
        require(in(k.employer, nc) && in(k.employee, nc) && opt(k.mission, w.missions.size()), "contract");
    }
    for (const Mission& mi : w.missions) {
        require(in(mi.poster, nc) && in(mi.at, np) && in(mi.target, np) && opt(mi.taker, nc), "mission");
        facts_ok(mi.packet);
    }
    for (const Planet& p : w.planets) require(opt(p.siege, w.sieges.size()), "planet siege");
    for (const Character& c : w.characters) {
        require(opt(c.army, w.armies.size()) && opt(c.battle, w.battles.size()) && opt(c.escort, nc) && opt(c.escorting, nc),
                "character war");
    }
    for (const Planet& p : w.planets) require(opt(p.warned_army, w.armies.size()), "planet warning");
    require(w.risk.size() == m.sectors.size(), "sector risk");
    for (const Relation& r : w.relations) {
        require(in(r.a, np) && in(r.b, np) && opt(r.payer, np) && opt(r.payee, np) && static_cast<int>(r.state) <= 2, "relation");
    }
    for (const Army& a : w.armies) {
        require(in(a.commissioner, nc) && in(a.home, np) && opt(a.target, np) && opt(a.planet, np) &&
                    opt(a.battle, w.battles.size()) && opt(a.siege, w.sieges.size()),
                "army");
        for (Id member : a.members) require(in(member, nc), "army member");
        for (Id member : a.called) require(in(member, nc), "army called");
        for (const Leg& l : a.route) require(opt(l.to_planet, np) && opt(l.to_node, m.nodes.size()), "army route");
        require(a.route.empty() || in(a.leg, a.route.size()), "army leg");
    }
    for (const Battle& b : w.battles) {
        require(opt(b.planet, np) && opt(b.armies[0], w.armies.size()) && opt(b.armies[1], w.armies.size()) && b.loot_good <= kGoodCount,
                "battle");
        for (const auto& side : b.sides) {
            for (Id c : side) require(in(c, nc), "battle side");
        }
    }
    for (const Siege& s : w.sieges) require(in(s.planet, np) && in(s.army, w.armies.size()), "siege");
    for (const CourierRun& r : w.courier_runs) {
        require(in(r.from, np) && in(r.to, np), "courier run");
        facts_ok(r.packet);
    }
    for (const Event& e : w.events.raw()) {
        switch (e.kind) {
            case EventKind::DailyTick:
            case EventKind::KinematicStep: break;
            case EventKind::DebugPing: require(in(e.record, w.ping_fires.size()), "ping event"); break;
            case EventKind::PlanetDay:
            case EventKind::PlanetLabour:
            case EventKind::PlanetPayment:
            case EventKind::CourierDue:
            case EventKind::GovernorWeekly: require(in(e.record, np), "planet event"); break;
            case EventKind::WorldEventEnd: require(in(e.record, w.world_events.size()), "world event end"); break;
            case EventKind::CharDecide:
            case EventKind::CharArrive: require(in(e.record, nc), "character event"); break;
            case EventKind::CourierArrive: require(in(e.record, w.courier_runs.size()), "courier event"); break;
            case EventKind::MissionExpire: require(in(e.record, w.missions.size()), "mission event"); break;
            case EventKind::ArmyMuster:
            case EventKind::ArmyArrive: require(in(e.record, w.armies.size()), "army event"); break;
            case EventKind::BattleRound: require(in(e.record, w.battles.size()), "battle event"); break;
            case EventKind::SiegeRound: require(in(e.record, w.sieges.size()), "siege event"); break;
            case EventKind::CallForHelp: require(in(e.record, w.sieges.size()) && in(e.a, np), "call event"); break;
            case EventKind::ThreatWarning: require(in(e.record, w.armies.size()) && in(e.a, np) && in(e.b, np), "warning event"); break;
            default: require(false, "event kind");
        }
    }
}

// The world after its config, in save order. Derived values aren't saved.
template <class Ar, class W>
void world_fields(Ar& ar, W& w) {
    ar(w.seed, w.now, w.events_processed, w.ping_fires, w.map, w.planets, w.world_events, w.characters, w.contracts,
       w.missions, w.courier_runs, w.relations, w.armies, w.battles, w.sieges, w.risk, w.authority, w.ledger);
}

}  // namespace

std::vector<std::uint8_t> save_world(const World& world) {
    Writer p;
    write_config(p, world.config);

    p.put(static_cast<std::uint64_t>(world.rng.streams.size()));
    for (const Rng& rng : world.rng.streams) p.put_string(rng.state());
    p.put(world.events.next_seq());
    p(world.events.raw());

    world_fields(p, world);

    Writer out;
    out.bytes.insert(out.bytes.end(), std::begin(kMagic), std::end(kMagic));
    out.put(kSaveVersion);
    out.put(static_cast<std::uint64_t>(p.bytes.size()));
    out.bytes.insert(out.bytes.end(), p.bytes.begin(), p.bytes.end());
    out.put(fnv1a(p.bytes));
    return std::move(out.bytes);
}

World load_world(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < sizeof(kMagic) || std::memcmp(bytes.data(), kMagic, sizeof(kMagic)) != 0) {
        throw SaveError("not a Lattice save");
    }
    Reader header(bytes.subspan(sizeof(kMagic)));
    const auto version = header.get<std::uint32_t>();
    if (version != kSaveVersion) {
        throw SaveError("save version " + std::to_string(version) + ", this build reads version " +
                        std::to_string(kSaveVersion));
    }
    const auto size = header.get<std::uint64_t>();
    if (size + 8 != header.remaining()) throw SaveError("save is truncated");
    const auto payload = bytes.subspan(sizeof(kMagic) + 4 + 8, size);
    Reader tail(bytes.subspan(sizeof(kMagic) + 4 + 8 + size));
    if (tail.get<std::uint64_t>() != fnv1a(payload)) throw SaveError("save is corrupt: hash mismatch");

    Reader r(payload);
    World world;
    world.config = read_config(r);

    if (r.get<std::uint64_t>() != world.rng.streams.size()) throw SaveError("save has a different number of RNG streams");
    for (Rng& rng : world.rng.streams) {
        try {
            rng.set_state(r.get_string());
        } catch (const std::runtime_error&) {
            throw SaveError("save has an invalid RNG state");
        }
    }
    const auto next_seq = r.get<std::uint64_t>();
    std::vector<Event> events;
    r(events);
    world.events.restore(std::move(events), next_seq);

    world_fields(r, world);
    if (r.remaining() != 0) throw SaveError("save has trailing data");
    check_references(world);
    rebuild_derived(world);
    return world;
}

void save_world_file(const World& world, const std::string& path) {
    const auto bytes = save_world(world);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw SaveError("cannot write save " + path);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!out) throw SaveError("cannot write save " + path);
}

World load_world_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw SaveError("cannot open save " + path);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return load_world(bytes);
}

std::uint64_t state_hash(const World& world) {
    return fnv1a(save_world(world));
}

}  // namespace lattice
