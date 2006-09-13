/*
 * Cisco 7200 (Predator) simulation platform.
 * Copyright (c) 2005,2006 Christophe Fillot (cf@utc.fr)
 *
 * Virtual Machines.
 */

#ifndef __VM_H__
#define __VM_H__

#include <pthread.h>

#include "mips64.h"
#include "dynamips.h"
#include "memory.h"
#include "cpu.h"
#include "dev_vtty.h"

#define VM_PCI_POOL_SIZE  32

/* VM instance status */
enum {   
   VM_STATUS_HALTED = 0,      /* VM is halted and no HW resources are used */
   VM_STATUS_SHUTDOWN,        /* Shutdown procedure engaged */
   VM_STATUS_RUNNING,         /* VM is running */
   VM_STATUS_SUSPENDED,       /* VM is suspended */
};

/* VM types */
enum {
   VM_TYPE_C7200 = 0,
   VM_TYPE_C3600,
};

/* Timer IRQ check interval */
#define VM_TIMER_IRQ_CHECK_ITV  1000

/* forward declarations */
typedef struct vm_obj vm_obj_t;

/* Shutdown function prototype for an object */
typedef void *(*vm_shutdown_t)(vm_instance_t *vm,void *data);

/* VM object, used to keep track of devices and various things */
struct vm_obj {
   char *name;
   void *data;
   struct vm_obj *next,**pprev;
   vm_shutdown_t shutdown;
};

/* VM instance */
struct vm_instance {
   char *name;
   int type;                       /* C7200, C3600, ... */
   int status;                     /* Instance status */
   int instance_id;                /* Instance Identifier */
   char *lock_file;                /* Lock file */
   char *log_file;                 /* Log file */
   u_int ram_size,rom_size;        /* RAM and ROM size in Mb */
   u_int iomem_size;               /* IOMEM size in Mb */
   u_int nvram_size;               /* NVRAM size in Kb */
   u_int pcmcia_disk_size[2];      /* PCMCIA disk0 and disk1 sizes (in Mb) */
   u_int conf_reg,conf_reg_setup;  /* Config register */
   u_int clock_divisor;            /* Clock Divisor (see cp0.c) */
   u_int ram_mmap;                 /* Memory-mapped RAM ? */
   u_int restart_ios;              /* Restart IOS on reload ? */
   u_int elf_machine_id;           /* ELF machine identifier */
   u_int exec_area_size;           /* Size of execution area for CPU */
   m_uint32_t ios_entry_point;     /* IOS entry point */
   char *ios_image;                /* IOS image filename */
   char *ios_config;               /* IOS configuration file */
   char *rom_filename;             /* ROM filename */
   char *sym_filename;             /* Symbol filename */
   FILE *lock_fd,*log_fd;          /* Lock/Log file descriptors */
   int debug_level;                /* Debugging Level */
   int jit_use;                    /* CPUs use JIT */

   /* Basic hardware: system CPU, PCI busses and PCI I/O space */
   cpu_group_t *cpu_group;
   cpu_mips_t *boot_cpu;
   struct pci_bus *pci_bus[2];
   struct pci_bus *pci_bus_pool[VM_PCI_POOL_SIZE];
   struct pci_io_data *pci_io_space;

   /* Memory mapped devices */
   struct vdevice *dev_list;
   struct vdevice *dev_array[MIPS64_DEVICE_MAX];

   /* "idling" pointer counter */
   m_uint64_t idle_pc;

   /* Timer IRQ interval check */
   u_int timer_irq_check_itv;

   /* Console and AUX port VTTY type and parameters */
   int vtty_con_type,vtty_aux_type;
   int vtty_con_tcp_port,vtty_aux_tcp_port;
   vtty_serial_option_t vtty_con_serial_option,vtty_aux_serial_option;

   /* Virtual TTY for Console and AUX ports */
   vtty_t *vtty_con,*vtty_aux;

   /* Space reserved in NVRAM by ROM monitor */
   u_int nvram_rom_space;

   /* Extract and push IOS configuration */
   ssize_t (*nvram_extract_config)(vm_instance_t *vm,char **buffer);
   int (*nvram_push_config)(vm_instance_t *vm,char *buffer,size_t len);

   /* Specific hardware data */
   void *hw_data;

   /* VM objects */
   struct vm_obj *vm_object_list;   
};

#define VM_C7200(vm) ((c7200_t *)vm->hw_data)
#define VM_C3600(vm) ((c3600_t *)vm->hw_data)

extern int vm_file_naming_type;

/* Initialize a VM object */
void vm_object_init(vm_obj_t *obj);

/* Add a VM object to an instance */
void vm_object_add(vm_instance_t *vm,vm_obj_t *obj);

/* Remove a VM object from an instance */
void vm_object_remove(vm_instance_t *vm,vm_obj_t *obj);

/* Find an object given its name */
vm_obj_t *vm_object_find(vm_instance_t *vm,char *name);

/* Check that a mandatory object is present */
int vm_object_check(vm_instance_t *vm,char *name);

/* Dump the object list of an instance */
void vm_object_dump(vm_instance_t *vm);

/* Get VM type */
char *vm_get_type(vm_instance_t *vm);

/* Generate a filename for use by the instance */
char *vm_build_filename(vm_instance_t *vm,char *name);

/* Check that an instance lock file doesn't already exist */
int vm_get_lock(vm_instance_t *vm);

/* Erase lock file */
void vm_release_lock(vm_instance_t *vm,int erase);

/* Log a message */
void vm_flog(vm_instance_t *vm,char *module,char *format,va_list ap);

/* Log a message */
void vm_log(vm_instance_t *vm,char *module,char *format,...);

/* Close the log file */
int vm_close_log(vm_instance_t *vm);

/* Create the log file */
int vm_create_log(vm_instance_t *vm);

/* Error message */
void vm_error(vm_instance_t *vm,char *format,...);

/* Create a new VM instance */
vm_instance_t *vm_create(char *name,int instance_id,int machine_type);

/* Shutdown hardware resources used by a VM */
int vm_hardware_shutdown(vm_instance_t *vm);

/* Free resources used by a VM */
void vm_free(vm_instance_t *vm);

/* Get an instance given a name */
vm_instance_t *vm_acquire(char *name);

/* Release a VM (decrement reference count) */
int vm_release(vm_instance_t *vm);

/* Initialize VTTY */
int vm_init_vtty(vm_instance_t *vm);

/* Delete VTTY */
void vm_delete_vtty(vm_instance_t *vm);

/* Bind a device to a virtual machine */
int vm_bind_device(vm_instance_t *vm,struct vdevice *dev);

/* Unbind a device from a virtual machine */
int vm_unbind_device(vm_instance_t *vm,struct vdevice *dev);

/* Map a device at the specified physical address */
int vm_map_device(vm_instance_t *vm,struct vdevice *dev,m_uint64_t base_addr);

/* Set an IRQ for a VM */
void vm_set_irq(vm_instance_t *vm,u_int irq);

/* Clear an IRQ for a VM */
void vm_clear_irq(vm_instance_t *vm,u_int irq);

/* Suspend a VM instance */
int vm_suspend(vm_instance_t *vm);

/* Resume a VM instance */
int vm_resume(vm_instance_t *vm);

/* Stop an instance */
int vm_stop(vm_instance_t *vm);

/* Monitor an instance periodically */
void vm_monitor(vm_instance_t *vm);

/* Save the Cisco IOS configuration from NVRAM */
int vm_ios_save_config(vm_instance_t *vm);

/* Set Cisco IOS image to use */
int vm_ios_set_image(vm_instance_t *vm,char *ios_image);

/* Unset a Cisco IOS configuration file */
void vm_ios_unset_config(vm_instance_t *vm);

/* Set Cisco IOS configuration file to use */
int vm_ios_set_config(vm_instance_t *vm,char *ios_config);

/* Extract IOS configuration from NVRAM and write it to a file */
int vm_nvram_extract_config(vm_instance_t *vm,char *filename);

/* Read an IOS configuraton from a file and push it to NVRAM */
int vm_nvram_push_config(vm_instance_t *vm,char *filename);

/* Save general VM configuration into the specified file */
void vm_save_config(vm_instance_t *vm,FILE *fd);

#endif
