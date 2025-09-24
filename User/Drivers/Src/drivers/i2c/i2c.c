#include <bus.h>
#include <device/driver.h>
#include <device/i2c/i2c.h>


#include <shell.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>

static struct list_head adapter_list = LIST_HEAD_INIT(adapter_list);
static struct list_head driver_list = LIST_HEAD_INIT(driver_list);

static uint8_t next_adapter_nr = 0;
static int8_t current_adapter = -1;

static int i2c_bus_probe(struct device *dev)
{
    return dev->driver->probe(dev);
}

static int i2c_bus_remove(struct device *dev)
{
    return dev->driver->remove(dev);
}

static int i2c_bus_match(struct device *dev, struct device_driver *drv)
{
    const struct device_match_table *ptr;

    for (ptr = drv->match_ptr; ptr && ptr->compatible; ptr++) {
        if (strcmp(dev->init_name, ptr->compatible) == 0) {
            dev->driver = drv;
            return 1;
        }
    }

    if (dev->init_name && drv->name) {
        if (strcmp(dev->init_name, drv->name) == 0) {
            dev->driver = drv;
            return 1;
        }
    }

    return 0;
}

struct bus_type i2c_bus_type = {
    .name = "i2c",
    .probe = i2c_bus_probe,
    .remove = i2c_bus_remove,
    .match = i2c_bus_match,
};

int i2c_add_addapter(struct i2c_adapter *adap)
{
    adap->nr = next_adapter_nr++;
    return i2c_register_adapter(adap);
}

int i2c_register_adapter(struct i2c_adapter *adap)
{
    int ret;

    if (!adap->dev.init_name)
        return -EINVAL;

    adap->dev.bus = &i2c_bus_type;

    ret = device_register(&adap->dev);

    if (ret)
        return ret;

    INIT_LIST_HEAD(&adap->client_list);

    list_add_tail(&adap->list, &adapter_list);

    return 0;
}

static int i2c_driver_probe(struct device *dev)
{
    struct i2c_client *client = to_i2c_device(dev);
    struct i2c_driver *drv = to_i2c_driver(dev->driver);
    const struct i2c_device_id *id;

    for (id = drv->match_table; id; id++) {
        if (strcmp(client->name, id->name) == 0) {
            client->drv = drv;
            return drv->probe(client, id);
        }
    }
    return -1;
}

static int i2c_driver_remove(struct device *dev)
{
    return 0;
}

int i2c_register_driver(struct i2c_driver *drv)
{
    int ret;

    if (!drv || !drv->probe || !drv->remove ||
        !drv->drv.name)
        return -EINVAL;

    drv->drv.probe = &i2c_driver_probe;
    drv->drv.remove = &i2c_driver_remove;
    drv->drv.bus = &i2c_bus_type;

    ret = driver_register(&drv->drv);
    if (ret)
        return ret;

    INIT_LIST_HEAD(&drv->device_list);

    list_add_tail(&drv->list, &driver_list);

    return 0;
}

int i2c_transfer(struct i2c_adapter *adap, struct i2c_message *msg, unsigned int count)
{
    return adap->algo->master_xfer(adap, msg, count);
}

int i2c_register_device(struct i2c_client *client)
{
    if (!client || 
        !client->adap || 
        !client->addr)
        return -EINVAL;

    client->dev.bus = &i2c_bus_type;

    return device_register(&client->dev);
}

static struct option i2c_options[] = {
    {"scan", no_argument, 0, 0},
    {"dev", required_argument, 0, 0},
    {"list", no_argument, 0, 'l'},
    {"read", required_argument, 0, 'r'},
    {"write", required_argument, 0, 'w'},
    {0, 0, 0, 0}
};

const char *help_str[] = {
    "scan device",
    "set adapter",
    "list adapter",
    "addr reg count",
    "addr reg value len"
};

static int show_help()
{
    int i;
    shell_puts("i2c utils\r\n");

    for (i = 0; i < ARRAY_SIZE(i2c_options) -1 ; i++) {
        shell_printf("\t--%s\t- %s\r\n", i2c_options[i].name, help_str[i]);
    }

    return 0;
}

static int do_scan(int8_t dev_id)
{
    struct i2c_adapter *adap;
    struct i2c_message msg = {0};
    size_t len = 0;
    char buf[128] = {0};
    int i;

    list_for_each_entry(adap, &adapter_list, list)
    {
        if (adap->nr == dev_id) {
            goto found;
        }
    }
    shell_puts("dev Not Set\r\n");
    return -1;
found:
    for (i = 0; i < 128; i++) {
        msg.addr = i;
        msg.flags = I2C_M_PROBE;
        if (adap->algo->master_xfer(adap, &msg, 1) == 0) {
            len += snprintf(buf + len, sizeof(buf) - len, "%02x", msg.addr);
        } else {
            len += snprintf(buf + len, sizeof(buf) - len, "--");
        }

        if ((i + 1) % 16 != 0)
            len += snprintf(buf + len, sizeof(buf) - len, " ");
        else {
            snprintf(buf + len, sizeof(buf) - len, "\r\n");
            shell_puts(buf);
            len = 0;
        }
    }
    return 0;
}

static void do_list_adapter()
{
    struct i2c_adapter *adap;
    list_for_each_entry(adap, &adapter_list, list)
    {
        shell_printf("%d - %s - driver %s\r\n",adap->nr, adap->dev.init_name, adap->dev.driver ? adap->dev.driver->name : "none");
    }
}

static int do_i2c(int argc, char *argv[])
{
    int c;
    int opt_ind;
    unsigned long val;

    if (argc < 2) {
        return show_help();
    }

    extern void getopt_reset();

    getopt_reset();

    while((c = getopt_long(argc, argv, "lr:w:", i2c_options, &opt_ind)) != -1) {
        switch(c) {
            case 0:
                switch(opt_ind) {
                    case 0:
                        if (current_adapter >= 0) {
                            return do_scan(current_adapter);
                        } else {
                            shell_puts("adapter Not Set\r\n");
                        }
                        break;
                    case 1:
                        val = strtoul(optarg, NULL, 0);
                        if ((errno && errno == ERANGE) || val > next_adapter_nr) {
                            shell_printf("Invalid controller id %s\r\n", optarg);
                            return 0;
                        }

                        shell_printf("Current adapter %d\r\n", val);

                        current_adapter = val & 0xff;

                        break;
                }
                break;
            case 'r':
                break;
            case 'w':
                break;
            case 'l':
                do_list_adapter();
                break;
            default:
                break;
        }
    }

    return 0;
}

static void i2c_cmd_help(int argc, char *argv[])
{
    char *cmd;
    int i;

    if (argc != 2)
        return;

    cmd = argv[1];

    for (i = 0; i < ARRAY_SIZE(i2c_options); i++) {
        if (strcmp(cmd, i2c_options[i].name) == 0) {
            shell_printf("%s - %s\r\n", i2c_options[i].name, help_str[i]);
            break;
        }
    }
}

shell_command_register(i2c, "i2c utils", do_i2c, i2c_cmd_help);

register_bus_type(i2c_bus_type);
