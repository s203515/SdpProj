#include <types.h>
#include <kern/errno.h>
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
#include <spinlock.h>
#include <mainbus.h>
#include <vfs.h>
#include <device.h>
#include <syscall.h>
#include <test.h>
#include <version.h>
#include "autoconf.h"  // for pseudoconfig
#include <addrspace.h>


void sys_exit(int status){
	//struct addrspace *as;
#if OPT_WAITPID
	struct proc *p = curproc;
	struct thread *cur = curthread;
	proc_remthread(cur);
	lock_acquire(p->lock);
	p->exitCode = status;
	
	cv_signal(p->cv,p->lock);   
	lock_release(p->lock);
	
#endif
	/*
	Now handled by proc_wait (proc.c)
	as=proc_getas();
	as_destroy(as);
	*/
	thread_exit();	
	


	(void)status;
}
#if OPT_WAITPID
pid_t sys_waitpid(pid_t pid ,int *status ){
	struct proc *p = getproc(pid);
	lock_acquire(p->lock);
	*status = proc_wait(p);
	
	return pid;
	
}

pid_t sys_getpid (struct proc *p){
		
	return p->pid;
}
#endif





