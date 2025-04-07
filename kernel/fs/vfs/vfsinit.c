#include <kernel/vfs.h>

/**
 * vfs_init - Initialize the VFS subsystem
 *
 * Initializes all the core VFS components in proper order.
 * Must be called early during kernel initialization before
 * any filesystem operations can be performed.
 */
int32 vfs_init(void) {
	int32 err;
	mcache_init();

	/* Initialize the dcache subsystem */
	kprintf("VFS: Initializing dentry cache...\n");
	err = init_dentry_hashtable();
	if (err < 0) {
		kprintf("VFS: Failed to initialize dentry cache\n");
		return err;
	}

	/* Initialize the inode subsystem */
	kprintf("VFS: Initializing inode cache...\n");
	err = icache_init();
	if (err < 0) {
		kprintf("VFS: Failed to initialize inode cache\n");
		return err;
	}

	kprintf("VFS: registering block devices...\n");
	err = blockdevice_register_all();
	if (err < 0) {
		kprintf("VFS: Failed to register block devices\n");
		return err;
	}


	/* Register built-in filesystems */
	kprintf("VFS: Registering built-in filesystems...\n");
	err = fstype_register_all();
	if (err < 0) {
		kprintf("VFS: Failed to register filesystems\n");
		return err;
	}

	kprintf("VFS: Initialization complete\n");
	return 0;
}