#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/io.h>

#include "io_monitor.h"
#include "io_conf.h"
#include "io_debug.h"

static const io_conf_t* io_conf; // Physical I/O configuration
static volatile void** addrs; // I/O virtual addresses
static const void* trusted_state; // Trusted state in I/O memory
static struct task_struct* task; // I/O monitor main task

static int monitor_loop(void* data);
static int map_addrs(void);
static void unmap_addrs(int mapped);

int start_io_monitor() {
	int res;

	// Get model-specific physical I/O configuration
	io_conf = PHYS_IO_CONF;

	// Map I/O physical address to kernel virtual addresses
	res = map_addrs();
	if (res) goto map_failed;

	// Allocate space for trusted state
	trusted_state = kmalloc(io_conf->size, GFP_KERNEL);
	if (!trusted_state) {
		log_err("Unable to allocate kernel space for I/O configuration monitor\n");
		res = -ENOMEM;
		goto trusted_failed;
	}

	// Read trusted state from I/O memory
	get_io_state(addrs, (void*)trusted_state);
	
	// Start monitor task
	task = kthread_run(&monitor_loop, NULL, "io_monitor");
	if (IS_ERR((void*)task)) {
		log_err("Unable to create thread: %ld\n", PTR_ERR((void*)task));
		res = PTR_ERR((void*)task);
		goto task_failed;
	}

	log_info("I/O configuration monitor started\n");
	return 0;

task_failed:
	kfree(trusted_state);
trusted_failed:
	unmap_addrs(io_conf->blocks);
map_failed:
	return res;
}

static int monitor_loop(void* data) {
	unsigned b, offset;

	dump_io_state();

	while (1) {
		// Check I/O blocks
		for (b = 0, offset = 0; b < io_conf->blocks; offset += io_conf->sizes[b++]) { // For each block
			// Read from I/O memory in an architecture-independent manner.
			check_io_state(addrs[b], trusted_state + offset, b);
		}

		if (kthread_should_stop()) return 0;
		msleep(IO_MONITOR_INTERVAL);
	}
}

void handle_io_detection(io_detect_t* info) {
	log_info("Change detected on I/O address 0x%08lx [old value = %ld, new value = %ld]\n",
		(long)info->target, info->old_val, info->new_val);
	dump_io_state();
	restore_io_state(info);
}

void stop_io_monitor(void) {
	kthread_stop(task);
	unmap_addrs(io_conf->blocks);
	kfree(trusted_state);
	log_info("I/O configuration monitor stopped\n");
}

static int map_addrs() {
	unsigned i;
	int res;

	addrs = kmalloc(sizeof(void*) * io_conf->blocks, GFP_KERNEL);
	if (!addrs) {
		log_err("Unable to allocate kernel space for I/O configuration monitor\n");
		return -ENOMEM;
	}

	for (i = 0; i < io_conf->blocks; i++) {
		addrs[i] = ioremap((phys_addr_t)io_conf->addrs[i], io_conf->sizes[i]);
		if (IS_ERR((void*)addrs[i])) {
			log_err("Unable to map I/O address %08lx\n", (long)io_conf->addrs[i]);
			res = PTR_ERR((void*)addrs[i]);
			goto iomap_failed;
		}
	}

	return 0;

iomap_failed:
	unmap_addrs(i);
	return res;
}

static void unmap_addrs(int mapped) {
	unsigned i;

	for (i = 0; i < mapped; i++) {
		iounmap(addrs[i]);
	}

	kfree(addrs);
}
