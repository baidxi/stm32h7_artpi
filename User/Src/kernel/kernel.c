#include <init.h>
#include <device/driver.h>
#include <device/device.h>

int early_init(void)
{
    device_init();
    driver_init();
    return 0;
}