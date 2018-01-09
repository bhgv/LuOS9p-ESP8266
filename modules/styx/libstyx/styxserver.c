#include <lib9.h>
//#include <styx.h>
#include <fcall.h>
#include "styxserver.h"
#include "styxaux.h"

#define MAXSTAT	256 //512
//#define EMSGLEN			256		/* %r */

#define TABSZ	32	/* power of 2 */

static	unsigned long		boottime;
static	char*	eve = "inferno";
static	int		Debug = 0;

char Enomem[] =		"out of memory";
char Eperm[] =			"permission denied";
char Enodev[] =		"no free devices";
char Ehungup[] =		"write to hungup channel";
char	Eexist[] =			"file exists";
char Enonexist[] =		"file does not exist";
char Ebadcmd[] =		"bad command";
char Ebadarg[] =			"bad arg in system call";
char Enofid[] =			"no such fid";
char Enotdir[] =		"not a directory";
char	Eopen[] =			"already open";
char	Ebadfid[] =		"bad fid";

/* client state */
enum{
	CDISC = 01,
	CNREAD = 02,
	CRECV = 04,
};

typedef struct Walkqid Walkqid;

struct Fid
{
	Client 	*client;
	Fid *next;
	short	fid;
	ushort	open;
	ushort	mode;	/* read/write */
	ulong	offset;	/* in file */
	int		dri;		/* dirread index */
	Qid		qid;
};

struct Walkqid
{
	Fid	*clone;
	int	nqid;
	Qid	qid[1];
};

#define ASSERT(A,B) styxassert((int)A,B)

static int hash(Path);
static void deletefids(Client *);

static void
styxfatal(char *fmt, ...)
{
//	char *buf, *out;
	va_list arg;
	//buf = styxmalloc(1024);
//	out = seprint(buf, buf+sizeof(buf), "Fatal error: ");
	printf("\nFatal error: ");
	va_start(arg, fmt);
//	out = vseprint(out, buf+sizeof(buf), fmt, arg);
	printf(fmt, arg);
	va_end(arg);
	//write(2, buf, out-buf);
	printf("\n");
//	styxfree(buf);
	styxexit(1);
}

static void
styxassert(int vtrue, char *reason)
{
	if(!vtrue)
		styxfatal("assertion failed: %s\n", reason);
}

void *
styxmalloc(int bytes)
{
	char *m = malloc(bytes);
	if(m == nil)
		styxfatal(Enomem);
	memset(m, 0, bytes);
	return m;
}

void
styxfree(void *p)
{
	free(p);
}

void
styxdebug()
{
	Debug = 1;
}

static Client *
newclient(Styxserver *server, int fd)
{
	Client *c = (Client *)styxmalloc(sizeof(Client));

	if(Debug)
		printf("New client at %lux\n", (ulong)c);
//		fprint(2, "New client at %lux\n", (ulong)c);
	c->server = server;
	c->fd = fd;
	c->nread = 0;
	c->nc = 0;
	c->state = 0;
	c->fids = nil;
	c->uname = strdup(eve);
	c->aname = strdup("");
	c->next = server->clients;
	c->ops = NULL;
	server->clients = c;
	if(server->ops->newclient)
		server->ops->newclient(c);
	return c;
}

static void
freeclient(Client *c)
{
	Client **p;
	Styxserver *server;

	if(Debug)
		printf("Freeing client at %lux\n", (ulong)c);
//		fprint(2, "Freeing client at %lux\n", (ulong)c);
	server = c->server;
	if(server->ops->freeclient)
		server->ops->freeclient(c);
	for(p = &( server->clients ); ((uint)*p & 0xfff00000) >= 0x3ff00000; p = &( (*p)->next) ){
		if(*p == c){
			styxclosesocket(c->fd);
			*p = c->next;
			deletefids(c);
			free(c->uname);
			free(c->aname);
			styxfree(c);
			return;
		}
	}
}

static int
nbread(Client *c, int nr)
{
	int nb;

	if(c->state&CDISC)
		return -1;
	nb = styxrecv(c->server, c->fd, c->msg + c->nread, nr, 0);
printf("%s: %d nb=%d\n", __func__, __LINE__, nb);
	if(nb <= 0){
		c->nread = 0;
		c->state |= CDISC;
		return -1;
	}
	c->nread += nb;
	return 0;
}

static int
rd(Client *c, Fcall *r)
{
	if(c->nc > 0){	/* last convM2S consumed nc bytes */
		c->nread -= c->nc;
		if((int)c->nread < 0){
			r->ename = "negative size in rd";
			return -1;
		}
		memmove(c->msg, c->msg+c->nc, c->nread);
		c->nc = 0;
	}
	if(c->state & CRECV){
		if(nbread(c, MSGMAX - c->nread) != 0){
			r->ename = "unexpected EOF";
			return -1;
		}
		c->state &= ~CRECV;
	}
	c->nc = convM2S((uchar*)(c->msg), c->nread, r);
	if(c->nc < 0){
		r->ename = "bad message format";
		return -1;
	}
	if(c->nc == 0 && c->nread > 0){
		c->nread = 0;
		c->state &= ~CNREAD;
		return 0;
	}
	if(c->nread > c->nc)
		c->state |= CNREAD;
	else
		c->state &= ~CNREAD;
	if(c->nc == 0)
		return 0;
	/* fprint(2, "rd: %F\n", r); */

	if(r->count > MSGMAX)
		r->count = MSGMAX;
	
	return 1;
}

static int
wr(Client *c, Fcall *r)
{
	int n;
	int l = sizeS2M(r);
	char* buf = malloc( l ); //MSGMAX); //[MSGMAX];
	int ret;

printf("%s: %d sz=%d, MAX=%d\n", __func__, __LINE__,  l, MSGMAX);

	n = convS2M(r, (uchar*)buf, l); //sizeof(buf));
	if(n < 0){
		r->ename = "bad message type in wr";
		return -1;
	}
	/* fprint(2, "wr: %F\n", r); */
	ret = styxsend(c->server, c->fd, buf, n, 0);
printf("%s: %d\n", __func__, __LINE__ );

	free(buf);
	
	return ret;
}

static void
sremove(Styxserver *server, Styxfile *f)
{
	Styxfile *s, *next, **p;

	if(f == nil)
		return;
	if(Debug)
		printf("Remove file %s Qid=%llx\n", f->d.name, f->d.qid.path);
//		fprint(2, "Remove file %s Qid=%llx\n", f->d.name, f->d.qid.path);
	if(f->d.qid.type&QTDIR)
		for(s = f->child; s != nil; s = next){
			next = s->sibling;
			sremove(server, s);
	}
	for(p = &server->ftab[hash(f->d.qid.path)]; *p; p = &(*p)->next)
		if(*p == f){
			*p = f->next;
			break;
		}
	for(p = &f->parent->child; *p; p = &(*p)->sibling)
		if(*p == f){
			*p = f->sibling;
			break;
		}
	
	if(f->d.qid.my_name)
		styxfree(f->d.qid.my_name);
	styxfree(f->d.name);
	styxfree(f->d.uid);
	styxfree(f->d.gid);
	styxfree(f);
}

int
styxrmfile(Styxserver *server, Path qid)
{
	Styxfile *f;

	f = styxfindfile(server, qid);
	if(f != nil){
		if(f->parent == nil)
			return -1;
		sremove(server, f);
		return 0;
	}
	return -1;
}

static void
incref(Styxfile *f)
{
	if(f != nil)
		f->ref++;
}

static void
decref(Styxfile *f)
{
	if(f != nil)
		--f->ref;
}

static void
increff(Fid *f)
{
	incref(styxfindfile(f->client->server, f->qid.path));
}

static void
decreff(Fid *f)
{
	decref(styxfindfile(f->client->server, f->qid.path));
}

static void
incopen(Fid *f)
{
	Styxfile *file;

	if(f->open && (file = styxfindfile(f->client->server, f->qid.path)) != nil)
		file->open++;
}

static void
decopen(Fid *f)
{
	Styxfile *file;

	if(f->open && (file = styxfindfile(f->client->server, f->qid.path)) != nil)
		file->open--;
}

int
styxperm(Styxfile *f, char *uid, int mode)
{
	int m, p;

	p = 0;
	switch(mode&3){
	case OREAD:	p = AREAD;	break;
	case OWRITE:	p = AWRITE;	break;
	case ORDWR:	p = AREAD+AWRITE;	break;
	case OEXEC:	p = AEXEC;	break;
	}
	if(mode&OTRUNC)
		p |= AWRITE;
	m = f->d.mode&7;
	if((p&m) == p)
		return 1;
	if(strcmp(f->d.uid, uid) == 0){
		m |= (f->d.mode>>6)&7;
		if((p&m) == p)
			return 1;
	}
	if(strcmp(f->d.gid, uid) == 0){
		m |= (f->d.mode>>3)&7;
		if((p&m) == p)
			return 1;
	}
	return 0;
}

static int
hash(Path path)
{
	return path & (TABSZ-1);
}

Styxfile *
styxfindfile(Styxserver *server, Path path)
{
	Styxfile *f;

	for(f = server->ftab[hash(path)]; f != nil; f = f->next){
		if(f->d.qid.path == path)
			return f;
	}
	return nil;
}

static Fid *
findfid(Client *c, short fid)
{
	Fid *f;
	for(f = c->fids; f && f->fid != fid; f = f->next)
		;
	return f;
}

static void
deletefid(Client *c, Fid *d)
{
	/* TODO: end any outstanding reads on this fid */
	Fid **f;

	for(f = &c->fids; *f; f = &(*f)->next)
		if(*f == d){
			decreff(d);
			decopen(d);
			*f = d->next;
			//if(d->qid.my_name)
			//	styxfree(d->qid.my_name);
			styxfree(d);
			return;
		}
}

static void
deletefids(Client *c)
{
	Fid *f, *g;

	for(f = c->fids; f; f = g){
		decreff(f);
		decopen(f);
		g = f->next;
		if(f->qid.my_name)
			styxfree(f->qid.my_name);
		styxfree(f);
	}
}

Fid *
newfid(Client *c, short fid, Qid qid){
	Fid *f;

	f = styxmalloc(sizeof(Fid));
	ASSERT(f, "newfid");
	f->client = c;
	f->fid = fid;
	f->open = 0;
	f->dri = 0;
	f->qid = qid;
	f->next = c->fids;
	c->fids = f;
	increff(f);
	return f;
}

static void
flushtag(int oldtag)
{
	USED(oldtag);
}

int
eqqid(Qid a, Qid b)
{
	return a.path == b.path && a.vers == b.vers;
}

static Fid *
fidclone(Fid *old, short fid)
{
	Fid *new;

	new = newfid(old->client, fid, old->qid);
	return new;
}

static Walkqid*
devwalk(Client *c, Styxfile *file, Fid *fp, Fid *nfp, char **name, int nname, char **err)
{
	Styxserver *server;
	long j;
	Walkqid *wq;
	char *n;
	Styxfile *p, *f;
	Styxops *ops;
	Qid qid;

	*err = nil;
	server = c->server;
	ops = server->ops;

	wq = styxmalloc(sizeof(Walkqid)+(nname-1)*sizeof(Qid));
	wq->nqid = 0;

printf("%s: %d\n", __func__, __LINE__);
	p = file;
	qid = (p != nil) ? p->d.qid : fp->qid;
	for(j = 0; j < nname; j++){
		if(!(qid.type & QTDIR)){
printf("%s: %d\n", __func__, __LINE__);
			if(j == 0){
printf("%s: %d\n", __func__, __LINE__);
				styxfatal("devwalk error");
			}
			*err = Enotdir;
			goto Done;
		}
		if(p != nil && !styxperm(p, c->uname, OEXEC)){
printf("%s: %d\n", __func__, __LINE__);
			*err = Eperm;
			goto Done;
		}
		n = name[j];
printf("%s: %d. nm=%s\n", __func__, __LINE__, n);
		if(strcmp(n, ".") == 0){
    Accept:
			wq->qid[wq->nqid++] = nfp->qid;
			continue;
		}
		if(p != nil && strcmp(n, "..") == 0 && p->parent){
			decref(p);
			nfp->qid.path = p->parent->d.qid.path;
			nfp->qid.type = p->parent->d.qid.type;
			nfp->qid.vers = 0;
			incref(p->parent);
			p = p->parent;
			qid = p->d.qid;
			goto Accept;
		}
		
		if(ops->walk != nil){
			char *e;

			e = ops->walk(&qid, n);
			if(e == nil){
				decreff(nfp);
				nfp->qid = qid;
				increff(nfp);
printf("%s: %d\n", __func__, __LINE__);
				p = styxfindfile(server, qid.path);
				if(server->needfile && p == nil)
					goto Done;
				qid = (p != nil) ? p->d.qid : nfp->qid;
				goto Accept;
			}
		}

		if(p != nil){
			/*
			if(p->d.qid.my_type == 1){
				int i = scan_devs(n);
				if(i > 0){
					nfp->qid.path = (1<<16) + i;
					nfp->qid.type = 0;
					
					nfp->qid.my_type = 2;
					
					nfp->qid.vers = 0;
					
					goto Accept;
				}
			}else
			*/
			for(f = p->child; f != nil; f = f->sibling){
printf("%s: %d. name = %s\n", __func__, __LINE__, f->d.name);
				if(strcmp(n, f->d.name) == 0){
printf("%s: %d. res nm = %s\n", __func__, __LINE__, n);
					decref(p);
					nfp->qid.path = f->d.qid.path;
					nfp->qid.type = f->d.qid.type;

					nfp->qid.my_type = f->d.qid.my_type;
					
					nfp->qid.vers = 0;
					incref(f);
					p = f;
					qid = p->d.qid;
					goto Accept;
				}
			}
		}
		if(j == 0 && *err == nil)
			*err = Enonexist;
		goto Done;
	}
Done:
	if(*err != nil){
//		if(wq.clone->qid.my_name){
//			styxfree(wq.clone->qid.my_name);
//			wq.clone->qid.my_name = NULL;
//		}
//		for(int i = 0; i < wq.nqid; i++){
//			styxfree(wq.qid[i].my_name);
//			wq.qid[i].my_name = NULL;
//		}
		styxfree(wq);
		return nil;
	}
	return wq;
}

static long
devdirread(Fid *fp, Styxfile *file, char *d, long n)
{
	long dsz, m;
	Styxfile *f = NULL;
	int i;

	struct{
		Dir d;
//		char slop[100];	/* TO DO */
	}dir;

//printf("%s: %d n = %d\n", __func__, __LINE__, n);
	f = file->child;
	for(i = 0; i < fp->dri; i++){
		if(f == 0)
			return 0;
		else
			f = f->sibling;
	}
	
	for(m = 0; m < n; fp->dri++){
		if(f == nil)
			break;
		dir.d = f->d;
		dsz = convD2M(&dir.d, (uchar*)d, n-m);
		if(dsz <= BIT16SZ)
			break;
		m += dsz;
		d += dsz;
		f = f->sibling;
//printf("%s: %d m = %d < n = %d, dsz = %d, f = %x, fp->dri=%d\n", __func__, __LINE__, 
//						m, n, dsz, f, fp->dri);
	}
	
//printf("%s: %d out cnt = %d\n", __func__, __LINE__, m);

	return m;
}

static char*
nilconv(char *s)
{
	if(s != nil && s[0] == '\0')
		return nil;
	return s;
}

static Styxfile *
newfile(Styxserver *server, Styxfile *parent, int isdir, Path qid, char *name, int mode, char *owner)
{
	Styxfile *file;
	Dir d;
	int h;

	if(qid == -1)
		qid = server->qidgen++;
	file = styxfindfile(server, qid);
	if(file != nil)
		return nil;
	if(parent != nil){
		for(file = parent->child; file != nil; file = file->sibling)
			if(strcmp(name, file->d.name) == 0)
				return nil;
	}
	file = (Styxfile *)styxmalloc(sizeof(Styxfile));
	file->type = 0;
	file->par.i = 0;
	file->parent = parent;
	file->child = nil;
	h = hash(qid);
	file->next = server->ftab[h];
	server->ftab[h] = file;
	if(parent){
		file->sibling = parent->child;
		parent->child = file;
	}else
		file->sibling = nil;

	d.type = 'X';
	d.dev = 'x';
	d.qid.path = qid;
	d.qid.type = 0;
	d.qid.vers = 0;
	
	d.qid.my_type = 0;
	d.qid.my_name = NULL;

	d.mode = mode;
	d.atime = styxtime(0);
	d.mtime = boottime;
	d.length = 0;
	d.name = strdup(name);
	d.uid = strdup(owner);
	d.gid = strdup(eve);
	d.muid = "";

	if(isdir){
		d.qid.type |= QTDIR;
		d.mode |= DMDIR;
	}
	else{
		d.qid.type &= ~QTDIR;
		d.mode &= ~DMDIR;
	}

	file->d = d;
	file->ref = 0;
	file->open = 0;
	if(Debug)
		printf("New file %s Qid=%llx\n", name, qid);
//		fprint(2, "New file %s Qid=%llx\n", name, qid);
	return file;
}

static void
run(Client *c)
{
	Fcall* f = malloc(sizeof(Fcall));
	Fid *fp, *nfp;
	int i, open, mode;
	//char ebuf[EMSGLEN];
	Walkqid *wq;
	Styxfile *file;
	Dir dir;
	Qid qid;
	Styxops *ops;
	char* strs = NULL; //[128];

	//ebuf[0] = 0;
	if(rd(c, f) <= 0){
		free(f);
		return;
	}
	if(f->type == Tflush){
		flushtag(f->oldtag);
		f->type = Rflush;
		wr(c, f);
		free(f);
		return;
	}

	if(c->ops == NULL)
		ops = c->server->ops;
	else
		ops = c->ops;
	
	file = nil;
	fp = findfid(c, f->fid);
	if(f->type != Tversion && f->type != Tauth && f->type != Tattach){
		if(fp == nil){
			f->type = Rerror;
			f->ename = Enofid;
			wr(c, f);
			free(f); 
			return;
		}
		else{
			file = styxfindfile(c->server, fp->qid.path);
			if(c->server->needfile && file == nil){
				f->type = Rerror;
				f->ename = Enonexist;
				wr(c, f);
				free(f); 
				return;
			}
		}
	}
	/* if(fp == nil) fprint(2, "fid not found for %d\n", f->fid); */
	switch(f->type){
	case	Twalk:
		if(Debug){
			printf("\nTwalk %d %d", f->fid, f->newfid);
//			fprint(2, "Twalk %d %d", f->fid, f->newfid);
			for(i = 0; i < f->nwname; i++)
				printf(" %s", f->wname[i]);
//				fprint(2, " %s", f->wname[i]);
			printf("\n");
//			fprint(2, "\n");
		}
		nfp = findfid(c, f->newfid);
		f->type = Rerror;
		if(nfp){
			deletefid(c, nfp);
			nfp = nil;
		}
printf("%s: %d\n", __func__, __LINE__);
		if(nfp){
			f->ename = "fid in use";
			if(Debug) printf("walk: %s\n", f->ename);
//			if(Debug) fprint(2, "walk: %s\n", f->ename);
printf("%s: %d\n", __func__, __LINE__);
			wr(c, f);
			break;
		}else if(fp->open){
			f->ename = "can't clone";
printf("%s: %d\n", __func__, __LINE__);
			wr(c, f);
			break;
		}
		if(f->newfid != f->fid)
			nfp = fidclone(fp, f->newfid);
		else
			nfp = fp;
printf("%s: %d\n", __func__, __LINE__);
		if((wq = devwalk(c, file, fp, nfp, f->wname, f->nwname, &f->ename)) == nil){
			if(nfp != fp)
				deletefid(c, nfp);
printf("%s: %d\n", __func__, __LINE__);
			f->type = Rerror;
		}else{
			if(nfp != fp){
				if(wq->nqid != f->nwname)
					deletefid(c, nfp);
			}
			f->type = Rwalk;
			f->nwqid = wq->nqid;
printf("%s: %d\n", __func__, __LINE__);
			for(i = 0; i < wq->nqid; i++){
				f->wqid[i] = wq->qid[i];
printf("%s: %d. %d) my_type=%d, t=%d, pth=%d\n", __func__, __LINE__, 
					i, wq->qid[i].my_type,
					wq->qid[i].type, 
					wq->qid[i].path
					);
			}
			styxfree(wq);
		}
printf("%s: %d\n", __func__, __LINE__);
		wr(c, f);
		break;
	case	Topen:
		if(Debug)
			printf("\nTopen %d\n", f->fid);
//			fprint(2, "Topen %d\n", f->fid);
		f->ename = nil;
printf("%s: %d. fp->open = %d, fp->qid.type = %x, f->mode = %x\n", __func__, __LINE__, fp->open, fp->qid.type, f->mode);
		if(fp->open)
			f->ename = Eopen;
//		else if(file == nil && (f->mode&(OWRITE|OTRUNC|ORCLOSE)))
//			f->ename = Eperm;
		else if((fp->qid.type & QTDIR) && (f->mode & (OWRITE|OTRUNC|ORCLOSE))){
printf("%s: %d\n", __func__, __LINE__);
			f->ename = Eperm;
		}else if(file != nil && !styxperm(file, c->uname, f->mode))
			f->ename = Eperm;
		else if((f->mode & ORCLOSE) && file != nil && file->parent != nil && !styxperm(file->parent, c->uname, OWRITE))
			f->ename = Eperm;
		if(f->ename != nil){
printf("%s: %d\n", __func__, __LINE__);
			f->type = Rerror;
			wr(c, f);
			break;
		}
printf("%s: %d\n", __func__, __LINE__);
		f->ename = Enonexist;
		decreff(fp);
		if(ops->open == nil || (f->ename = ops->open(&fp->qid, f->mode)) == nil){
printf("%s: %d\n", __func__, __LINE__);
			f->type = Ropen;
			f->qid = fp->qid;
			fp->mode = f->mode;
			fp->open = 1;
			fp->offset = 0;
			incopen(fp);
		}
		else
			f->type = Rerror;
printf("%s: %d\n", __func__, __LINE__);
		increff(fp);
		wr(c, f);
		break;
	case	Tcreate:
		if(Debug)
			printf("\nTcreate %d %s\n", f->fid, f->name);
//			fprint(2, "Tcreate %d %s\n", f->fid, f->name);
		f->ename = nil;
		if(fp->open)
			f->ename = Eopen;
		else if(!(fp->qid.type&QTDIR))
			f->ename = Enotdir;
		else if((f->perm&DMDIR) && (f->mode&(OWRITE|OTRUNC|ORCLOSE)))
			f->ename = Eperm;
		else if(file != nil && !styxperm(file, c->uname, OWRITE))
			f->ename = Eperm;
		if(f->ename != nil){
			f->type = Rerror;
			wr(c, f);
			break;
		}
		f->ename = Eperm;
		decreff(fp);
		if(file != nil){
			if(f->perm&DMDIR)
				f->perm = (f->perm&~0777) | (file->d.mode & f->perm & 0777) | DMDIR;
			else
				f->perm = (f->perm&(~0777 | 0111)) | (file->d.mode & f->perm & 0666);
		}
		if(ops->create && (f->ename = ops->create(&fp->qid, f->name, f->perm, f->mode)) == nil){
			f->type = Rcreate;
			f->qid = fp->qid;
			fp->mode = f->mode;
			fp->open = 1;
			fp->offset = 0;
			incopen(fp);
		}
		else
			f->type = Rerror;
		increff(fp);
		wr(c, f);
		break;
	case	Tread:
		if(Debug)
			printf("\nTread %d\n", f->fid);
		if(!fp->open){
			f->type = Rerror;
			f->ename = Ebadfid;
			wr(c, f);
			break;
		}
printf("%s: %d\n", __func__, __LINE__);
		if(fp->qid.type & QTDIR || (file != nil && file->d.qid.type & QTDIR)){
			f->type = Rread;
printf("%s: %d file = %x, ops = %x, ops->read = %x\n", __func__, __LINE__, file, ops, ops->read);
			if(file == nil){
				f->ename = Eperm;
printf("%s: %d\n", __func__, __LINE__);
				if(
					ops->read && 
					(f->ename = ops->read(fp->qid, c->data, (ulong*)(&f->count), &fp->dri)) == nil
				){
					f->data = c->data;
printf("%s: %d\n", __func__, __LINE__);
				}
				else
					f->type = Rerror;
			}
			else{
				if(
					fp->qid.my_type != 0 &&
					ops->read && 
					(f->ename = ops->read(fp->qid, c->data, (ulong*)(&f->count), &fp->dri)) == nil
				){
					f->data = c->data;
printf("%s: %d\n", __func__, __LINE__);
				}
				else{
				//	f->type = Rerror;
printf("%s: %d\n", __func__, __LINE__);
					f->count = devdirread(fp, file, c->data, f->count);
					f->data = c->data;
printf("%s: %d\n", __func__, __LINE__);
				}
			}		
printf("%s: %d\n", __func__, __LINE__);
		}else{
			f->ename = Eperm;
			f->type = Rerror;
printf("%s: %d\n", __func__, __LINE__);
			if(ops->read && (f->ename = ops->read(fp->qid, c->data, (ulong*)(&f->count), &f->offset)) == nil){
printf("%s: %d\n", __func__, __LINE__);
				f->type = Rread;
				f->data = c->data;
			}
printf("%s: %d\n", __func__, __LINE__);
		}
printf("%s: %d\n", __func__, __LINE__);
		wr(c, f);
printf("%s: %d\n", __func__, __LINE__);
		break;
	case	Twrite:
		if(Debug)
			printf("\nTwrite %d\n", f->fid);
		if(!fp->open){
			f->type = Rerror;
			f->ename = Ebadfid;
			wr(c, f);
			break;
		}
		f->ename = Eperm;
		f->type = Rerror;
		if(ops->write && (f->ename = ops->write(fp->qid, f->data, (ulong*)(&f->count), f->offset)) == nil){
			f->type = Rwrite;
		}
		wr(c, f);
		break;
	case	Tclunk:
		if(Debug)
			printf("\nTclunk %d\n", f->fid);
		open = fp->open;
		mode = fp->mode;
		qid = fp->qid;
		deletefid(c, fp);
		f->type = Rclunk;
		if(open && ops->close && (f->ename = ops->close(qid, mode)) != nil)
			f->type = Rerror;
		wr(c, f);
		break;
	case	Tremove:
		if(Debug)
			printf("\nTremove %d\n", f->fid);
		if(file != nil && file->parent != nil && !styxperm(file->parent, c->uname, OWRITE)){
			f->type = Rerror;
			f->ename = Eperm;
			deletefid(c, fp);
			wr(c, f);
			break;
		}
		f->ename = Eperm;
		if(ops->remove && (f->ename = ops->remove(fp->qid)) == nil) 
			f->type = Rremove;
		else
			f->type = Rerror;
		deletefid(c, fp);
		wr(c, f);
		break;
	case	Tstat:
		if(Debug)
			printf("\nTstat %d qid=%llx\n", f->fid, fp->qid.path);
		f->stat = styxmalloc(MAXSTAT);
		f->ename = "stat error";
		if(ops->stat == nil && file != nil){
			f->type = Rstat;
			f->nstat = convD2M(&file->d, f->stat, MAXSTAT);
		}
		else if(ops->stat && (f->ename = ops->stat(fp->qid, &dir)) == nil){
			f->type = Rstat;
			f->nstat = convD2M(&dir, f->stat, MAXSTAT);
		}
		else
			f->type = Rerror;
		wr(c, f);
		styxfree(f->stat);
		break;
	case	Twstat:
		if(Debug)
			printf("\nTwstat %d\n", f->fid);
		f->ename = Eperm;
		strs = malloc(256);
		convM2D(f->stat, f->nstat, &dir, strs);
		free(strs); strs = NULL;
		dir.name = nilconv(dir.name);
		dir.uid = nilconv(dir.uid);
		dir.gid = nilconv(dir.gid);
		dir.muid = nilconv(dir.muid);
		if(ops->wstat && (f->ename = ops->wstat(fp->qid, &dir)) == nil)
			f->type = Rwstat;
		else
			f->type = Rerror;
		wr(c, f);
		break;
	case	Tversion:
		if(Debug)
			printf("\nTversion\n");
		f->type = Rversion;
		f->msize = MSGMAX;
		f->tag = NOTAG;
		wr(c, f);
		break;
	case Tauth:
		if(Debug)
			printf("\nTauth\n");
		f->type = Rauth;
		wr(c, f);
		break;
	case	Tattach:
		if(Debug)
			printf("\nTattach %d %s %s\n", f->fid, f->uname[0] ? f->uname : c->uname, f->aname[0]? f->aname: c->aname);
		if(fp){
			f->type = Rerror;
			f->ename = "fid in use";
		}else{
			Qid q;

			if(f->uname[0]){
				free(c->uname);
				c->uname = strdup(f->uname);
			}
			if(f->aname[0]){
				free(c->aname);
				c->aname = strdup(f->aname);
			}
			q.path = Qroot;
			q.type = QTDIR;
			q.vers = 0;
			fp = newfid(c, f->fid, q);
			f->type = Rattach;
			f->fid = fp->fid;
			f->qid = q;
			if(ops->attach && (f->ename = ops->attach(c->uname, c->aname)) != nil)
				f->type = Rerror;
		}
		wr(c, f);
		break;
	}
	if(f != NULL)
		free(f);
}

char *
styxinit(Styxserver *server, Styxops *ops, char *port, int perm, int needfile)
{
	int i;

	if(Debug)
		printf("Initialising Styx server on port %s\n", port);
	if(perm == -1)
		perm = 0555;
	server->ops = ops;
	server->clients = nil;
	server->root = nil;
	server->ftab = (Styxfile**)malloc(TABSZ*sizeof(Styxfile*));
	for(i = 0; i < TABSZ; i++)
		server->ftab[i] = nil;
	server->qidgen = Qroot+1;
	if(styxinitsocket() < 0)
		return "styxinitsocket failed";
	server->connfd = styxannounce(server, port);
	if(server->connfd < 0)
		return "can't announce on network port";
	styxinitwait(server);
	server->root = newfile(server, nil, 1, Qroot, "/", perm|DMDIR, eve);
	server->needfile = needfile;
	return nil;
}

char*
styxend(Styxserver *server)
{
	USED(server);
	styxendsocket();
	return nil;
}

char *
styxwait(Styxserver *server)
{
	return styxwaitmsg(server);
}

char *
styxprocess(Styxserver *server)
{
	Client *c;
	int s;

	if(styxnewcall(server)){
		s = styxaccept(server);
		if(s >= 0){
			newclient(server, s);
			styxnewclient(server, s);
		}
	}
	for(c = server->clients; c != nil /*((uint)c & 0xfff00000) >= 0x3ff00000*/; ){
		Client *next = c->next;

		server->curc = c;
		if(c->fd >= 0 && styxnewmsg(server, c->fd))
			c->state |= CRECV;
		if(c->state&(CNREAD|CRECV)){
			if(c->state&CDISC){
				styxfreeclient(server, c->fd);
				freeclient(c);
			}else{
				do
					run(c);
				while(c->state&CNREAD);
			}
		}
		c = next;
	}
	
	return nil;
}

Client*
styxclient(Styxserver *server)
{
	return server->curc;
}

Styxfile*
styxaddfile(Styxserver *server, Path pqid, Path qid, char *name, int mode, char *owner)
{
	Styxfile *f, *parent;

	parent = styxfindfile(server, pqid);
	if(parent == nil || (parent->d.qid.type & QTDIR) == 0)
		return nil;
	f = newfile(server, parent, 0, qid, name, mode, owner);
	return f;
}

Styxfile*
styxadddir(Styxserver *server, Path pqid, Path qid, char *name, int mode, char *owner)
{
	Styxfile *f, *parent;

	parent = styxfindfile(server, pqid);
	if(parent == nil || (parent->d.qid.type&QTDIR) == 0)
		return nil;
	f = newfile(server, parent, 1, qid, name, mode|DMDIR, owner);
	return f;
}

long
styxreadstr(ulong off, char *buf, ulong n, char *str)
{
	int size;

	size = strlen(str);
	if(off >= size)
		return 0;
	if(off+n > size)
		n = size-off;
	memmove(buf, str+off, n);
	return n;
}

Qid*
styxqid(int path, int isdir)
{
	Qid* q = malloc(sizeof(Qid));

	q->path = path;
	q->vers = 0;
	if(isdir)
		q->type = QTDIR;
	else
		q->type = 0;
	return q;
}

void
styxsetowner(char *name)
{
	eve = name;
}
