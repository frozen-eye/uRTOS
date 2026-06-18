#include "os.h"
#include "port.h"

void app_main(void);

void os_boot_main(void)
{
    port_boot_banner();
    os_kernel_init();
    app_main();
    os_kernel_start();
}
