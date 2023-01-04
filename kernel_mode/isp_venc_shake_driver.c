/*
 * Copyright (C) 2021 - 2022  Alibaba Group. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <linux/cdev.h>
#include <linux/pci.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include "isp_venc_shake_driver.h"

#define IVS_SWREG_AMOUNT 27

typedef struct _theadivs_dev
{
	struct cdev cdev;
	dev_t devt;
	struct class *class;
    unsigned long base_addr;
    u32 iosize;
    volatile u8 *hwregs;
    int irq;
    int state;
} theadivs_dev;

static int theadivs_major = 0;  /* dynamic */
static int theadivs_minor = 0;
static theadivs_dev* theadivs_data = NULL;
static unsigned int device_register_index = 0;
struct class *theadivs_class;

static irqreturn_t theadivs_isr(int irq, void *dev_id);

static void print_registers(void)
{
    printk("pic_width = %d\n", ioread32((void*)(theadivs_data->hwregs + 0x00)));
    printk("pic_height = %d\n", ioread32((void*)(theadivs_data->hwregs + 0x04)));
    printk("encode_width = %d\n", ioread32((void*)(theadivs_data->hwregs + 0x08)));
    printk("encode_height = %d\n", ioread32((void*)(theadivs_data->hwregs + 0x0c)));
    printk("wid_y = %d\n", ioread32((void*)(theadivs_data->hwregs + 0x10)));
    printk("wid_uv = %d\n", ioread32((void*)(theadivs_data->hwregs + 0x14)));
    printk("sram_size = %d\n", ioread32((void*)(theadivs_data->hwregs + 0x18)));
    printk("encode_n = %d\n", ioread32((void*)(theadivs_data->hwregs + 0x1c)));
    printk("stride_y = %d\n", ioread32((void*)(theadivs_data->hwregs + 0x20)));
    printk("stride_uv = %d\n", ioread32((void*)(theadivs_data->hwregs + 0x24)));
    printk("encode_x = %d\n", ioread32((void*)(theadivs_data->hwregs + 0x2c)));
    printk("encode_y = %d\n", ioread32((void*)(theadivs_data->hwregs + 0x30)));
    printk("int_state = %d\n", ioread32((void*)(theadivs_data->hwregs + 0x38)));
    printk("int_mask = %d\n", ioread32((void*)(theadivs_data->hwregs + 0x40)));
}

static int config_ivs(struct file *filp, struct ivs_parameter *params)
{
    iowrite32(params->pic_width, (void*)(theadivs_data->hwregs + 0x00));
    iowrite32(params->pic_height, (void*)(theadivs_data->hwregs + 0x04));
    iowrite32(params->encode_width, (void*)(theadivs_data->hwregs + 0x08));
    iowrite32(params->encode_height, (void*)(theadivs_data->hwregs + 0x0c));
    iowrite32(params->wid_y, (void*)(theadivs_data->hwregs + 0x10));
    iowrite32(params->wid_uv, (void*)(theadivs_data->hwregs + 0x14));
    iowrite32(params->sram_size, (void*)(theadivs_data->hwregs + 0x18));
    iowrite32(params->encode_n, (void*)(theadivs_data->hwregs + 0x1c));
    iowrite32(params->stride_y, (void*)(theadivs_data->hwregs + 0x20));
    iowrite32(params->stride_uv, (void*)(theadivs_data->hwregs + 0x24));
    iowrite32(1, (void*)(theadivs_data->hwregs + 0x28)); // CLEAR
    iowrite32(params->encode_x, (void*)(theadivs_data->hwregs + 0x2c));
    iowrite32(params->encode_y, (void*)(theadivs_data->hwregs + 0x30));
    iowrite32(0, (void*)(theadivs_data->hwregs + 0x34)); // START
    iowrite32(0, (void*)(theadivs_data->hwregs + 0x3c)); // INT_CLEAN
    iowrite32(params->int_mask, (void*)(theadivs_data->hwregs + 0x40));

    return 0;
}

static long theadivs_ioctl(struct file *filp,
                          unsigned int cmd, unsigned long arg)
{
    int err = 0;

    if (_IOC_TYPE(cmd) != THEAD_IOC_MAGIC)
        return -ENOTTY;
    else if (_IOC_NR(cmd) > THEAD_IOC_MAXNR)
        return -ENOTTY;

    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok((void *) arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok((void *) arg, _IOC_SIZE(cmd));
    if (err)
        return -EFAULT;

    switch (cmd)
    {
    case THEAD_IOCH_CONFIG_IVS:
    {
        struct ivs_parameter params;
        printk("%s: THEAD_IOCH_CONFIG_IVS\n", __func__);
        err = copy_from_user(&params, (struct ivs_parameter*)arg, sizeof(struct ivs_parameter));
        config_ivs(filp, &params);
        print_registers();
        theadivs_data->state = THEADIVS_READY;
        break;
    }
    case THEAD_IOCH_START_IVS:
    {
        printk("%s: THEAD_IOCH_START_IVS\n", __func__);
        iowrite32(1, theadivs_data->hwregs + 0x34); // START
        print_registers();
        theadivs_data->state = THEADIVS_RUNNING;
        break;
    }
    case THEAD_IOCH_RESET_IVS:
    {
        printk("%s: THEAD_IOCH_RESET_IVS\n", __func__);
        iowrite32(1, theadivs_data->hwregs + 0x40); // INT_MASK
        iowrite32(1, theadivs_data->hwregs + 0x28); // CLEAR
        iowrite32(0, theadivs_data->hwregs + 0x40); // INT_MASK
        print_registers();
        theadivs_data->state = THEADIVS_IDLE;
        break;
    }
    case THEAD_IOCH_GET_STATE:
    {
        printk("%s: THEAD_IOCH_GET_STATE: %d\n", __func__, theadivs_data->state);
        err = copy_to_user((int *)arg, &theadivs_data->state, sizeof(int));
        break;
    }
    default:
    {
        printk("%s: undefined command: 0x%x\n", __func__, cmd);
    }
    }
    return 0;
}

static int theadivs_open(struct inode *inode, struct file *filp)
{
    int result = 0;
    //theadivs_dev *dev = theadivs_data;
    //filp->private_data = (void *) dev;

    return result;
}
static int theadivs_release(struct inode *inode, struct file *filp)
{
    //theadivs_dev *dev = (theadivs_dev *) filp->private_data;

    return 0;
}

static struct file_operations theadivs_fops = {
    .owner= THIS_MODULE,
    .open = theadivs_open,
    .release = theadivs_release,
    .unlocked_ioctl = theadivs_ioctl,
    .fasync = NULL,
};

static const struct of_device_id thead_of_match[] = {
        { .compatible = "thead,light-ivs",  },
        { /* sentinel */  },
};

static int theadivs_reserve_IO(void)
{
    if(!request_mem_region
        (theadivs_data->base_addr, theadivs_data->iosize, "shake"))
    {
        printk(KERN_INFO "theadivs: failed to reserve HW regs\n");
        printk(KERN_INFO "theadivs: base_addr = 0x%08lx, iosize = %d\n",
                          theadivs_data->base_addr,
                          theadivs_data->iosize);
        return -1;
    }

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,17,0))
    theadivs_data->hwregs =
        (volatile u8 *) ioremap_nocache(theadivs_data->base_addr,
                                        theadivs_data->iosize);
#else
    theadivs_data->hwregs =
        (volatile u8 *) ioremap(theadivs_data->base_addr,
                                        theadivs_data->iosize);
#endif

    if (theadivs_data->hwregs == NULL)
    {
        printk(KERN_INFO "theadivs: failed to ioremap HW regs\n");
        release_mem_region(theadivs_data->base_addr, theadivs_data->iosize);
        return -1;
    }

    printk("theadivs: mapped from 0x%lx to %p with size %d\n", 
        theadivs_data->base_addr, theadivs_data->hwregs, theadivs_data->iosize);

    return 0;
}

static void theadivs_release_IO(void)
{
    if(theadivs_data->hwregs)
    {
        iounmap((void *) theadivs_data->hwregs);
        release_mem_region(theadivs_data->base_addr, theadivs_data->iosize);
        theadivs_data->hwregs = NULL;
    }
}

int __init theadivs_probe(struct platform_device *pdev)
{
    int result = -1;
    struct resource *mem;
    printk("enter %s\n",__func__);

    theadivs_data = (theadivs_dev *)vmalloc(sizeof(theadivs_dev));
    if (theadivs_data == NULL)
        return result;
    memset(theadivs_data, 0, sizeof(theadivs_dev));

    mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(mem->start)
        theadivs_data->base_addr = mem->start; 
    theadivs_data->irq = platform_get_irq(pdev, 0);
    printk("%s:get irq %d\n", __func__, theadivs_data->irq);
    theadivs_data->iosize = IVS_SWREG_AMOUNT * 4;
#if 1
	if (device_register_index == 0)
	{
		if (theadivs_major == 0)
		{
			result = alloc_chrdev_region(&theadivs_data->devt, 0, 1, "shake");
			if (result != 0)
			{
				printk("%s:alloc_chrdev_region error\n", __func__);
				goto err1;
			}
			theadivs_major = MAJOR(theadivs_data->devt);
			theadivs_minor = MINOR(theadivs_data->devt);
		}
		else
		{
			theadivs_data->devt = MKDEV(theadivs_major, theadivs_minor);
			result = register_chrdev_region(theadivs_data->devt, 1, "shake");
			if (result)
			{
				printk("%s:register_chrdev_region error\n", __func__);
				goto err1;
			}
		}

		theadivs_class = class_create(THIS_MODULE, "shake");
		if (IS_ERR(theadivs_class))
		{
			printk("%s[%d]:class_create error!\n", __func__, __LINE__);
			goto err;
		}
	}
	theadivs_data->devt = MKDEV(theadivs_major, theadivs_minor + pdev->id);

	cdev_init(&theadivs_data->cdev, &theadivs_fops);
	result = cdev_add(&theadivs_data->cdev, theadivs_data->devt, 1);
	if ( result )
	{
		printk("%s[%d]:cdev_add error!\n", __func__, __LINE__);
		goto err;
	}
	theadivs_data->class = theadivs_class;
	device_create(theadivs_data->class, NULL, theadivs_data->devt,
			theadivs_data, "shake");

	device_register_index++;
#else
    result = register_chrdev(theadivs_major, "shake", &theadivs_fops);
    if (result < 0)
    {
        printk(KERN_INFO "theadivs_driver: unable to get major <%d>\n",
              theadivs_major);
        goto err1;
    }
    else if (result != 0)    /* this is for dynamic major */
    {
        theadivs_major = result;
    }
#endif

    theadivs_reserve_IO();

    /* get the IRQ line */
    if (theadivs_data->irq!= -1)
    {
        result = request_irq(theadivs_data->irq, theadivs_isr,
                              IRQF_SHARED,
                              "shake", (void *)theadivs_data);
        if (result == -EINVAL)
        {
            printk(KERN_ERR "theadivs_driver: Bad irq number or handler.\n");
            theadivs_release_IO();
            goto err;
        }
        else if (result == -EBUSY)
        {
            printk(KERN_ERR "theadivs_driver: IRQ <%d> busy, change your config.\n",
                    theadivs_data->irq);
            theadivs_release_IO();
            goto err;
        }
    }
    else
    {
        printk(KERN_INFO "theadivs_driver: IRQ not in use!\n");
    }

    printk(KERN_INFO "theadivs_driver: module inserted. Major <%d>\n", theadivs_major);

    return 0;
err:
    //unregister_chrdev(theadivs_major, "shake");
    unregister_chrdev_region(theadivs_data->devt, 1);
err1:
    if (theadivs_data != NULL)
        vfree(theadivs_data);
    printk(KERN_INFO "theadivs_driver: module not inserted\n");
    return result;
}

static int theadivs_remove(struct platform_device *pdev)
{
    free_irq(theadivs_data->irq, theadivs_data);
    theadivs_release_IO();
    //unregister_chrdev(theadivs_major, "shake");
    device_register_index--;
	cdev_del(&theadivs_data->cdev);
	device_destroy(theadivs_data->class, theadivs_data->devt);
    unregister_chrdev_region(theadivs_data->devt, 1);
	if (device_register_index == 0)
	{
		class_destroy(theadivs_data->class);
	}
    if (theadivs_data != NULL)
        vfree(theadivs_data);

    printk(KERN_INFO "theadivs_driver: module removed\n");
    return 0;
}


static struct platform_driver theadivs_driver = {
    .probe = theadivs_probe,
    .remove = theadivs_remove,
    .driver = {
        .name = "shake",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(thead_of_match),
    }
};


int __init theadivs_init(void)
{
    int ret = 0;
    printk("enter %s\n",__func__);
#if 1
    ret = platform_driver_register(&theadivs_driver);
    if (ret)
    {
        pr_err("register platform driver failed!\n");
    }
#endif
    return ret;
}

void __exit theadivs_cleanup(void)
{
    printk("enter %s\n",__func__);
    platform_driver_unregister(&theadivs_driver);
    return;
}

static irqreturn_t theadivs_isr(int irq, void *dev_id)
{
    theadivs_dev *dev = (theadivs_dev *) dev_id;
    u32 irq_status = 0;

    printk( "theadivs_isr: received IRQ!\n");
    irq_status = (u32)ioread32((void*)dev->hwregs + 0x38); // INT_STATE
    printk( "INT_STATE of is: 0x%x\n", irq_status);
    theadivs_data->state = THEADIVS_ERROR;

    iowrite32(1, theadivs_data->hwregs + 0x40); // INT_MASK
    iowrite32(1, theadivs_data->hwregs + 0x3c); // INT_CLEAN
    iowrite32(1, theadivs_data->hwregs + 0x28); // CLEAR
    iowrite32(0, theadivs_data->hwregs + 0x40); // INT_MASK

    return IRQ_HANDLED;
}


module_init(theadivs_init);
module_exit(theadivs_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("T-HEAD");
MODULE_DESCRIPTION("T-HEAD ISP-VENC-SHAKE driver");

