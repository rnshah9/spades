#pragma once

#include <istream>
#include <array>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <type_traits>
#include <tuple>

enum class Columns : size_t {
    match       = 0,
    mismatch    = 1,
    rep_match   = 2,
    Ns          = 3,
    Q_gap_count = 4,
    Q_gap_bases = 5,
    T_gap_count = 6,
    T_gap_bases = 7,
    strand      = 8,
    Q_name      = 9,
    Q_size      = 10,
    Q_start     = 11,
    Q_end       = 12,
    T_name      = 13,
    T_size      = 14,
    T_start     = 15,
    T_end       = 16,
    block_count = 17,
    blockSizes  = 18,
    qStarts     = 19,
    tStarts     = 20,
    TOTAL_COLUMNS_SIZE = 21
};

template<Columns el>
struct type_getter{
    using type = std::conditional_t<
                        el == Columns::Q_name ||
                        el == Columns::T_name ||
                        el == Columns::blockSizes ||
                        el == Columns::qStarts ||
                        el == Columns::tStarts,
                        std::string,
                 std::conditional_t<
                        el == Columns::strand,
                        char,
                 long long>>;
};

template<Columns el>
using type_getter_t = typename type_getter<el>::type;

template<class T>
T CastTo(std::string && t);

template<>
std::string CastTo<std::string>(std::string && t) {
    return t;
}

template<>
long long CastTo<long long>(std::string && t) {
    return std::stoll(t);
}

template<>
char CastTo<char>(std::string && t) {
    if (t.size() != 1)
        throw std::string("blat output file is corrupted");
    return t.front();
}

template<Columns ... columns>
struct Record  {
    std::tuple<type_getter_t<columns>...> fields;

    template<Columns el>
    constexpr static bool Contains() {
        return ContainsHandler<el, columns ...>();
    }

    template<Columns el, typename = std::enable_if_t<Contains<el>()>>
    constexpr static size_t GetIndex() {
        return GetIndexHandler<0, el, columns ...>();
    }

    template<Columns el, typename = std::enable_if_t<Contains<el>()>>
    type_getter_t<el> const & Get() const {
        constexpr auto index = GetIndex<el>();
        return std::get<index>(fields);
    }

    template<Columns el, typename = std::enable_if_t<Contains<el>()>>
    type_getter_t<el> & Get() {
        constexpr auto index = GetIndex<el>();
        return std::get<index>(fields);
    }
private:
    template<size_t pos, Columns el, Columns head, Columns ... tail>
    constexpr static size_t GetIndexHandler() {
        return (el == head ? pos : GetIndexHandler<pos+1, el, tail ...>());
    }

    template<size_t pos, Columns el>
    constexpr static size_t GetIndexHandler() {
        return -1ull;
    }

    template<Columns el, Columns head, Columns ... tail>
    constexpr static bool ContainsHandler() {
        return (el == head ? true : ContainsHandler<el, tail ...>());
    }

    template<Columns el>
    constexpr static bool ContainsHandler() {
        return false;
    }
};

class TokenIterator {
    size_t from;
    size_t len;
    std::string const & str;
public:
    TokenIterator(std::string const & str)
        : from(0)
        , len(0)
        , str(str)
    {
        Step();
    }

    TokenIterator & operator ++ () {
        Step();
        return *this;
    }

    std::string operator * () const {
        return str.substr(from, len);
    }

    bool IsEnd() const noexcept {
        return len == 0;
    }

private:
    void Step() noexcept {
        from += len;
        len = 0;
        while (from < str.size() && isspace(str[from]))
            ++from;
        while (from + len < str.size() && !isspace(str[from + len]))
            ++len;
    }
};

template<Columns ... columns>
using Records = std::vector<Record<columns ...>>;

template<Columns ... columns>
using FilterType = std::function<bool(Record<columns ...> const &)>;

template<Columns ... columns>
class RecordPusher {
private:
    Records<columns ...> & records;
    FilterType<columns ...> const & filter;
public:
    RecordPusher(Records<columns ...> & records, FilterType<columns ...> const & filter) 
        : records(records)
        , filter(filter)
    {}

    bool Push(std::string const & line) {
        TokenIterator it(line);
        Record<columns ...> rd;
        Fill(it, rd);
        auto should_be_pushed = filter(rd);
        if (should_be_pushed)
            records.push_back(std::move(rd));
        return should_be_pushed;
    }

private:
    template<size_t columns_index = 0,
             size_t used = 0, 
             typename = std::enable_if_t<columns_index < static_cast<size_t>(Columns::TOTAL_COLUMNS_SIZE) && 
                                         used < sizeof...(columns)>
            >
    void Fill(TokenIterator & it, Record<columns ...> & rd) {
        if (it.IsEnd())
            throw std::string("blat output file is corrupted");
        constexpr auto el = static_cast<Columns>(columns_index);
        constexpr auto contains = rd.template Contains<el>();
        PushIfNeeded<contains, el>(it, rd);
        ++it;
        Fill<columns_index + 1, used + contains>(it, rd);
    }

    template<size_t columns_index, size_t used>
    void Fill(...) {
        if (used != sizeof...(columns))
            throw std::string("blat output file is corrupted");
    }

    template<bool b, Columns el, typename = std::enable_if_t<b>>
    void PushIfNeeded(TokenIterator const & it, Record<columns ...> & rd) {
        rd.template Get<el>() = CastTo<type_getter_t<el>>(*it);
    }

    template<bool b, Columns el>
    void PushIfNeeded(...) {}

};

bool GetNextNonemptyLine(std::istream & inp, std::string & result) {
    while (std::getline(inp, result)) {
        if (result.empty())
            continue;
        return true;
    }
    return false;
}

void SkipHeader(std::istream & inp) {
    std::string line;
    if (!GetNextNonemptyLine(inp, line))
        throw std::string("Empty file");
    if (line != "psLayout version 3")
        throw std::string("Sorry, unsupported blat output version");
    GetNextNonemptyLine(inp, line); // column's_names_upper_part
    GetNextNonemptyLine(inp, line); // column's_names_lower_part
    GetNextNonemptyLine(inp, line); // -------------------------
    if (line != std::string(159, '-'))
        throw std::string("blat output file is corrupted");
}

template<Columns ... columns>
Records<columns ...> Read(std::istream & inp, FilterType<columns ...> const & filter) {
    Records<columns ...> records;
    RecordPusher<columns ...> pusher(records, filter);
    std::string line;
    SkipHeader(inp);
    size_t total_lines = 0;
    size_t accepted_lines = 0;
    while (GetNextNonemptyLine(inp, line)) {
        accepted_lines += pusher.Push(line);
        ++total_lines;
    }
    INFO("Total line read: " << total_lines);
    INFO("Accepted lines: " << accepted_lines);
    return records;
}
