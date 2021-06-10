#pragma once
#include "Common.hpp"

namespace ib {
    using Offset = intptr_t;  // size_t can't be negative

    class Addr {
    public:
        Byte* p;

        Addr(void* p) : p((Byte*)p) { }
        Addr(uintptr_t p) : p((Byte*)p) { }

        template<typename T>
        operator T*() const {
            return (T*)p;
        }

        // For method chaining. Same as operator T*().
        // Non-static data members can't have templates, so it has to be a function.
        template<typename T>
        T* ptr() const {
            return (T*)p;
        };

        explicit operator uintptr_t() const {
            return (uintptr_t)p;
        }



        Addr operator+(Offset offset) const {
            return p + offset;
        }
        Addr operator-(Offset offset) const {
            return p - offset;
        }

        // For method chaining. Same as operator+().
        [[nodiscard]] Addr offset(Offset offset) const {
            return p + offset;
        }

        //Equals to offset(sizeof(T) * num).
        template<typename T>
        [[nodiscard]] Addr offset(size_t num = 1) const {
            return p + sizeof(T) * num;
        }

        Addr& operator+=(Offset offset) {
            p += offset;
            return *this;
        }
        Addr& operator-=(Offset offset) {
            p -= offset;
            return *this;
        }

        // Read the pointer at (p + offset).
        // addr[off] equals to *(Addr*)(addr + off). ([addr + off] in assembly language)
        Addr operator[](Offset offset) const {
            return *(void**)(p + offset);
        }
        


        // Function as operator bool, operator<=>, operator delete
        operator void* () const {
            return p;
        }

        Offset operator-(Addr addr) const {
            return p - addr.p;
        }



        [[nodiscard]] Addr align_up(size_t alignment) const {
            --alignment;
            return ((uintptr_t)p + alignment) & ~alignment;
        }
        [[nodiscard]] Addr align_down(size_t alignment) const {
            --alignment;
            return (uintptr_t)p & ~alignment;
        }



        template<typename T>
        T read() const {
            return *(T*)p;
        }

        auto read() const {
            return read_return_auto{ *this };
        }
    private:
        struct read_return_auto {
            const Addr& addr;

            template<typename T>
            operator T() const {
                return addr.read<T>();
            }
        };
    public:

        template<typename T>
        void write(T value) const {
            *(T*)p = value;
        }

        

        // Return (size_t)-1 if fails.
        size_t heap_size(HANDLE heap = GetProcessHeap()) const {
            return HeapSize(heap, 0, p);
        }
    };

    namespace mem {
        // **It does not equal to Windows's flProtect.**
        using ProtectFlags = DWORD;
        class Protect {
        public:
            using T = const ProtectFlags;
            static T NoAccess = PAGE_NOACCESS - 1;  // false (otherwise true)
            static T Read = PAGE_READONLY - 1;
            static T Write = PAGE_READWRITE - 1;  // Include Read
            static T WriteCopy = PAGE_WRITECOPY - 1;  // Include Read and Write

            static T Execute = PAGE_EXECUTE;

            static T Guard = PAGE_GUARD;
            static T NoCache = PAGE_NOCACHE;
            static T WriteCombine = PAGE_WRITECOMBINE;

            static DWORD to_flProtect(ProtectFlags protect) {
                ++protect;
                if (protect & Execute) {
                    protect = (protect & ~0xFF) | (protect & 0xFF) << 4;
                }
                return protect;
            }

            static ProtectFlags to_protect(DWORD flProtect) {
                --flProtect;
                if (flProtect & 0x8) {
                    flProtect = flProtect >> 4 | Execute;
                }
                return flProtect;
            }
        };

        // Example: change_protect(addr, 5, Protect::Write | Protect::Execute);
        inline ProtectFlags change_protect(Addr addr, size_t size, ProtectFlags protect) {
            DWORD flProtect = PAGE_NOACCESS;
            VirtualProtect(addr, size, Protect::to_flProtect(protect), &flProtect);
            return Protect::to_protect(flProtect);
        }

        // Change protect, call f, and then change protect back.
        inline bool protect_changed(Addr addr, size_t size, ProtectFlags protect, function<void(Addr)> f) {
            DWORD flProtect;
            if (!VirtualProtect(addr, size, Protect::to_flProtect(protect), &flProtect))
                return false;
            f(addr);
            //lpflOldProtect can't be nullptr
            return VirtualProtect(addr, size, flProtect, &flProtect);
        }
    }
}