#pragma once

#include "utils/logger/logger.hpp"

#include <istream>
#include <vector>
#include <string>
#include <functional>
#include <type_traits>
#include <tuple>

template<class Columns, Columns el>
struct type_getter;

template<class Columns, Columns el>
using type_getter_t = typename type_getter<Columns, el>::type;

template<class T>
T CastTo(std::string && t);

template<>
inline std::string CastTo<std::string>(std::string && t) {
    return std::move(t);
}

template<>
inline long long CastTo<long long>(std::string && t) {
    return std::stoll(t);
}

template<>
inline char CastTo<char>(std::string && t) {
    if (t.size() != 1)
        throw std::string("aligner output file is corrupted");
    return t.front();
}

template<class Columns, Columns ... columns>
struct Record  {
    std::tuple<type_getter_t<Columns, columns>...> fields;

    template<Columns el>
    constexpr static bool Contains() {
        return ContainsHandler<el, columns ...>();
    }

    template<Columns el, typename = std::enable_if_t<Contains<el>()>>
    constexpr static size_t GetIndex() {
        return GetIndexHandler<0, el, columns ...>();
    }

    template<Columns el, typename = std::enable_if_t<Contains<el>()>>
    type_getter_t<Columns, el> const & Get() const {
        constexpr auto index = GetIndex<el>();
        return std::get<index>(fields);
    }

    template<Columns el, typename = std::enable_if_t<Contains<el>()>>
    type_getter_t<Columns, el> & Get() {
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

template<class Columns, Columns ... columns>
using Records = std::vector<Record<Columns, columns ...>>;

template<class Columns, Columns ... columns>
using FilterType = std::function<bool(Record<Columns, columns ...> const &)>;

template<class Columns, Columns ... columns>
class RecordPusher {
private:
    Records<Columns, columns ...> & records;
    FilterType<Columns, columns ...> const & filter;
public:
    RecordPusher(Records<Columns, columns ...> & records, FilterType<Columns, columns ...> const & filter) 
        : records(records)
        , filter(filter)
    {}

    bool Push(std::string const & line) {
        TokenIterator it(line);
        Record<Columns, columns ...> rd;
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
    void Fill(TokenIterator & it, Record<Columns, columns ...> & rd) {
        if (it.IsEnd())
            throw std::string("aligner output file is corrupted");
        constexpr auto el = static_cast<Columns>(columns_index);
        constexpr auto contains = rd.template Contains<el>();
        PushIfNeeded<contains, el>(it, rd);
        ++it;
        Fill<columns_index + 1, used + contains>(it, rd);
    }

    template<size_t columns_index, size_t used>
    void Fill(...) {
        if (used != sizeof...(columns))
            throw std::string("aligner output file is corrupted");
    }

    template<bool b, Columns el, typename = std::enable_if_t<b>>
    void PushIfNeeded(TokenIterator const & it, Record<Columns, columns ...> & rd) {
        rd.template Get<el>() = CastTo<type_getter_t<Columns, el>>(*it);
    }

    template<bool b, Columns el>
    void PushIfNeeded(...) {}

};

inline bool GetNextNonemptyLine(std::istream & inp, std::string & result) {
    while (std::getline(inp, result)) {
        if (result.empty())
            continue;
        return true;
    }
    return false;
}
