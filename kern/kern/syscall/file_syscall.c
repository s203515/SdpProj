#include <types.h>
#include <kern/errno.h>
#include <kern/stat.h>
#include <kern/reboot.h>
#include <kern/unistd.h>
#include <lib.h>
#include <spl.h>
#include <clock.h>
#include <thread.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <vm.h>
#include <mainbus.h>
#include <vfs.h>
#include <device.h>
#include <syscall.h>
#include <test.h>
#include <version.h>
#include <uio.h>
#include <elf.h>
#include <vnode.h>
#include "autoconf.h"  // for pseudoconfig
#include "opt-shell.h"

#define SEEK_SET      0      /* Seek relative to beginning of file */
#define SEEK_CUR      1      /* Seek relative to current position in file */
#define SEEK_END      2      /* Seek relative to end of file */

int sys_write(int fd, const void *buf, int size ) 	{
	char* x;
	int result;
	struct iovec iov;
	struct uio u;
	struct proc *proc =curproc;
	struct vnode *vnode;
	struct addrspace *as;
	x=(char *)buf;	
	if(fd!=STDOUT_FILENO && fd!=STDERR_FILENO){
		//kprintf("Only STDIN and STDOUT are implemented \n");
		//return -1;
		as = proc_getas();

		iov.iov_ubase = (userptr_t)buf;
		iov.iov_len = size;		 // length of the memory space
		u.uio_iov = &iov;
		u.uio_iovcnt = 1;
		u.uio_resid = size;          // amount to read from the file
#if OPT_SHELL
		u.uio_offset = proc->open_files[fd-3]->offset;
#else
		u.uio_offset = 0;
#endif
		u.uio_segflg = UIO_USERSPACE;
		u.uio_rw = UIO_WRITE;
		u.uio_space = as;
	
		vnode = getvnode(proc,fd);	
	
		result = VOP_WRITE(vnode, &u);
	
		return result;	

	}
	for(int i=0; i<size; i++){
		putch(x[i]);		
	}			
	return size;
}

int sys_read(int fd, const void *buf, int size ) 	{
	char* x;
	int result;
	struct iovec iov;
	struct uio u;
	struct proc *proc =curproc;
	struct vnode *vnode;
	struct addrspace *as;
	x=(char *)buf;	
	if(fd!=STDIN_FILENO){
		//kprintf("Only STDIN and STDOUT are implemented \n");
		//return -1;
	/* read from file    */

	as = proc_getas();

	iov.iov_ubase = (userptr_t)buf;
	iov.iov_len = size;		 // length of the memory space
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_resid = size;          // amount to read from the file
#if OPT_SHELL
	u.uio_offset = proc->open_files[fd-3]->offset;
#else
	u.uio_offset = 0;
#endif
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_READ;
	u.uio_space = as;
	
	vnode = getvnode(proc,fd);	
	
	result = VOP_READ(vnode, &u);
	
	return result;	
	
	}
	
	for(int i=0; i<size; i++){
		x[i]=getch();
		if(x[i]<0)
			return i;
	}
			
	return size;
}

int sys_open (const char *path, int oflag){
	mode_t mode=0;
	struct vnode *vnode;
	struct proc *proc = curproc;
	int fd;

	if(vfs_open((char*)path,oflag,mode,&vnode)){
		return -1;
	}
	
	fd = fd_assign(proc,vnode);

	return fd;
}
int sys_close(int fd){
	int ret;
	struct proc *proc = curproc;
	struct vnode *vnode;
	vnode = getvnode(proc,fd);
	vfs_close(vnode);
	ret = fd_remove(proc,fd);
	return ret;
}
#if OPT_SHELL
off_t sys_lseek(int filehandle, off_t pos, int code,int *err){
//in case of failure returns -1
//code can be SEEK_SET SEEK_CUR SEEK_END
	struct proc *proc = curproc;
	struct openfile *of;
	struct stat st;  //used for VOP_STAT
	off_t retval;	
	off_t size;

	of = proc->open_files[filehandle-3];
	
	if(of == NULL){
		*err = EBADF;
		return -1;	
	}
	if(!VOP_ISSEEKABLE(of->vn)){
		*err = ESPIPE;
		return -1;
	}
	if(VOP_STAT(of->vn,&st)){  
		return -1;   //failed on stat
	}
	size = st.st_size;
	
	switch(code){
		case SEEK_SET:   //from the beginning of the file
		if(pos > size || pos < 0){  
			*err = EINVAL;
			return -1;
		}
		of->offset = pos;
		retval = pos;
		break;
		
 		case SEEK_CUR:
		retval = of->offset + pos;
		if(retval>size || retval < 0){
			*err = EINVAL;
			return -1;
		}
		of->offset = of->offset + pos;
		break;
		
		case SEEK_END:
		retval = pos + size;		
		if(retval > size || retval < 0){
			*err = EINVAL;
			return -1;
		}
		of->offset = pos + size;
		break;
		
		default: 
			*err = EINVAL;
			return -1;
		break;
	}
	return retval;
}
#endif
