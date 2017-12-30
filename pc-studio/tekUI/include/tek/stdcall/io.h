#ifndef _TEK_STDCALL_IO_H
#define _TEK_STDCALL_IO_H

/*
**	$Id: io.h $
**	teklib/tek/stdcall/io.h - io module interface
**
**	See copyright notice in teklib/COPYRIGHT
*/

#define TIOLockFile(io,name,mode,tags) \
	(*(((TMODCALL TFILE *(**)(TAPTR,TSTRPTR,TUINT,TTAGITEM *))(io))[-9]))(io,name,mode,tags)

#define TIOUnlockFile(io,lock) \
	(*(((TMODCALL void(**)(TAPTR,TFILE *))(io))[-10]))(io,lock)

#define TIOOpenFile(io,name,mode,tags) \
	(*(((TMODCALL TFILE *(**)(TAPTR,TSTRPTR,TUINT,TTAGITEM *))(io))[-11]))(io,name,mode,tags)

#define TIOCloseFile(io,a) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TFILE *))(io))[-12]))(io,a)

#define TIORead(io,a,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TAPTR,TINT))(io))[-13]))(io,a,buf,len)

#define TIOWrite(io,a,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TAPTR,TINT))(io))[-14]))(io,a,buf,len)

#define TIOFlush(io,a) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TFILE *))(io))[-15]))(io,a)

#define TIOSeek(io,a,offs,offshi,mode) \
	(*(((TMODCALL TUINT(**)(TAPTR,TFILE *,TINT,TINT *,TINT))(io))[-16]))(io,a,offs,offshi,mode)

#define TIOFPutC(io,a,c) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TINT))(io))[-17]))(io,a,c)

#define TIOFGetC(io,a) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *))(io))[-18]))(io,a)

#define TIOFEoF(io,a) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TFILE *))(io))[-19]))(io,a)

#define TIOFRead(io,a,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TAPTR,TINT))(io))[-20]))(io,a,buf,len)

#define TIOFWrite(io,a,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TAPTR,TINT))(io))[-21]))(io,a,buf,len)

#define TIOExamine(io,a,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TTAGITEM *))(io))[-22]))(io,a,tags)

#define TIOExNext(io,a,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TTAGITEM *))(io))[-23]))(io,a,tags)

#define TIOChangeDir(io,a) \
	(*(((TMODCALL TFILE *(**)(TAPTR,TFILE *))(io))[-24]))(io,a)

#define TIOParentDir(io,a) \
	(*(((TMODCALL TFILE *(**)(TAPTR,TFILE *))(io))[-25]))(io,a)

#define TIONameOf(io,a,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TSTRPTR,TINT))(io))[-26]))(io,a,buf,len)

#define TIODupLock(io,a) \
	(*(((TMODCALL TFILE *(**)(TAPTR,TFILE *))(io))[-27]))(io,a)

#define TIOOpenFromLock(io,a) \
	(*(((TMODCALL TFILE *(**)(TAPTR,TFILE *))(io))[-28]))(io,a)

#define TIOAddPart(io,p1,p2,buf,len) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TSTRPTR,TSTRPTR,TINT))(io))[-29]))(io,p1,p2,buf,len)

#define TIOAssignLate(io,name,path) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TSTRPTR))(io))[-30]))(io,name,path)

#define TIOAssignLock(io,name,lock) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TFILE *))(io))[-31]))(io,name,lock)

#define TIORename(io,name,newname) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TSTRPTR))(io))[-32]))(io,name,newname)

#define TIOMakeDir(io,name,tags) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TSTRPTR,TTAGITEM *))(io))[-33]))(io,name,tags)

#define TIODeleteFile(io,name) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR))(io))[-34]))(io,name)

#define TIOSetIOErr(io,newerr) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT))(io))[-35]))(io,newerr)

#define TIOGetIOErr(io) \
	(*(((TMODCALL TINT(**)(TAPTR))(io))[-36]))(io)

#define TIOObtainPacket(io,path,namepart) \
	(*(((TMODCALL TAPTR(**)(TAPTR,TSTRPTR,TSTRPTR *))(io))[-37]))(io,path,namepart)

#define TIOReleasePacket(io,packet) \
	(*(((TMODCALL void(**)(TAPTR,TAPTR))(io))[-38]))(io,packet)

#define TIOFault(io,err,buf,len,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TINT,TSTRPTR,TINT,TTAGITEM *))(io))[-39]))(io,err,buf,len,tags)

#define TIOWaitChar(io,a,timeout) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TFILE *,TINT))(io))[-40]))(io,a,timeout)

#define TIOIsInteractive(io,a) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TFILE *))(io))[-41]))(io,a)

#define TIOOutputFH(io) \
	(*(((TMODCALL TFILE *(**)(TAPTR))(io))[-42]))(io)

#define TIOInputFH(io) \
	(*(((TMODCALL TFILE *(**)(TAPTR))(io))[-43]))(io)

#define TIOErrorFH(io) \
	(*(((TMODCALL TFILE *(**)(TAPTR))(io))[-44]))(io)

#define TIOMakeName(io,name,dest,dlen,mode,tags) \
	(*(((TMODCALL TINT(**)(TAPTR,TSTRPTR,TSTRPTR,TINT,TINT,TTAGITEM *))(io))[-45]))(io,name,dest,dlen,mode,tags)

#define TIOMount(io,name,action,tags) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TINT,TTAGITEM *))(io))[-46]))(io,name,action,tags)

#define TIOFUngetC(io,a,c) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TINT))(io))[-47]))(io,a,c)

#define TIOFPutS(io,a,s) \
	(*(((TMODCALL TINT(**)(TAPTR,TFILE *,TSTRPTR))(io))[-48]))(io,a,s)

#define TIOFGetS(io,a,buf,len) \
	(*(((TMODCALL TSTRPTR(**)(TAPTR,TFILE *,TSTRPTR,TINT))(io))[-49]))(io,a,buf,len)

#define TIOSetFileDate(io,name,date,tags) \
	(*(((TMODCALL TBOOL(**)(TAPTR,TSTRPTR,TDATE *,TTAGITEM *))(io))[-50]))(io,name,date,tags)

#endif /* _TEK_STDCALL_IO_H */
