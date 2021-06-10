#pragma once
#include "Common.hpp"
#include "System.Memory.hpp"
#include <type_traits>

namespace ib {
    struct Module {
        union {
            HMODULE handle;
            Addr base;
        };

        Module(HMODULE handle) : handle(handle) {};

        wzstring GetPath() {
            wzstring pathbuf = wzstring::New(MAX_PATH);
            if (!GetModuleFileNameW(handle, pathbuf, MAX_PATH))
                pathbuf.Delete();
            return pathbuf;
        }

        bool Free() {
            return FreeLibrary(handle);
        }
    };

    class ModuleFactory {
    public:
        //Doesn't increase reference count
        static optional<Module> Find(cwzstring module_name) {
            HMODULE h;
            if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, module_name, &h))
                return Module(h);
            else
                return {};
        }

        static optional<Module> Load(cwzstring module_name) {
            auto h = LoadLibraryW(module_name);
            return h ? optional(Module(h)) : nullopt;
        }

        static optional<Module> CurrentProcess() {
            HMODULE h;
            if (GetModuleHandleExW(0, nullptr, &h))
                return Module(h);
            else
                return {};
        }
    };
}