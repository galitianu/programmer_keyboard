#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/input.h>

#define GPIO_BASE 0x3F200000
#define GPIO_LEN 0xB4

// Register offsets
#define GPFSEL2_OFFSET 0x08
#define GPLEV0_OFFSET 0x34
#define GPPUD_OFFSET 0x94
#define GPPUDCLK0_OFFSET 0x98

#define GPIO_BTN_INC 23
#define GPIO_BTN_DEC 24

#define GPIO_FSEL_SHIFT(pin) (((pin) % 10) * 3)

static void __iomem *gpio_base;
static struct task_struct *poll_thread;
static struct input_dev *input_dev;
static bool running = true;
static int last_inc_state = 1;
static int last_dec_state = 1;

static inline int gpio_read_pin(int pin)
{
    unsigned int val = ioread32(gpio_base + GPLEV0_OFFSET);
    return (val & (1 << pin)) ? 1 : 0;
}

static int poll_thread_fn(void *data)
{
    while (!kthread_should_stop() && running)
    {
        int curr_inc = gpio_read_pin(GPIO_BTN_INC);
        int curr_dec = gpio_read_pin(GPIO_BTN_DEC);

        if (curr_inc != last_inc_state)
        {
            if (curr_inc == 0)
            {
                input_report_key(input_dev, KEY_1, 1);
                input_sync(input_dev);
            }
            else
            {
                input_report_key(input_dev, KEY_1, 0);
                input_sync(input_dev);
            }
            last_inc_state = curr_inc;
        }

        if (curr_dec != last_dec_state)
        {
            if (curr_dec == 0)
            {
                input_report_key(input_dev, KEY_0, 1);
                input_sync(input_dev);
            }
            else
            {
                input_report_key(input_dev, KEY_0, 0);
                input_sync(input_dev);
            }
            last_dec_state = curr_dec;
        }

        msleep(10);
    }
    return 0;
}

static int __init gpio_keyboard_init(void)
{
    unsigned int val;

    pr_info("Programmer keyboard driver: loading...\n");

    gpio_base = ioremap(GPIO_BASE, GPIO_LEN);
    if (!gpio_base)
    {
        pr_err("Failed to ioremap\n");
        return -ENOMEM;
    }

    unsigned int *gpfsel2 = (unsigned int *)(gpio_base + GPFSEL2_OFFSET);
    val = ioread32(gpfsel2);
    val &= ~(0x7 << GPIO_FSEL_SHIFT(GPIO_BTN_INC));
    val &= ~(0x7 << GPIO_FSEL_SHIFT(GPIO_BTN_DEC));
    iowrite32(val, gpfsel2);

    iowrite32(0x2, gpio_base + GPPUD_OFFSET);
    ndelay(150);
    iowrite32((1 << GPIO_BTN_INC) | (1 << GPIO_BTN_DEC), gpio_base + GPPUDCLK0_OFFSET);
    ndelay(150);
    iowrite32(0, gpio_base + GPPUD_OFFSET);
    iowrite32(0, gpio_base + GPPUDCLK0_OFFSET);

    input_dev = input_allocate_device();
    if (!input_dev)
    {
        pr_err("Failed to allocate input device\n");
        iounmap(gpio_base);
        return -ENOMEM;
    }

    input_dev->name = "Programmer Keyboard";
    input_dev->phys = "gpio/input0";
    input_dev->id.bustype = BUS_HOST;
    input_dev->evbit[0] = BIT_MASK(EV_KEY);
    set_bit(KEY_0, input_dev->keybit);
    set_bit(KEY_1, input_dev->keybit);

    if (input_register_device(input_dev))
    {
        pr_err("Failed to register input device\n");
        input_free_device(input_dev);
        iounmap(gpio_base);
        return -EINVAL;
    }

    running = true;
    poll_thread = kthread_run(poll_thread_fn, NULL, "gpio_keyboard_poll");
    if (IS_ERR(poll_thread))
    {
        pr_err("Failed to create polling thread\n");
        input_unregister_device(input_dev);
        iounmap(gpio_base);
        return PTR_ERR(poll_thread);
    }

    pr_info("Programmer keyboard driver loaded.\n");
    return 0;
}

static void __exit gpio_keyboard_exit(void)
{
    running = false;
    if (poll_thread)
    {
        kthread_stop(poll_thread);
    }
    if (input_dev)
    {
        input_unregister_device(input_dev);
    }
    if (gpio_base)
    {
        iounmap(gpio_base);
        gpio_base = NULL;
    }
    pr_info("Programmer keyboard driver unloaded.\n");
}

module_init(gpio_keyboard_init);
module_exit(gpio_keyboard_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrei Galitianu");
MODULE_DESCRIPTION("Programmer keyboard driver on Raspberry Pi");