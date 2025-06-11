#include "httplib.h"
#include <windows.h>
#include <atomic>

#include "../riscvm.h"

static std::atomic<uint32_t>    g_request_id = 0;
static thread_local std::string g_response_data;

extern "C" __declspec(dllexport) void append_response(const char* data)
{
    g_response_data += data;
}

static void handle_riscvm(const httplib::Request& req, httplib::Response& res)
{
    auto request_id = g_request_id.fetch_add(1);
    g_response_data.clear();

    const std::string& body = req.body;
    if (body.size() < 0x10 || memcmp(body.c_str(), "RV64", 4) != 0)
    {
        res.status = 400;
        res.set_content("Invalid RV64 code", "text/plain");
        printf("[c2] invalid payload header!\n");
        return;
    }
    std::vector<uint8_t> vm_code((body.size() + 0xFFF) & ~0xFFF);
    memcpy(vm_code.data(), body.c_str() + 4, body.size());
    std::vector<uint8_t> stack(0x10000);

    printf("[c2] executing %zu byte payload\n", body.size());

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
#endif // TRACING

    auto self = &vm;
    reg_write(reg_a0, 0x1122334455667788);
    reg_write(reg_sp, (uint64_t)(stack.data() + stack.size() - 0x18));
    self->pc = (int64_t)vm_code.data();
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
    response += "data:" + g_response_data + "\n";
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
