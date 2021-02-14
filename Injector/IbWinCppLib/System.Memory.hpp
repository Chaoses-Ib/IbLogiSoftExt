#pragma once
#include "Common.hpp"

namespace ib {
    using offset_t = int32_t; //since size_t can't be negative

    struct addr {
        void* p;

        addr(void* p) : p(p) { }
        addr(byte_t* p) : p(p) { }

        operator void* () {
            return p;
        }
        operator byte_t* () {
            return (byte_t*)p;
        }
        template<typename T>
        operator T* () {
            return (T*)p;
        }
#ifdef _WIN64
        explicit operator uint64_t() {
            return (uint64_t)p;
        }
#else
        explicit operator uint32_t() {
            return (uint32_t)p;
        }
#endif

        //It doesn't modify *this, but return a new addr.
        addr offset(offset_t offset) {
            return addr((byte_t*)p + offset);
        }
        addr operator+(offset_t offset) {
            return this->offset(offset);
        }
        addr operator-(offset_t offset) {
            return this->offset(-offset);
        }

        bool Unprotected(size_t size, function<bool(addr)> f) {
            DWORD old_protect;
            if (!VirtualProtect(p, size, PAGE_EXECUTE_READWRITE, &old_protect))  //#TODO
                return false;
            bool success = f(*this);
            //lpflOldProtect can't be nullptr
            return VirtualProtect(p, size, old_protect, &old_protect) && success;  //avoid short-circuit evaluation
        }

        bool Unprotected(size_t size, function<void(addr)> f) {
            DWORD old_protect;
            if (!VirtualProtect(p, size, PAGE_EXECUTE_READWRITE, &old_protect))  //#TODO
                return false;
            f(*this);
            return VirtualProtect(p, size, old_protect, &old_protect);  //lpflOldProtect can't be nullptr
        }

    };
}