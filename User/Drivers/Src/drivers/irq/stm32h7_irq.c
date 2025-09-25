#include <irq/stm32h7_irq.h>
#include <device/driver.h>

#include <errno.h>

#include <FreeRTOS.h>

static int default_irq_handler(unsigned int irq, void *dev)
{
    return 0;
}

static int stm32h7_irq_chip_probe(struct irq_chip *chip)
{
    chip->descs = pvPortMalloc(sizeof(*chip->descs) * chip->nr_irq);
    struct irq_desc *desc;
    int i;

    if (!chip->descs)
        return -ENOMEM;

    for (i = 0; i < chip->nr_irq; i++)
    {
        desc = &chip->descs[i];
        desc->handler = default_irq_handler;
    }

    return 0;
}

static int stm32h7_irq_chip_remove(struct irq_chip *chip)
{
    return 0;
}

static const struct device_match_table match_table[] = {
    {
        .compatible = "stm32h7-irq-chip",
    },
    {

    }
};

static void stm32h7_irq_chip_driver_init(struct device_driver *drv)
{
    struct irq_chip_driver *_drv = container_of(drv, struct irq_chip_driver, drv);
    register_irq_driver(_drv);
}

static struct irq_chip_driver stm32h7_irq_chip_drv = {
    .drv = {
            .match_ptr = match_table,
            .name = "stm32h7-irq-chip",
            .init = stm32h7_irq_chip_driver_init,
    },
    .probe = stm32h7_irq_chip_probe,
    .remove = stm32h7_irq_chip_remove,
};

register_driver(stm32h7_irq_drv, stm32h7_irq_chip_drv.drv);
