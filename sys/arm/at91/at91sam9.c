/*-
 * Copyright (c) 2005 Olivier Houchard.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/arm/at91/at91sam9.c,v 1.1 2010/07/14 00:48:53 cognet Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/module.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/pmap.h>
#include <vm/vm_page.h>
#include <vm/vm_extern.h>

#define	_ARM32_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/intr.h>
#include <arm/at91/at91_aicreg.h>
#include <arm/at91/at91sam9g20reg.h>
#include <arm/at91/at91var.h>

static struct at91_softc *at91_softc;

static void at91_eoi(void *);

uint32_t at91_master_clock = AT91C_MASTER_CLOCK;

static int
at91_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int flags,
    bus_space_handle_t *bshp)
{
	vm_paddr_t pa, endpa;

	pa = trunc_page(bpa);
	if (pa >= 0xfff00000) {
		*bshp = pa - 0xf0000000 + 0xd0000000;
		return (0);
	}
	if (pa >= 0xdff00000)
		return (0);
	endpa = round_page(bpa + size);

	*bshp = (vm_offset_t)pmap_mapdev(pa, endpa - pa);
		       
	return (0);
}

static void
at91_bs_unmap(void *t, bus_space_handle_t h, bus_size_t size)
{
	vm_offset_t va, endva;

	va = trunc_page((vm_offset_t)t);
	endva = va + round_page(size);

	/* Free the kernel virtual mapping. */
	kmem_free(kernel_map, va, endva - va);
}

static int
at91_bs_subregion(void *t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;
	return (0);
}

static void
at91_barrier(void *t, bus_space_handle_t bsh, bus_size_t size, bus_size_t b, 
    int a)
{
}

bs_protos(generic);
bs_protos(generic_armv4);

struct bus_space at91_bs_tag = {
	/* cookie */
	(void *) 0,

	/* mapping/unmapping */
	at91_bs_map,
	at91_bs_unmap,
	at91_bs_subregion,

	/* allocation/deallocation */
	NULL,
	NULL,

	/* barrier */
	at91_barrier,

	/* read (single) */
	generic_bs_r_1,
	generic_armv4_bs_r_2,
	generic_bs_r_4,
	NULL,

	/* read multiple */
	generic_bs_rm_1,
	generic_armv4_bs_rm_2,
	generic_bs_rm_4,
	NULL,

	/* read region */
	generic_bs_rr_1,
	generic_armv4_bs_rr_2,
	generic_bs_rr_4,
	NULL,

	/* write (single) */
	generic_bs_w_1,
	generic_armv4_bs_w_2,
	generic_bs_w_4,
	NULL,

	/* write multiple */
	generic_bs_wm_1,
	generic_armv4_bs_wm_2,
	generic_bs_wm_4,
	NULL,

	/* write region */
	NULL,
	generic_armv4_bs_wr_2,
	generic_bs_wr_4,
	NULL,

	/* set multiple */
	NULL,
	NULL,
	NULL,
	NULL,

	/* set region */
	NULL,
	generic_armv4_bs_sr_2,
	generic_bs_sr_4,
	NULL,

	/* copy */
	NULL,
	generic_armv4_bs_c_2,
	NULL,
	NULL,

	/* read (single) stream */
	generic_bs_r_1,
	generic_armv4_bs_r_2,
	generic_bs_r_4,
	NULL,

	/* read multiple stream */
	generic_bs_rm_1,
	generic_armv4_bs_rm_2,
	generic_bs_rm_4,
	NULL,

	/* read region stream */
	generic_bs_rr_1,
	generic_armv4_bs_rr_2,
	generic_bs_rr_4,
	NULL,

	/* write (single) stream */
	generic_bs_w_1,
	generic_armv4_bs_w_2,
	generic_bs_w_4,
	NULL,

	/* write multiple stream */
	generic_bs_wm_1,
	generic_armv4_bs_wm_2,
	generic_bs_wm_4,
	NULL,

	/* write region stream */
	NULL,
	generic_armv4_bs_wr_2,
	generic_bs_wr_4,
	NULL,
};

static int
at91_probe(device_t dev)
{
	device_set_desc(dev, "AT91 device bus");
	arm_post_filter = at91_eoi;
	return (0);
}

static void
at91_identify(driver_t *drv, device_t parent)
{
	
	BUS_ADD_CHILD(parent, 0, "atmelarm", 0);
}

struct arm32_dma_range *
bus_dma_get_range(void)
{

	return (NULL);
}

int
bus_dma_get_range_nb(void)
{
	return (0);
}

extern void irq_entry(void);

static void
at91_add_child(device_t dev, int prio, const char *name, int unit,
    bus_addr_t addr, bus_size_t size, int irq0, int irq1, int irq2)
{
	device_t kid;
	struct at91_ivar *ivar;

	kid = device_add_child_ordered(dev, prio, name, unit);
	if (kid == NULL) {
	    printf("Can't add child %s%d ordered\n", name, unit);
	    return;
	}
	ivar = malloc(sizeof(*ivar), M_DEVBUF, M_NOWAIT | M_ZERO);
	if (ivar == NULL) {
		device_delete_child(dev, kid);
		printf("Can't add alloc ivar\n");
		return;
	}
	device_set_ivars(kid, ivar);
	resource_list_init(&ivar->resources);
	if (irq0 != -1)
		bus_set_resource(kid, SYS_RES_IRQ, 0, irq0, 1);
	if (irq1 != 0)
		bus_set_resource(kid, SYS_RES_IRQ, 1, irq1, 1);
	if (irq2 != 0)
		bus_set_resource(kid, SYS_RES_IRQ, 2, irq2, 1);
	if (addr != 0)
		bus_set_resource(kid, SYS_RES_MEMORY, 0, addr, size);
}

struct cpu_devs
{
	const char *name;
	int unit;
	bus_addr_t mem_base;
	bus_size_t mem_len;
	int irq0;
	int irq1;
	int irq2;
};

struct cpu_devs at91sam9_devs[] =
{
	{
		"at91_pit", 0,
		AT91SAM9G20_BASE + AT91SAM9G20_PIT_BASE, AT91SAM9G20_PIT_SIZE,
		AT91SAM9G20_IRQ_SYSTEM
	},
	{
		"at91_wdt", 0,
		AT91SAM9G20_BASE + AT91SAM9G20_WDT_BASE, AT91SAM9G20_WDT_SIZE,
		AT91SAM9G20_IRQ_SYSTEM
	},
	{
		"at91_pmc", 0,
		AT91SAM9G20_BASE + AT91SAM9G20_PMC_BASE, AT91SAM9G20_PMC_SIZE,
		AT91SAM9G20_IRQ_SYSTEM
	},
	{
		"at91_pio", 0,
		AT91SAM9G20_BASE + AT91SAM9G20_PIOA_BASE, AT91SAM9G20_PIO_SIZE,
		AT91SAM9G20_IRQ_SYSTEM
	},
	{
		"at91_pio", 1,
		AT91SAM9G20_BASE + AT91SAM9G20_PIOB_BASE, AT91SAM9G20_PIO_SIZE,
		AT91SAM9G20_IRQ_SYSTEM
	},
	{
		"at91_pio", 2,
		AT91SAM9G20_BASE + AT91SAM9G20_PIOC_BASE, AT91SAM9G20_PIO_SIZE,
		AT91SAM9G20_IRQ_SYSTEM
	},
	{
		"uart", 0,
		AT91SAM9G20_BASE + AT91SAM9G20_DBGU_BASE, AT91SAM9G20_DBGU_SIZE,
		AT91SAM9G20_IRQ_SYSTEM
	},
	{
		"uart", 1,
		AT91SAM9G20_BASE + AT91SAM9G20_USART0_BASE, AT91SAM9G20_USART_SIZE,
		AT91SAM9G20_IRQ_USART0
	},
	{
		"uart", 2,
		AT91SAM9G20_BASE + AT91SAM9G20_USART1_BASE, AT91SAM9G20_USART_SIZE,
		AT91SAM9G20_IRQ_USART1
	},
	{
		"uart", 3,
		AT91SAM9G20_BASE + AT91SAM9G20_USART2_BASE, AT91SAM9G20_USART_SIZE,
		AT91SAM9G20_IRQ_USART2
	},
	{
		"spi", 0,
		AT91SAM9G20_BASE + AT91SAM9G20_SPI0_BASE, AT91SAM9G20_SPI0_SIZE,
		AT91SAM9G20_IRQ_SPI0
	},
	{
		"spi", 1,
		AT91SAM9G20_BASE + AT91SAM9G20_SPI1_BASE, AT91SAM9G20_SPI1_SIZE,
		AT91SAM9G20_IRQ_SPI1
	},
	{
		"ohci", 0,
		AT91SAM9G20_OHCI_BASE, AT91SAM9G20_OHCI_SIZE,
		AT91SAM9G20_IRQ_UHP
	},
	{
		"macb", 0,
		AT91SAM9G20_BASE + AT91SAM9G20_EMAC_BASE, AT91SAM9G20_EMAC_SIZE,
		AT91SAM9G20_IRQ_EMAC
	},
	{
		"nand", 0,
		AT91SAM9G20_NAND_BASE, AT91SAM9G20_NAND_SIZE,
		-1
	},
	{	0, 0, 0, 0, 0 }
};

static void
at91_cpu_add_builtin_children(device_t dev, struct at91_softc *sc)
{
	int i;
	struct cpu_devs *walker;
	
	// XXX should look at the device id in the DBGU register and
	// XXX based on the CPU load in these devices
	for (i = 0, walker = at91sam9_devs; walker->name; i++, walker++) {
		at91_add_child(dev, i, walker->name, walker->unit,
		    walker->mem_base, walker->mem_len, walker->irq0,
		    walker->irq1, walker->irq2);
	}
}

#define NORMDEV 50

/*
 * Standard priority levels for the system.  0 is lowest and 7 is highest.
 * These values are the ones Atmel uses for its Linux port
 */
static int irq_prio[32] =
{
	7,	/* Advanced Interrupt Controller */
	7,	/* System Peripherals */
	1,	/* Parallel IO Controller A */
	1,	/* Parallel IO Controller B */
	1,	/* Parallel IO Controller C */
	0,	/* Analog-to-Digital Converter */
	5,	/* USART 0 */
	5,	/* USART 1 */
	5,	/* USART 2 */
	0,	/* Multimedia Card Interface */
	2,	/* USB Device Port */
	6,	/* Two-Wire Interface */
	5,	/* Serial Peripheral Interface 0 */
	5,	/* Serial Peripheral Interface 1 */
	5,	/* Serial Synchronous Controller */
	0,
	0,
	0,	/* Timer Counter 0 */
	0,	/* Timer Counter 1 */
	0,	/* Timer Counter 2 */
	2,	/* USB Host port */
	3,	/* Ethernet */
	0,	/* Image Sensor Interface */
	5,	/* USART 3 */
	5,	/* USART 4 */
	5,	/* USART 5 */
	0,	/* Timer Counter 3 */
	0,	/* Timer Counter 4 */
	0,	/* Timer Counter 5 */
	0,	/* Advanced Interrupt Controller */
	0,	/* Advanced Interrupt Controller */
	0,	/* Advanced Interrupt Controller */
};

static int
at91_attach(device_t dev)
{
	struct at91_softc *sc = device_get_softc(dev);
	int i;

	at91_softc = sc;
	sc->sc_st = &at91_bs_tag;
	sc->sc_sh = AT91SAM9G20_BASE;
	sc->dev = dev;
	if (bus_space_subregion(sc->sc_st, sc->sc_sh, AT91SAM9G20_SYS_BASE,
	    AT91SAM9G20_SYS_SIZE, &sc->sc_sys_sh) != 0)
		panic("Enable to map IRQ registers");
	sc->sc_irq_rman.rm_type = RMAN_ARRAY;
	sc->sc_irq_rman.rm_descr = "AT91 IRQs";
	sc->sc_mem_rman.rm_type = RMAN_ARRAY;
	sc->sc_mem_rman.rm_descr = "AT91 Memory";
	if (rman_init(&sc->sc_irq_rman) != 0 ||
	    rman_manage_region(&sc->sc_irq_rman, 1, 31) != 0)
		panic("at91_attach: failed to set up IRQ rman");
	if (rman_init(&sc->sc_mem_rman) != 0 ||
	    rman_manage_region(&sc->sc_mem_rman, 0xdff00000ul,
	    0xdffffffful) != 0)
		panic("at91_attach: failed to set up memory rman");
	if (rman_manage_region(&sc->sc_mem_rman, AT91SAM9G20_OHCI_BASE,
	    AT91SAM9G20_OHCI_BASE + AT91SAM9G20_OHCI_SIZE - 1) != 0)
		panic("at91_attach: failed to set up ohci memory");

	if (rman_manage_region(&sc->sc_mem_rman, AT91SAM9G20_NAND_BASE,
	    AT91SAM9G20_NAND_BASE + AT91SAM9G20_NAND_SIZE - 1) != 0)
		panic("at91_attach: failed to set up ohci memory");


#if 0
	if (rman_manage_region(&sc->sc_mem_rman, AT91SAM9G20_CF_BASE,
	    AT91SAM9G20_CF_BASE + AT91SAM9G20_CF_SIZE - 1) != 0)
		panic("at91_attach: failed to set up CompactFlash ATA memory");
#endif

	for (i = 0; i < 32; i++) {
		bus_space_write_4(sc->sc_st, sc->sc_sys_sh, 0x1000 + IC_SVR + 
		    i * 4, i);
		/* Priority. */
		bus_space_write_4(sc->sc_st, sc->sc_sys_sh, 0x1000 + IC_SMR + i * 4,
		    irq_prio[i]);
		if (i < 8)
			bus_space_write_4(sc->sc_st, sc->sc_sys_sh, 0x1000 + IC_EOICR,
			    1);
	}
	bus_space_write_4(sc->sc_st, sc->sc_sys_sh, 0x1000 + IC_SPU, 32);
	/* No debug. */
	bus_space_write_4(sc->sc_st, sc->sc_sys_sh, 0x1000 + IC_DCR, 0);
	/* Disable and clear all interrupts. */
	bus_space_write_4(sc->sc_st, sc->sc_sys_sh, 0x1000 + IC_IDCR, 0xffffffff);
	bus_space_write_4(sc->sc_st, sc->sc_sys_sh, 0x1000 + IC_ICCR, 0xffffffff);

	/* XXX */
	/* Disable all interrupts for RTC (0xe24 == RTC_IDR) */
	//bus_space_write_4(sc->sc_st, sc->sc_sys_sh, 0xe24, 0xffffffff);

	/* DIsable all interrupts for DBGU */
	bus_space_write_4(sc->sc_st, sc->sc_sys_sh, 0x120c, 0xffffffff);
	/* Disable all interrupts for the SDRAM controller */
	//bus_space_write_4(sc->sc_st, sc->sc_sys_sh, 0xfa8, 0xffffffff);


	i = bus_space_read_4(sc->sc_st,
			     sc->sc_sys_sh, AT91SAM9G20_EBICSA);

	/*activate NAND*/
	bus_space_write_4(sc->sc_st, sc->sc_sys_sh, AT91SAM9G20_EBICSA, 
		i | AT91_MATRIX_EBI_CS3A_SMC_SMARTMEDIA);
	

	at91_cpu_add_builtin_children(dev, sc);

	bus_generic_probe(dev);
	bus_generic_attach(dev);
	enable_interrupts(I32_bit | F32_bit);
	return (0);
}

static struct resource *
at91_alloc_resource(device_t dev, device_t child, int type, int *rid,
    u_long start, u_long end, u_long count, u_int flags)
{
	struct at91_softc *sc = device_get_softc(dev);
	struct resource_list_entry *rle;
	struct at91_ivar *ivar = device_get_ivars(child);
	struct resource_list *rl = &ivar->resources;

	if (device_get_parent(child) != dev)
		return (BUS_ALLOC_RESOURCE(device_get_parent(dev), child,
		    type, rid, start, end, count, flags));
	
	rle = resource_list_find(rl, type, *rid);
	if (rle == NULL)
		return (NULL);
	if (rle->res)
		panic("Resource rid %d type %d already in use", *rid, type);
	if (start == 0UL && end == ~0UL) {
		start = rle->start;
		count = ulmax(count, rle->count);
		end = ulmax(rle->end, start + count - 1);
	}
	switch (type)
	{
	case SYS_RES_IRQ:
		rle->res = rman_reserve_resource(&sc->sc_irq_rman,
		    start, end, count, flags, child);
		break;
	case SYS_RES_MEMORY:

		rle->res = rman_reserve_resource(&sc->sc_mem_rman,
		    start, end, count, flags, child);
		if (rle->res != NULL) {
			rman_set_bustag(rle->res, &at91_bs_tag);
			rman_set_bushandle(rle->res, start);
		}
		break;
	}
	if (rle->res) {
		rle->start = rman_get_start(rle->res);
		rle->end = rman_get_end(rle->res);
		rle->count = count;
		rman_set_rid(rle->res, *rid);
	}
	return (rle->res);
}

static struct resource_list *
at91_get_resource_list(device_t dev, device_t child)
{
	struct at91_ivar *ivar;

	ivar = device_get_ivars(child);
	return (&(ivar->resources));
}

static int
at91_release_resource(device_t dev, device_t child, int type,
    int rid, struct resource *r)
{
	struct resource_list *rl;
	struct resource_list_entry *rle;

	rl = at91_get_resource_list(dev, child);
	if (rl == NULL)
		return (EINVAL);
	rle = resource_list_find(rl, type, rid);
	if (rle == NULL)
		return (EINVAL);
	rman_release_resource(r);
	rle->res = NULL;
	return (0);
}

static int
at91_setup_intr(device_t dev, device_t child,
    struct resource *ires, int flags, driver_filter_t *filt, 
    driver_intr_t *intr, void *arg, void **cookiep)    
{
	struct at91_softc *sc = device_get_softc(dev);

	if (rman_get_start(ires) == AT91SAM9G20_IRQ_SYSTEM && filt == NULL)
		panic("All system interrupt ISRs must be FILTER");
	BUS_SETUP_INTR(device_get_parent(dev), child, ires, flags, filt, 
	    intr, arg, cookiep);
	bus_space_write_4(sc->sc_st, sc->sc_sys_sh, 0x1000 + IC_IECR,
	    1 << rman_get_start(ires));
	return (0);
}

static int
at91_teardown_intr(device_t dev, device_t child, struct resource *res,
    void *cookie)
{
	struct at91_softc *sc = device_get_softc(dev);

	bus_space_write_4(sc->sc_st, sc->sc_sys_sh, 0x1000 + IC_IDCR, 
	    1 << rman_get_start(res));
	return (BUS_TEARDOWN_INTR(device_get_parent(dev), child, res, cookie));
}

static int
at91_activate_resource(device_t bus, device_t child, int type, int rid,
    struct resource *r)
{
#if 0
	u_long p;
	int error;
	
	if (type == SYS_RES_MEMORY) {
		error = bus_space_map(rman_get_bustag(r),
		    rman_get_bushandle(r), rman_get_size(r), 0, &p);
		if (error) 
			return (error);
		rman_set_bushandle(r, p);
	}
#endif	
	return (rman_activate_resource(r));
}

static int
at91_print_child(device_t dev, device_t child)
{
	struct at91_ivar *ivars;
	struct resource_list *rl;
	int retval = 0;

	ivars = device_get_ivars(child);
	rl = &ivars->resources;

	retval += bus_print_child_header(dev, child);

	retval += resource_list_print_type(rl, "port", SYS_RES_IOPORT, "%#lx");
	retval += resource_list_print_type(rl, "mem", SYS_RES_MEMORY, "%#lx");
	retval += resource_list_print_type(rl, "irq", SYS_RES_IRQ, "%ld");
	if (device_get_flags(dev))
		retval += printf(" flags %#x", device_get_flags(dev));

	retval += bus_print_child_footer(dev, child);

	return (retval);
}

void
arm_mask_irq(uintptr_t nb)
{
	
	bus_space_write_4(at91_softc->sc_st, 
	    at91_softc->sc_sys_sh, 0x1000 + IC_IDCR, 1 << nb);

}

int
arm_get_next_irq(int last __unused)
{
	int status;
	int irq;
	
	irq = bus_space_read_4(at91_softc->sc_st,
	    at91_softc->sc_sys_sh, 0x1000 + IC_IVR);
	status = bus_space_read_4(at91_softc->sc_st,
	    at91_softc->sc_sys_sh, 0x1000 + IC_ISR);
	if (status == 0) {
		bus_space_write_4(at91_softc->sc_st,
		    at91_softc->sc_sys_sh, 0x1000 + IC_EOICR, 1);
		return (-1);
	}
	return (irq);
}

void
arm_unmask_irq(uintptr_t nb)
{
	
	bus_space_write_4(at91_softc->sc_st, 
	at91_softc->sc_sys_sh, 0x1000 + IC_IECR, 1 << nb);
	bus_space_write_4(at91_softc->sc_st, at91_softc->sc_sys_sh,
	    0x1000 + IC_EOICR, 0);

}

static void
at91_eoi(void *unused)
{
	bus_space_write_4(at91_softc->sc_st, at91_softc->sc_sys_sh,
	    0x1000 + IC_EOICR, 0);
}

static device_method_t at91_methods[] = {
	DEVMETHOD(device_probe, at91_probe),
	DEVMETHOD(device_attach, at91_attach),
	DEVMETHOD(device_identify, at91_identify),

	DEVMETHOD(bus_alloc_resource, at91_alloc_resource),
	DEVMETHOD(bus_setup_intr, at91_setup_intr),
	DEVMETHOD(bus_teardown_intr, at91_teardown_intr),
	DEVMETHOD(bus_activate_resource, at91_activate_resource),
	DEVMETHOD(bus_deactivate_resource, bus_generic_deactivate_resource),
	DEVMETHOD(bus_get_resource_list,at91_get_resource_list),
	DEVMETHOD(bus_set_resource,	bus_generic_rl_set_resource),
	DEVMETHOD(bus_get_resource,	bus_generic_rl_get_resource),
	DEVMETHOD(bus_release_resource,	at91_release_resource),
	DEVMETHOD(bus_print_child,	at91_print_child),

	{0, 0},
};

static driver_t at91_driver = {
	"atmelarm",
	at91_methods,
	sizeof(struct at91_softc),
};
static devclass_t at91_devclass;

DRIVER_MODULE(atmelarm, nexus, at91_driver, at91_devclass, 0, 0);