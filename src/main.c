#include "tcp-echo.h"
#include "controller.h"
#include "worker.h"

int main(int argc, char *argv[])
{
    te_set_libuv_allocator();

    if (te_os_checkenv("TE_WORKER_ID")) {
        return te_worker_main(argc, argv);
    }

    return te_controller_main(argc, argv);
}
