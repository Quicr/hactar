// Not used, but required for many linux builds
#ifndef _LINUX_PRCTL_H
#define _LINUX_PRCTL_H
/* Values to pass as first argument to prctl() */
#define PR_SET_PDEATHSIG  1  /* Second arg is a signal */
#define PR_GET_PDEATHSIG  2  /* Second arg is a ptr to return the signal */
/* Get/set current->mm->dumpable */
#define PR_GET_DUMPABLE   3
#define PR_SET_DUMPABLE   4
/* Get/set unaligned access control bits (if meaningful) */
#define PR_GET_UNALIGN	  5
#define PR_SET_UNALIGN	  6
# define PR_UNALIGN_NOPRINT	1	/* silently fix up unaligned user accesses */
# define PR_UNALIGN_SIGBUS	2	/* generate SIGBUS on unaligned user access */
/* Get/set whether or not to drop capabilities on setuid() away from
 * uid 0 (as per security/commoncap.c) */
#define PR_GET_KEEPCAPS   7
#define PR_SET_KEEPCAPS   8
/* Get/set floating-point emulation control bits (if meaningful) */
#define PR_GET_FPEMU  9
#define PR_SET_FPEMU 10
# define PR_FPEMU_NOPRINT	1	/* silently emulate fp operations accesses */
# define PR_FPEMU_SIGFPE	2	/* don't emulate fp operations, send SIGFPE instead */
/* Get/set floating-point exception mode (if meaningful) */
#define PR_GET_FPEXC	11
#define PR_SET_FPEXC	12
# define PR_FP_EXC_SW_ENABLE	0x80	/* Use FPEXC for FP exception enables */
# define PR_FP_EXC_DIV		0x010000	/* floating point divide by zero */
# define PR_FP_EXC_OVF		0x020000	/* floating point overflow */
# define PR_FP_EXC_UND		0x040000	/* floating point underflow */
# define PR_FP_EXC_RES		0x080000	/* floating point inexact result */
# define PR_FP_EXC_INV		0x100000	/* floating point invalid operation */
# define PR_FP_EXC_DISABLED	0	/* FP exceptions disabled */
# define PR_FP_EXC_NONRECOV	1	/* async non-recoverable exc. mode */
# define PR_FP_EXC_ASYNC	2	/* async recoverable exception mode */
# define PR_FP_EXC_PRECISE	3	/* precise exception mode */
/* Get/set whether we use statistical process timing or accurate timestamp
 * based process timing */
#define PR_GET_TIMING   13
#define PR_SET_TIMING   14
# define PR_TIMING_STATISTICAL  0       /* Normal, traditional,
                                                   statistical process timing */
# define PR_TIMING_TIMESTAMP    1       /* Accurate timestamp based
                                                   process timing */
#define PR_SET_NAME    15		/* Set process name */
#define PR_GET_NAME    16		/* Get process name */
/* Get/set process endian */
#define PR_GET_ENDIAN	19
#define PR_SET_ENDIAN	20
# define PR_ENDIAN_BIG		0
# define PR_ENDIAN_LITTLE	1	/* True little endian mode */
# define PR_ENDIAN_PPC_LITTLE	2	/* "PowerPC" pseudo little endian */
/* Get/set process seccomp mode */
#define PR_GET_SECCOMP	21
#define PR_SET_SECCOMP	22
/* Get/set the capability bounding set (as per security/commoncap.c) */
#define PR_CAPBSET_READ 23
#define PR_CAPBSET_DROP 24
/* Get/set the process' ability to use the timestamp counter instruction */
#define PR_GET_TSC 25
#define PR_SET_TSC 26
# define PR_TSC_ENABLE		1	/* allow the use of the timestamp counter */
# define PR_TSC_SIGSEGV		2	/* throw a SIGSEGV instead of reading the TSC */
/* Get/set securebits (as per security/commoncap.c) */
#define PR_GET_SECUREBITS 27
#define PR_SET_SECUREBITS 28
/*
 * Get/set the timerslack as used by poll/select/nanosleep
 * A value of 0 means "use default"
 */
#define PR_SET_TIMERSLACK 29
#define PR_GET_TIMERSLACK 30
#define PR_TASK_PERF_EVENTS_DISABLE		31
#define PR_TASK_PERF_EVENTS_ENABLE		32
/*
 * Set early/late kill mode for hwpoison memory corruption.
 * This influences when the process gets killed on a memory corruption.
 */
#define PR_MCE_KILL	33
# define PR_MCE_KILL_CLEAR   0
# define PR_MCE_KILL_SET     1
# define PR_MCE_KILL_LATE    0
# define PR_MCE_KILL_EARLY   1
# define PR_MCE_KILL_DEFAULT 2
#define PR_MCE_KILL_GET 34
/*
 * Tune up process memory map specifics.
 */
#define PR_SET_MM		35
# define PR_SET_MM_START_CODE		1
# define PR_SET_MM_END_CODE		2
# define PR_SET_MM_START_DATA		3
# define PR_SET_MM_END_DATA		4
# define PR_SET_MM_START_STACK		5
# define PR_SET_MM_START_BRK		6
# define PR_SET_MM_BRK			7
# define PR_SET_MM_ARG_START		8
# define PR_SET_MM_ARG_END		9
# define PR_SET_MM_ENV_START		10
# define PR_SET_MM_ENV_END		11
# define PR_SET_MM_AUXV			12
# define PR_SET_MM_EXE_FILE		13
/*
 * Set specific pid that is allowed to ptrace the current task.
 * A value of 0 mean "no process".
 */
#define PR_SET_PTRACER 0x59616d61
# define PR_SET_PTRACER_ANY ((unsigned long)-1)
#define PR_SET_CHILD_SUBREAPER	36
#define PR_GET_CHILD_SUBREAPER	37
/*
 * If no_new_privs is set, then operations that grant new privileges (i.e.
 * execve) will either fail or not grant them.  This affects suid/sgid,
 * file capabilities, and LSMs.
 *
 * Operations that merely manipulate or drop existing privileges (setresuid,
 * capset, etc.) will still work.  Drop those privileges if you want them gone.
 *
 * Changing LSM security domain is considered a new privilege.  So, for example,
 * asking selinux for a specific new context (e.g. with runcon) will result
 * in execve returning -EPERM.
 */
#define PR_SET_NO_NEW_PRIVS	38
#define PR_GET_NO_NEW_PRIVS	39
#define PR_GET_TID_ADDRESS	40

// dummy function, normally used to set thread name
int prctl(int op, char *buf, int a1, int a2, int a3) { return 0;};
#endif /* _LINUX_PRCTL_H */