#include "httplib.h"
#include <atomic>

#include "../riscvm.h"

static std::atomic<uint32_t>    g_request_id = 0;
static thread_local std::string g_response_data;

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif // _WIN32

extern "C" EXPORT void append_response(const char* data)
{
    g_response_data += data;
}

static void handle_riscvm(const httplib::Request& req, httplib::Response& res)
{
    auto request_id = g_request_id.fetch_add(1);
    g_response_data.clear();

    auto error = [&res](const char* message)
    {
        res.status = 400;
        res.set_content(std::string("error:") + message, "text/plain");
        printf("[c2] %s\n", message);
    };

    const std::string& body = req.body;
    if (body.size() < 0x10 || memcmp(body.c_str(), "RV64", 4) != 0)
    {
        error("Invalid RV64 payload header");
        return;
    }
    std::vector<uint8_t> vm_code((body.size() + 0xFFF) & ~0xFFF);
    memcpy(vm_code.data(), body.c_str() + 4, body.size() - 4);
    std::vector<uint8_t> stack(0x10000);

    printf("[c2] executing %zu byte payload\n", body.size() - 4);

    riscvm vm = {};
#ifdef TRACING
    auto trace = req.get_param_value("trace");
    if (!trace.empty())
    {
        printf(
            "[c2] tracing enabled (base: %p, first instruction: 0x%08x)\n",
            vm_code.data(),
            *(uint32_t*)vm_code.data()
        );
        auto filename = "c2-" + std::to_string(request_id) + ".trace";
        vm.trace      = fopen(filename.c_str(), "w");
        vm.rebase     = -(int64_t)vm_code.data();
    }
#else
    (void)request_id;
#endif // TRACING

    auto self = &vm;
    reg_write(reg_a0, 0x1122334455667788);
    reg_write(reg_sp, (uint64_t)(stack.data() + stack.size() - 0x18));
    self->pc = (int64_t)vm_code.data();

#pragma pack(1)
    struct Features
    {
        uint32_t magic;
        struct
        {
            bool encrypted : 1;
            bool shuffled  : 1;
        };
        uint32_t key;
    };
    static_assert(sizeof(Features) == 9, "");

    auto features = (Features*)(vm_code.data() + body.size() - 4 - sizeof(Features));
    if (features->magic != 'TAEF')
    {
#if defined(CODE_ENCRYPTION) || defined(OPCODE_SHUFFLING)
        error("no features found in file");
        return;
#else
        printf("[c2] no features in the file (unencrypted payload?)\n");
#endif // CODE_ENCRYPTION || OPCODE_SHUFFLING
    }
    else
    {
#ifdef OPCODE_SHUFFLING
        if (!features->shuffled)
        {
            error("shuffling enabled on the host, disabled in the bytecode");
            return;
        }
#else
        if (features->shuffled)
        {
            error("shuffling disabled on the host, enabled in the bytecode");
            return;
        }
#endif // OPCODE_SHUFFLING

#ifdef CODE_ENCRYPTION
        if (!features->encrypted)
        {
            error("encryption enabled on the host, disabled in the bytecode");
            return;
        }
        self->base = self->pc;
        self->key  = features->key;
#else
        if (features->encrypted)
        {
            error("encryption disabled on the host, enabled in the bytecode");
            return;
        }
#endif // CODE_ENCRYPTION
    }

    riscvm_run(self);
    auto status = (int)reg_read(reg_a0);

#ifdef TRACING
    if (vm.trace != nullptr)
    {
        fclose(vm.trace);
    }
#endif // TRACING

    auto response = "epoch:" + std::to_string(time(nullptr)) + "\n";
    response += "status:" + std::to_string(status) + "\n";
    if (!g_response_data.empty())
    {
        response += "data:" + g_response_data + "\n";
    }
    res.set_content(response, "text/plain");

    printf("[c2] riscvm_run returned exit code %d\n", status);
}

#define HOST "127.0.0.1"
#define PORT 13337

int main(int argc, char** argv)
{
#ifdef TRACING
    g_trace = true;
#endif // TRACING
    httplib::Server svr;
    svr.Get(
        "/ping",
        [](const httplib::Request& req, httplib::Response& res)
        {
            auto epoch = time(nullptr);
            printf("[%lu] /hi %s:%d\n", (unsigned long)epoch, req.remote_addr.c_str(), req.remote_port);
            fflush(stdout);
            res.set_content("pong:" + std::to_string(epoch), "text/plain");
        }
    );
    // curl -X POST -d @payload.bin http://127.0.0.1:13337
    svr.Post("/riscvm", handle_riscvm);
    printf("[c2] starting server on %s:%d\n", HOST, PORT);
    if (!svr.listen(HOST, PORT))
    {
        puts("Failed to start server!");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
