#pragma once
#include "Common.hpp"

namespace ib {
    //An iterator for ::FindWindowExW
    class FindWindowEx_i : public iterator<std::input_iterator_tag, HWND, size_t, void, void> {
        using iterator = FindWindowEx_i;

        HWND parent;
        HWND current;
        cwzstring classname;
        cwzstring title;
        FindWindowEx_i(HWND parent, HWND current, cwzstring classname, cwzstring title)
            : parent(parent), current(current), classname(classname), title(title) {}
    public:
        FindWindowEx_i(HWND parent = 0, cwzstring classname = nullptr, cwzstring title = nullptr)
            : FindWindowEx_i(parent, FindWindowExW(parent, 0, classname, title), classname, title) {}
        FindWindowEx_i(const iterator& i) : FindWindowEx_i(i.parent, i.current, i.classname, i.title) {}

        iterator begin() { return *this; }
        iterator end() { iterator tmp = *this; tmp.current = 0; return tmp; }

        value_type operator*() const { return current; }

        iterator& operator++() {
            current = FindWindowExW(parent, current, classname, title);
            return *this;
        }
        iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }

        friend bool operator==(const iterator& a, const iterator& b) {
            return a.current == b.current;
            //return a.current == b.current && a.parent == b.parent
            //    && a.classname == b.classname && a.title == b.title;
        }
        friend bool operator!=(const iterator& a, const iterator& b) { return !(a == b); }
    };
}