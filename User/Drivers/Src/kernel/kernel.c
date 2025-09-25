#include <init.h>
#include <bus.h>
#include <device/driver.h>
#include <device/device.h>

int early_init(void)
{
    bus_type_init();
    device_init();
    return 0;
}