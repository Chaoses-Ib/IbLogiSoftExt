#pragma once
#include <cstdint>

struct Request {
    uint32_t opcode;
};

#pragma pack(push, 4)
struct Response {
    uint64_t error;
    union {
        struct {
            uint64_t device;
            uint32_t flags;
        } op1;
    };
private:
    void assert_size() {
        static_assert(sizeof Response == 20);
    }
};
#pragma pack(pop)


//Server
void IpcStart();