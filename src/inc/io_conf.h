#ifndef __IO_CONF_H
#define __IO_CONF_H

/*
 * I/O definitions
 */

// I/O configuration to monitor, modeled as a set of I/O memory blocks.
typedef struct {
	const void** addrs; // Set of address blocks
	const unsigned* sizes; // Size of each block in bytes
	const unsigned blocks; // Number of blocks
	const unsigned size; // Total size
} io_conf_t;

// Information about I/O change detection
typedef struct {
	void* target;     	// Target address
	long new_val;     	// I/O value after detection
	long old_val;     	// I/O value before (trusted)
	void* target_info;	// Extra target info (implementation defined)
} io_detect_t;

// I/O change detection handler.
extern void handle_io_detection(io_detect_t*);


/************************ I/O monitor interface ************************/

/*
 * The following interface should be implemented by the architecture specific header.
 * To optimize the code, all the functions must be defined as static inline.
 *
 * The system has access to I/O pins by using I/O memory.
 * Some address space in I/O memory is designated for configuration registers, used during I/O initialization process.
 * The number and the nature of these registers varies according to the specific architecture.
 * The I/O configuration registers can be leveraged to perform an I/O attack, e.g. by disabling or modifying specific I/O pins.
 *
 * The implementation, according to the target system, must specify which registers should be protected by the monitor,
 * providing low-level functions to get values from these registers, compare their values with trusted ones and restore them if necessary.
 */

/*
 * The implementation must define an io_conf_t object named phys_io_conf, filling all the necessary fields.
 * The structure must be statically initialized and const, so it will be put in the read-only section by the compiler.
 *
 * Declaration example:
 * static const io_conf_t phys_io_conf = {
 * 	<physical_values>
 * };
 *
 * The structure will be then accessed through the following pointer, allocated and freed by the monitor.
 * The pointer can also be dereferenced and used from the implementation header, if preferred,
 * but the value of the pointer must not be modified.
 */
extern const io_conf_t* io_conf;

/*
 * The implementation should define the size (in bytes) needed to store the entire I/O configuration memory.
 * This size will be used by the monitor to allocate the necessary space.
 */
#define IO_STATE_TOTAL_SIZE	__IO_STATE_TOTAL_SIZE

/*
 * Get the current state from I/O memory pointed by the given set of virtual addresses and store it into the given pointer.
 * @state is allocated and freed by the monitor, the implementation should just use it to store the data.
 * The state should be saved contiguously in the memory region pointed by @state, having io_conf->size bytes.
 *
 * @addrs: set of virtual base addresses of I/O blocks (their size is known for the implementation)
 * @state: pointer to the I/O state data structure, _already_ allocated
 */
static inline void get_io_state(volatile void** addrs, void* state);

/*
 * Iterate over the addresses of each I/O block to compare the current state in I/O memory with the trusted one.
 * This interface allows the implementation to decide size and alignment of each I/O memory access
 * as required by the specific architecture reference manual, thus optimizing the code.
 * Different I/O blocks may contain I/O registers with different sizes, requiring different access types.
 * The access type may vary according to which block is accessed; therefore, the implementation may need
 * to know which is the specified block (@index). The @index also allows it to get the size of the block,
 * which doesn't need to be passed as argument (io_conf->io_sizes[index]).
 * For each detected change the implementation should fill detect_info_t and notify handle_io_detection().
 * The information contained into detect_info_t must be enough to eventually restore the trusted state later.
 *
 * @block: the block base address (virtual)
 * @trusted_state: the trusted state base address, relative to this block
 * @index: the index of the block, that is, the position inside io_conf->addrs
 */
static inline void check_io_state(volatile void* block, const void* state, unsigned index);

/*
 * Restore the I/O memory to its trusted state.
 * detect_info_t contains an extra opaque pointer (@target_info) which can be managed (allocated and freed)
 * by the specific implementation, in order to apply the correct access type needed by the target address.
 * The target_info opaque pointer will not be used by the monitor after a call to restore_io_state(), so it must be freed here.
 *
 * @info: detection info pointer, as filled in by check_io_state()
 */
static inline void restore_io_state(io_detect_t* info);


/*
 * Include architecture-dependent I/O header.
 */

#include "io_impl.h"


#endif