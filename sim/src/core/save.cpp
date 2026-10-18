#include "lattice/save.hpp"

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

private:
    void need(std::uint64_t n) const {
        if (n > remaining()) throw SaveError("save is truncated");
    }

    std::span<const std::uint8_t> bytes_;
    std::size_t pos_ = 0;
};

enum class FieldType : std::uint8_t { Integer = 0, Number = 1, Boolean = 2 };

void write_config(Writer& w, const Config& config) {
    std::uint64_t count = 0;
    for_each_field(config, [&](std::string_view, const auto&) { ++count; });
    w.put(count);
    for_each_field(config, [&](std::string_view key, const auto& field) {
        using T = std::remove_cvref_t<decltype(field)>;
        w.put_string(std::string(key));
        if constexpr (std::is_same_v<T, double>) {
            w.put(FieldType::Number);
            w.put_double(field);
        } else if constexpr (std::is_same_v<T, bool>) {
            w.put(FieldType::Boolean);
            w.put(static_cast<std::uint64_t>(field));
        } else {
            static_assert(std::is_same_v<T, std::int64_t>);
            w.put(FieldType::Integer);
            w.put(field);
        }
    });
}

Config read_config(Reader& r) {
    struct Stored {
        FieldType type;
        std::uint64_t bits;
    };
    std::map<std::string, Stored> stored;
    const auto count = r.get_count(10);
    for (std::uint64_t i = 0; i < count; ++i) {
        std::string key = r.get_string();
        const auto type = r.get<FieldType>();
        stored[key] = {type, r.get<std::uint64_t>()};
    }

    Config config;
    for_each_field(config, [&](std::string_view key, auto& field) {
        auto it = stored.find(std::string(key));
        if (it == stored.end()) return;  // newer tunable: keep the default
        using T = std::remove_cvref_t<decltype(field)>;
        const FieldType want = std::is_same_v<T, double> ? FieldType::Number
                             : std::is_same_v<T, bool>   ? FieldType::Boolean
                                                         : FieldType::Integer;
        if (it->second.type != want) throw SaveError("save has the wrong type for config " + it->first);
        if constexpr (std::is_same_v<T, double>) {
            field = std::bit_cast<double>(it->second.bits);
        } else if constexpr (std::is_same_v<T, bool>) {
            field = it->second.bits != 0;
        } else {
            field = static_cast<std::int64_t>(it->second.bits);
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

void write_event(Writer& w, const Event& e) {
    w.put(e.time);
    w.put(e.phase);
    w.put(e.seq);
    w.put(e.kind);
    w.put(e.record);
    w.put(e.period);
    w.put(e.a);
    w.put(e.b);
}

Event read_event(Reader& r) {
    Event e;
    e.time = r.get<Time>();
    e.phase = r.get<Phase>();
    e.seq = r.get<std::uint64_t>();
    e.kind = r.get<EventKind>();
    e.record = r.get<std::int64_t>();
    e.period = r.get<Time>();
    e.a = r.get<std::int64_t>();
    e.b = r.get<std::int64_t>();
    return e;
}

constexpr std::size_t kEventBytes = 8 + 1 + 8 + 2 + 8 + 8 + 8 + 8;

}  // namespace

std::vector<std::uint8_t> save_world(const World& world) {
    Writer p;
    write_config(p, world.config);
    p.put(world.seed);
    p.put(world.now);
    p.put(world.events_processed);

    p.put(static_cast<std::uint64_t>(world.rng.streams.size()));
    for (const Rng& rng : world.rng.streams) p.put_string(rng.state());

    p.put(world.events.next_seq());
    p.put(static_cast<std::uint64_t>(world.events.size()));
    for (const Event& e : world.events.raw()) write_event(p, e);

    p.put(static_cast<std::uint64_t>(world.ping_fires.size()));
    for (std::int64_t n : world.ping_fires) p.put(n);

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
    world.seed = r.get<std::uint64_t>();
    world.now = r.get<Time>();
    world.events_processed = r.get<std::uint64_t>();

    if (r.get<std::uint64_t>() != world.rng.streams.size()) throw SaveError("save has a different number of RNG streams");
    for (Rng& rng : world.rng.streams) {
        try {
            rng.set_state(r.get_string());
        } catch (const std::runtime_error&) {
            throw SaveError("save has an invalid RNG state");
        }
    }

    const auto next_seq = r.get<std::uint64_t>();
    const auto event_count = r.get_count(kEventBytes);
    std::vector<Event> events;
    events.reserve(event_count);
    for (std::uint64_t i = 0; i < event_count; ++i) events.push_back(read_event(r));
    world.events.restore(std::move(events), next_seq);

    const auto pings = r.get_count(8);
    world.ping_fires.resize(pings);
    for (auto& n : world.ping_fires) n = r.get<std::int64_t>();

    if (r.remaining() != 0) throw SaveError("save has trailing data");
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
