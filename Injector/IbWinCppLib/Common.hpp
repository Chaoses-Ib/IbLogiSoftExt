#pragma once
#include <Windows.h>
#include <cstdint>

#include <string>
#include <iterator>
#include <functional>
#include <optional>

//For forcing MSVC to evaluate a const expression at compile time
#define CONSTEXPR(kv) ib::as_constexpr<decltype(kv),kv>

namespace ib {
    using wchar = wchar_t;
    using byte_t = uint8_t;
    using QWORD = uint64_t;

    using std::function;
    using std::optional, std::nullopt;
    using std::wstring;

    //Alternative std::iterator
    template<typename TCategory, typename TValue, typename TDiff = ptrdiff_t, typename TPointer = TValue*, typename TReference = TValue&>
    class iterator {
    public:
        using iterator_category = TCategory;
        using value_type = TValue;
        using difference_type = TDiff;
        using pointer = TPointer;
        using reference = TReference;
    };

    //For forcing MSVC to evaluate a const expression at compile time
    //Alternative: CONSTEXPR
    template<typename T, T A>
    constexpr T as_constexpr = A;

    //A simple wrapper for TChar*
    //Could be nullptr
    template<typename TChar, typename TString>
    class basic_zstring {
    public:
        TChar* p;

        basic_zstring(TChar* p) : p(p) {}
        basic_zstring(TString s) {
            static_assert(std::is_const_v<TChar>, "ib::basic_zstring: Can't convert TString to non-const TChar*");
            p = s.c_str();
        }

        static basic_zstring New(size_t size) {
            return new TChar[size];
        }
        static basic_zstring New(TString s) {
            auto p = new TChar[s.size() + 1];
            memcpy(p, s.c_str(), (s.size() + 1) * sizeof TChar);
            return p;
        }

        //Set to nullptr
        void Delete() {
            delete[] p;
            p = nullptr;
        }

        operator TChar* () {
            return p;
        }
        operator TString () {
            return p;
        }

        operator bool() {
            return p;
        }
    };
    
    using cwzstring = basic_zstring<const wchar, const wstring>;
    using wzstring = basic_zstring<wchar, wstring>;
}