#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <syscall.h>

#include <kern/unistd.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>
#define PATH_MAX 1024
/*
 * System call handler.
 *
 * A pointer to the trapframe created during exception entry (in
 * exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary
 * function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. In addition, the system call number is
 * passed in the v0 register.
 *
 * On successful return, the return value is passed back in the v0
 * register, like an ordinary function call, and the a3 register is
 * also set to 0 to indicate success.
 *
 * On an error return, the error code is passed back in the v0
 * register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and
 * returning the value -1 from the actual userlevel syscall function.
 * See src/lib/libc/syscalls.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe
 * must be incremented by one instruction; otherwise the exception
 * return code will restart the "syscall" instruction and the system
 * call will repeat forever.
 *
 * Since none of the OS/161 system calls have more than 4 arguments,
 * there should be no need to fetch additional arguments from the
 * user-level stack.
 *
 * Watch out: if you make system calls that have 64-bit quantities as
 * arguments, they will get passed in pairs of registers, and not
 * necessarily in the way you expect. We recommend you don't do it.
 * (In fact, we recommend you don't use 64-bit quantities at all. See
 * arch/mips/include/types.h.)
 */

void
mips_syscall(struct trapframe *tf)
{
        int callno;
        int32_t retval;
        int err;

        assert(curspl==0);

        callno = tf->tf_v0;

        /*
         * Initialize retval to 0. Many of the system calls don't
         * really return a value, just 0 for success and -1 on
         * error. Since retval is the value returned on success,
         * initialize it to 0 by default; thus it's not necessary to
         * deal with it except for calls that return other values, 
         * like write.
         */

        retval = 0;

        switch (callno) {
            case SYS_reboot:
                err = sys_reboot(tf->tf_a0);
                break;
	case SYS__exit:
		sys_exit();
		break;

        case SYS_read:
            retval = sys_read(tf->tf_a0, (char *)tf->tf_a1, tf->tf_a2);
            err = (retval < 0 ? retval : 0);
        break;

        case SYS_write:
            retval = sys_write(tf->tf_a0, (char *)tf->tf_a1, tf->tf_a2);
            err = (retval < 0 ? retval : 0);
            break;

	case SYS_sbrk:
		kprintf("\nBefore SBRK\n");
		err=sys_sbrk(tf->tf_a0,&retval);
		
		kprintf("\nAfter SBRK:ERR:%d,tf-tf_a0:%d,retval:%d\n",err,tf->tf_a0,retval);
		break;
            /* Add stuff here */
        case SYS_fork:
	err = sys_fork(tf, &retval);
		  break;
	 case SYS_getpid:        	
                 err = sys_getpid((pid_t *)&retval); 
                break;
        case SYS_execv:
                err = sys_execv((userptr_t)tf->tf_a0, (userptr_t)tf->tf_a1);
		break;
            default:
                err = ENOSYS;
                break;
        }


        if (err) {
                /*
                 * Return the error code. This gets converted at
                 * userlevel to a return value of -1 and the error
                 * code in errno.
                 */
                tf->tf_v0 = err;
                tf->tf_a3 = 1;      /* signal an error */
        }
/*
	else if (err == 498)
	{
		kprintf("\nNo error 498\n");
		tf->tf_v0 = retval;
		tf->tf_a3 = 0;
	}
*/  
      else  {
                /* Success. */
                tf->tf_v0 = retval;
                tf->tf_a3 = 0;      /* signal no error */
        }

        /*
         * Now, advance the program counter, to avoid restarting
         * the syscall over and over again.
         */

	
        tf->tf_epc += 4;

        /* Make sure the syscall code didn't forget to lower spl */
        assert(curspl==0);
}


