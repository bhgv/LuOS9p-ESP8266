#ifndef _TEK_EXEC_H
#define	_TEK_EXEC_H

/*
**	teklib/tek/exec.h - Public Exec types, structures, macros, constants
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/type.h>

/*****************************************************************************/
/*
**	Forward declarations of internal Exec structures
*/

/* Exec module base structure: */
struct TExecBase;
/* Exec task structure: */
struct TTask;
/* Exec message port: */
struct TMsgPort;
/* Exec memory manager: */
struct TMemManager;
/* Exec memory pool: */
struct TPool;
/* Exec lock: */
struct TLock;
/* Exec atom: */
struct TAtom;

/*****************************************************************************/
/*
**	Nodes and lists
**
**	Lists are doubly-linked with a header acting as a "Null" node.
**	A typical iterator for that topology may look like this:
**
**	struct TNode *next, *node = list->tlh_Head.tln_Succ;
**	for (; (next = node->tln_Succ); node = next)
**	{
**		you can operate on 'node' here, remove it safely,
**		as well as 'break' out from the loop and 'continue' it
**	}
*/

struct TNode
{
	/* Ptr to successor in the list */
	struct TNode *tln_Succ;
	/* Ptr to predecessor in the list */
	struct TNode *tln_Pred;
};

struct TList
{
	/* Head node of list */
	struct TNode tlh_Head;
	/* Tail node of list */
	struct TNode tlh_Tail;
};

/*****************************************************************************/
/*
**	Hooks
*/

struct THook;

typedef TTAG (*THOOKFUNC)(struct THook *, TAPTR obj, TTAG msg);

struct THook
{
	/* Entrypoint following uniform, per-platform calling conventions: */
 	THOOKENTRY THOOKFUNC thk_Entry;
	/* Optional language/compiler specific entrypoint: */
	THOOKFUNC thk_SubEntry;
	/* User data: */
	TAPTR thk_Data;
};

/* Pre-defined hook messages: */

/* Destroy object: */
#define TMSG_DESTROY		0
/* Iterate taglist: */
#define TMSG_FOREACHTAG		1
/* Compare two TTAG values pointed to by object: */
#define TMSG_COMPAREKEYS	2
/* Calculate a 32bit hash: */
#define TMSG_CALCHASH32		3
/* Open module: */
#define TMSG_OPENMODULE		4
/* Close module: */
#define	TMSG_CLOSEMODULE	5
/* Query module interface: */
#define TMSG_QUERYIFACE		6
/* Drop module interface: */
#define TMSG_DROPIFACE		7
/* Initialize user task: */
#define TMSG_INITTASK		8
/* Call task procedure: */
#define TMSG_RUNTASK		9
/* Get next entry: */
#define TMSG_GETNEXTENTRY	10
/* Offset to user-defined hook messages: */
#define TMSG_USER			0x1000

/*****************************************************************************/
/*
**	Generic named handle with owner and callback hook
**
**	Objects of this type can be linked into lists, searched for by name,
**	and cleaned up by invoking the NULL message on them. The link library
**	provides the functions TDestroy(), TDestroyList(), and TFindHandle().
*/

struct THandle
{
	/* Node header */
	struct TNode thn_Node;
	/* Object callback hook */
	struct THook thn_Hook;
	/* Module base / owner of this object */
	TAPTR thn_Owner;
	/* Object name */
	TSTRPTR thn_Name;
	/* Object type: */
	TUINT16 thn_Type;
	/* Object flags: */
	TUINT16 thn_Flags;
};

#define TGetOwner(obj)		(((struct THandle *) (obj))->thn_Owner)
#define TGetModuleBase(obj)	TGetOwner(obj)
#define TGetExecBase(obj)	TGetModuleBase(obj)

/*****************************************************************************/
/*
**	Module header
*/

struct TModule
{
	/* Module handle */
	struct THandle tmd_Handle;
	/* Ptr to HAL module handle */
	TAPTR tmd_HALMod;
	/* Task that opened this instance */
	struct TTask *tmd_InitTask;
	/* (Back-) ptr to module super instance */
	struct TModule *tmd_ModSuper;
	/* Modbase negative size [bytes] */
	TSIZE tmd_NegSize;
	/* Modbase positive size [bytes] */
	TSIZE tmd_PosSize;
	/* Flags */
	TUINT tmd_Flags;
	/* Number of instances open */
	TUINT tmd_RefCount;
	/* Major (checked against) */
	TUINT16 tmd_Version;
	/* Minor (informational) */
	TUINT16 tmd_Revision;
};

/* Reserved module vectors */
#define TMODV_NUMRESERVED		8
#define TMODV_BEGINIO			-3
#define TMODV_ABORTIO			-4

/*****************************************************************************/
/*
**	Interface
*/

struct TInterface
{
	/* Interface handle: */
	struct THandle tif_Handle;
	/* Module this interface is bound to: */
	struct TModule *tif_Module;
	/* Reserved for future extensions, initialize to TNULL: */
	TAPTR tif_Reserved;
	/* Interface version: */
	TUINT16 tif_Version;
	/* Padding, leave untouched: */
	TUINT16 tif_Reserved2;
};

struct TInterfaceQuery
{
	/* Interface name: */
	TSTRPTR tifq_Name;
	/* Interface query tags, currently TNULL: */
	TTAGITEM *tifq_Tags;
	/* Interface version: */
	TUINT16 tifq_Version;
};

/*****************************************************************************/
/*
**	Time and Date
*/

typedef TDATE_T TTIME;
typedef TDATE_T TDATE;

/*****************************************************************************/
/*
**	Task signals
**
**	Currently there are 28 free user signals, but the number of signals with
**	predefined meaning (like TTASK_SIG_ABORT or those reserved for the
**	inbuilt message ports) may grow in the future. TEKlib guarantees that the
**	upper 24 signal bits (0x80000000 through 0x00000100) will remain available
**	to the user. More free user signals (if available) can be obtained safely
**	with TExecAllocSignal(). Allocation is recommended anyway.
*/

/* Reserved meaning: ABORT */
#define TTASK_SIG_ABORT		0x00000001
/* Reserved meaning: BREAK */
#define TTASK_SIG_BREAK		0x00000002
/* Reserved for task's syncport */
#define TTASK_SIG_SINGLE	0x00000004
/* Reserved for task's userport */
#define TTASK_SIG_USER		0x00000008

/*
**	Task entry function
*/

typedef TTASKENTRY void (*TTASKFUNC)(struct TTask *task);

/*
**	Task tags
*/

#define TEXECTAGS_			(TTAG_USER + 0x300)

/* Ptr to user/init data */
#define TTask_UserData		(TEXECTAGS_ + 0)
/* Task name */
#define TTask_Name			(TEXECTAGS_ + 1)
/* Parent memory manager */
#define TTask_MemManager	(TEXECTAGS_ + 2)
/* Task's heap memmanager */
#define TTask_HeapMemManager (TEXECTAGS_ + 3)
/* Message memory manager */
#define TTask_MsgMemManager	(TEXECTAGS_ + 4)
/* Lock to a currentdir */
#define TTask_CurrentDir	(TEXECTAGS_ + 5)
/* Input file handle */
#define TTask_InputFH		(TEXECTAGS_ + 6)
/* Output file handle */
#define TTask_OutputFH		(TEXECTAGS_ + 7)
/* Error file handle */
#define TTask_ErrorFH		(TEXECTAGS_ + 8)
/* Msgport hook */ 
#define TMsgPort_Hook		(TEXECTAGS_ + 9)
/* Ptr to user/init data */
#define TTask_InitData		(TEXECTAGS_ + 10)

/*****************************************************************************/
/*
**	Application, Execbase and HAL entry tags
*/

/* Exec module version */
#define TExecBase_Version	(TEXECTAGS_ + 16)
/* Argument count */
#define TExecBase_ArgC		(TEXECTAGS_ + 17)
/* Argument array */
#define TExecBase_ArgV		(TEXECTAGS_ + 18)
/* Startup modules */
#define TExecBase_ModInit	(TEXECTAGS_ + 20)
/* Ptr to return value */
#define TExecBase_RetValP	(TEXECTAGS_ + 21)
/* System global path */
#define TExecBase_SysDir	(TEXECTAGS_ + 22)
/* Global path to modules */
#define TExecBase_ModDir	(TEXECTAGS_ + 23)
/* Application progdir */
#define TExecBase_ProgDir	(TEXECTAGS_ + 24)
/* Pass HAL base to Exec */
#define TExecBase_HAL		(TEXECTAGS_ + 25)
/* Pass Exec base to HAL */
#define THalBase_Exec		(TEXECTAGS_ + 26)
/* Single string of args */
#define TExecBase_Arguments	(TEXECTAGS_ + 27)
/* Progname, like argv[0] */
#define TExecBase_ProgName	(TEXECTAGS_ + 28)
/* System memory base */
#define TExecBase_MemBase	(TEXECTAGS_ + 29)
/* System memory size */
#define TExecBase_MemSize	(TEXECTAGS_ + 30)
/* Boot handle passed to HAL */
#define TExecBase_BootHnd	(TEXECTAGS_ + 31)

/*****************************************************************************/
/*
**	System names
*/

/* Name of task running Exec */
#define TTASKNAME_EXEC		"exec.task"
/* Name of initial entry task */
#define TTASKNAME_ENTRY		"entry.task"
/* Name of module loader task */
#define TTASKNAME_RAMLIB	"ramlib.task"
/* Name of HAL device task */
#define TTASKNAME_HALDEV	"hal.task"
/* Name of the Exec module */
#define TMODNAME_EXEC		"exec"
/* Name of the HAL module */
#define TMODNAME_HAL		"hal"
/* Alias for the HAL device */
#define TMODNAME_TIMER		"hal"

/*****************************************************************************/
/*
**	Initial startup module
**
**	An array of this structure can be passed to a TEKlib framework using
**	the TExecBase_ModInit tag. This allows to statically link modules to
**	an application. The array is terminated with an entry whose tinm_Name
**	is TNULL.
*/

typedef TMODCALL TUINT (*TMODINITFUNC) (struct TTask *task,
	struct TModule *mod, TUINT16 version, struct TTagItem *tags);

struct TInitModule
{
	/* Module name */
	TSTRPTR tinm_Name;
	/* Mod init function */
	TMODINITFUNC tinm_InitFunc;
	/* For future extensions, must be TNULL */
	TAPTR tinm_Reserved;
	/* Reserved, must be 0 */
	TUINT tinm_Flags;
};

/*
**	Node of initmodules to be passed to Exec via TAddModules():
*/

struct TModInitNode
{
	struct TNode tmin_Node;
	/* Ptr to an array of InitModules: */
	const struct TInitModule *tmin_Modules;
	/* Reserved for future extensions, must be TNULL: */
	TAPTR tmin_Extended;
};

/*****************************************************************************/
/*
**	Atom mode flags
*/

/* Do not create a new or destroy an atom */
#define TATOMF_KEEP		0x00
/* Create a new atom, if it does not exist */
#define TATOMF_CREATE	0x01
/* Destroy an existing atom */
#define TATOMF_DESTROY	0x02
/* Lock shared */
#define TATOMF_SHARED	0x04
/* Name is passed instead of an atom */
#define TATOMF_NAME		0x08
/* Only attempt locking (or creation) */
#define TATOMF_TRY		0x10

/*****************************************************************************/
/*
**	Memory manager types
*/

/* Null MemManager incapable of allocating */
#define TMMT_Void		0x00000000
/* Put MemManager on top of a parent MemManager */
#define TMMT_MemManager	0x00000001
/* Put MemManager on top of a static memblock */
#define TMMT_Static		0x00000002
/* Put MemManager on top of a pooled allocator */
#define TMMT_Pooled		0x00000004
/* Leak-tracking on top of a parent MemManager */
#define TMMT_Tracking	0x00000008
/* Thread-safety on top of a parent MemManager */
#define TMMT_TaskSafe	0x00000100
/* Msg allocator on parent msg MemManager */
#define TMMT_Message	0x00000200
/* Bounds checking on top of parent MemManager */
#define TMMT_Debug		0x00000400

/*
**	Tags for memory allocators
*/

/* Low-fragment strategy */
#define	TMem_LowFrag		(TEXECTAGS_ + 64)
/* Static mmu: size of memblock */
#define TMem_StaticSize		(TEXECTAGS_ + 65)
/* Parent memory manager */
#define	TPool_MemManager	(TEXECTAGS_ + 66)
/* Size of puddles */
#define	TPool_PudSize		(TEXECTAGS_ + 67)
/* Thressize for large puddles */
#define	TPool_ThresSize		(TEXECTAGS_ + 68)
/* Auto-adaptive pud/thressize */
#define	TPool_AutoAdapt		(TEXECTAGS_ + 69)
#define TPool_Static		(TEXECTAGS_ + 70)
#define TPool_StaticSize	(TEXECTAGS_ + 71)

/*
**	Tags for module scanning
*/

#define TExec_ModuleName	(TEXECTAGS_ + 80)
#define TExec_ModulePrefix	(TEXECTAGS_ + 81)

/*****************************************************************************/
/*
**	Message status, as returned by TExecSendMsg().
**	Flags are used by Exec internally, applications use the status defines.
*/

/* Flag: Message has been sent */
#define TMSGF_SENT			0x0001
/* Flag: Message was returned */
#define TMSGF_RETURNED		0x0002
/* Flag: Message body may be modified */
#define TMSGF_MODIFIED		0x0004
/* Flag: Message is queued in a port */
#define TMSGF_QUEUED		0x0008

/* Status: Message was dropped */
#define TMSG_STATUS_FAILED	0x0000
/* Status: Message was sent */
#define TMSG_STATUS_SENT	0x0001
/* Status: Message TExecAckMsg()'ed */
#define TMSG_STATUS_ACKD	0x0003
/* Status: Message TExecReplyMsg()'ed */
#define TMSG_STATUS_REPLIED	0x0007

/*****************************************************************************/
/*
**	Standard device I/O return codes
*/

#define TIOERR_SUCCESS				0
#define TIOERR_UNKNOWN_COMMAND		(-1)
#define TIOERR_ABORTED				(-2)
#define TIOERR_DEVICE_OPEN_FAILED	(-3)

/*****************************************************************************/
/*
**	Module flags
*/

#define TMODF_NONE			0x0000
/* Module is ready - internal use only */
#define TMODF_INITIALIZED	0x0001
/* module has vector table */
#define TMODF_VECTORTABLE	0x0002
/* Module supports open and close */
#define TMODF_OPENCLOSE		0x0004
/* Module supports interface queries */
#define TMODF_QUERYIFACE	0x0008

/*****************************************************************************/
/*
**	Execbase command codes:
*/

#define TEXEC_CMD_RUN	0
#define TEXEC_CMD_INIT	1
#define TEXEC_CMD_EXIT	2

/*****************************************************************************/
/*
**	Macro versions of some library functions
*/

/* Call hook packet */
#define TCALLHOOKPKT(hook, obj, msg) ( \
	(*(hook)->thk_Entry) ? \
	(*(hook)->thk_Entry)(hook, obj, msg) : 0)

/* Destroy, i.e. invoke handle callback with NULL message */
#define TDESTROY(h) ( \
	(h) ? TCALLHOOKPKT(&((struct THandle *)(h))->thn_Hook, \
	h, TNULL) : 0)

/* Setup a list header */
#define TINITLIST(l) ( \
	(l)->tlh_Head.tln_Succ = &(l)->tlh_Tail, \
	(l)->tlh_Head.tln_Pred = TNULL, \
	(l)->tlh_Tail.tln_Succ = TNULL, \
	(l)->tlh_Tail.tln_Pred = &(l)->tlh_Head)

/* Get first node of a list */
#define	TFIRSTNODE(list) ( \
	(list)->tlh_Head.tln_Succ->tln_Succ ? \
	(list)->tlh_Head.tln_Succ : TNULL)

/* Get last node of a list */
#define TLASTNODE(list) ( \
	(list)->tlh_Tail.tln_Pred->tln_Pred ? \
	(list)->tlh_Tail.tln_Pred : TNULL)

/* Test if a list is empty */
#define TISLISTEMPTY(list) ( \
	(list)->tlh_Tail.tln_Pred == &(list)->tlh_Head)

/* Remove node from whatever list it is linked into */
#define TREMOVE(n) ( \
	(n)->tln_Pred->tln_Succ = (n)->tln_Succ, \
	(n)->tln_Succ->tln_Pred = (n)->tln_Pred)

/* These require a temp node: */

/* Unlink node from head of a list */
#define TREMHEAD(l,t) ( \
	((t) = (l)->tlh_Head.tln_Succ)->tln_Succ ? \
	(l)->tlh_Head.tln_Succ = (t)->tln_Succ, \
	(t)->tln_Succ->tln_Pred = &(l)->tlh_Head, \
	(t) : TNULL)

/* Unlink node from tail of a list */
#define TREMTAIL(l,t) ( \
	((t) = (l)->tlh_Tail.tln_Pred)->tln_Pred ? \
	(l)->tlh_Tail.tln_Pred = (t)->tln_Pred, \
	(t)->tln_Pred->tln_Succ = &(l)->tlh_Tail, \
	(t) : TNULL)

/* Add node to head of a list */
#define TADDHEAD(l,n,t) ( \
	(t) = (l)->tlh_Head.tln_Succ, \
	(l)->tlh_Head.tln_Succ = (n), \
	(n)->tln_Succ = (t), \
	(n)->tln_Pred = &(l)->tlh_Head, \
	(t)->tln_Pred = (n))

/* Add node to tail of a list */
#define TADDTAIL(l,n,t) ( \
	(t) = (l)->tlh_Tail.tln_Pred, \
	(l)->tlh_Tail.tln_Pred = (n), \
	(n)->tln_Succ = &(l)->tlh_Tail, \
	(n)->tln_Pred = (t), \
	(t)->tln_Succ = (n))

#endif
