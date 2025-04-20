#include <kernel.h>
#include <kernel/device/buffer_head.h>
#include <kernel/drivers/virtio_device.h>
#include <kernel/fs/ext4_adaptor.h>

int32 do_root_mount(const char* source, const char* target, const char* fstype_name, uint64 flags, const void* data) {
	target;
	int ret = 0;
	struct ext4_blockdev* e_blockdevice = ext4_blockdev_create_adapter(&virtio_bd);
	CHECK_PTR_VALID(e_blockdevice, PTR_TO_ERR(e_blockdevice));
	ret = ext4_device_register(e_blockdevice, "virtio");
	if (ret != EOK) {
		kprintf("ext4_device_register failed: %d\n", ret);
		kfree(e_blockdevice);
		return ret;
	}
	/* ext4 */
	kprintf("do_root_mount: checking  EXT4\n");
	char block2_buffer[512];
	virtio_bd.bd_ops->read_blocks(&virtio_bd, block2_buffer, 2, 1);
	if (!(block2_buffer[56] == 0x53 && block2_buffer[57] == 0xEF)) {
		return -ENOSYS; /* can't get filesystem type */
	}
	for (int i = 0; i < 512; i += 8) {
		if(i % 64 == 0) kprintf("\n");
		kprintf("%x ", block2_buffer[i]);
	}
	kprintf("do_root_mount: yes it's ext4\n");
	return ext4_adapter_mount(&ext4_fs_type, flags, data);
}

/**
 * do_mount - 挂载文件系统到指定挂载点
 * @source: 源设备名称
 * @target: 挂载点路径
 * @fstype_name: 文件系统类型
 * @flags: 挂载标志
 * @data: 文件系统特定数据
 *
 * 此函数是系统调用 sys_mount 的主要实现。它处理不同类型的挂载操作，
 * 包括常规挂载、bind挂载和重新挂载。
 *
 * 返回: 成功时返回0，失败时返回负的错误码
 */
static int root_mount_flag = 1;
// 因为没有做ramfs，所以说硬编码一个第一次做mount的flag。
int32 do_mount(const char* source, const char* target, const char* fstype_name, uint64 flags, const void* data) {
	struct path mount_path;
	struct fstype* type;
	struct vfsmount* newmnt;
	int32 ret;
	if (root_mount_flag) {
		root_mount_flag = 0;
		return do_root_mount(source, target, fstype_name, flags, data);
	}
	/* 查找文件系统类型 */
	type = fstype_lookup(fstype_name);
	if (!type) return -ENODEV;

	/* 查找挂载点路径 */
	ret = path_create(target, 0, &mount_path);
	if (ret) return ret;

	/* 处理特殊挂载标志 */
	if (flags & MS_BIND) {
		/* Bind mount 处理 */
		struct path source_path;
		ret = path_create(source, 0, &source_path);
		if (ret) goto out_path;

		newmnt = mount_bind(&source_path, flags);
		path_destroy(&source_path);
	} else if (flags & MS_REMOUNT) {
		/* 重新挂载处理 */
		ret = remount(&mount_path, flags, data);
		goto out_path;
	} else {
		/* 常规挂载 */

		newmnt = vfs_kern_mount(type, flags, source, data);
	}

	if (PTR_IS_ERROR(newmnt)) {
		ret = PTR_TO_ERR(newmnt);
		goto out_path;
	}

	/* 添加新挂载点到挂载点层次结构 */
	ret = mount_add(newmnt, &mount_path, flags);
	if (ret) mount_unref(newmnt);

out_path:
	path_destroy(&mount_path);
	return ret;
}