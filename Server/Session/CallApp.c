/*
 *  CallApp.c
 *  server
 *
 *  Created by Damian Dome on 3/30/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */


#include "Session.h"
#include "CallApp.h"
#include "../App/fileHandler.h"
#include "../Transport/OutputPipe.h"

/* Funciones generales */

static session_t
FillPack(string senderID, string msg, uInt opCode, 
		 pid_t pid, size_t dataSize, byte *data)
{
	session_t aux;
	
	if( msg == NULL || strlen(msg) > MAXMSG )
		strcpy(aux.msg,"");
	else	
		strcpy(aux.msg,msg);
	
	if( senderID == NULL || strlen(senderID) > MAXSENDER )
		strcpy(aux.senderID,"");
	else	
		strcpy(aux.senderID,senderID);
	
	aux.opCode   = opCode;
	aux.pid      = pid;
	aux.dataSize = dataSize;
	aux.data     = data;
	
	return aux;
}

/* Client -> Server: Nueva conección */

int 
CallNewConection( session_t *dataPtr )
{
	
	if( (NewClient((*dataPtr).pid)) != NEW_USR_OK ) {
		return ERROR;
	}

	(*dataPtr).opCode = SR_CONECT_OK;
	
	return OK;
}

/* Client -> Server: New Client */

static int
MakeNewClientRetPack(int op, session_t *dataPtr )
{
	session_t aux;
	int ret;
	uInt opCode;
	
	switch(op) {
		case NEW_USRNAME_EXIST: 
			opCode = SR_NEW_USR_ERR;
			ret = OK;
			break;
			
		case NEW_USRNAME_OK:
			opCode = SR_NEW_USR_OK;
			ret = OK;
			break;
			
		default:
			opCode = SR_NEW_USR_ERR;
			ret = ERROR;
			break;
	}
		
	aux = FillPack(NULL,NULL, opCode, 0, 0, NULL );
	
	/* Guardo la respuesta del Server */
	*dataPtr = aux;
	return ret;		
}

int 
CallNewClient(session_t *dataPtr)
{	
	int ret;
	
	ret=ConnectUser((*dataPtr).pid,(*dataPtr).msg);	
	return (MakeNewClientRetPack(ret, dataPtr));
}	

/* Client -> Server: DirRem */	

static string
GetDirName(session_t data)
{
	if( data.dataSize == 0 ) {
		return NULL;
	}
	else {
		return data.data;
	}
}

int 
CallDirRem(session_t data)
{
	string dir;
	string userName;

	dir = GetDirName(data);
	userName = data.msg;
	
    DelDir(userName,dir);
    return OK;
}

/* Client -> Server: FileAdd/FileMod/FileRem */

void
GetFileData(session_t data,fileT *filePtr, byte **fileData_ptr)
{
	int pos;
	byte *fileData;
	
	pos=0;
	memmove(filePtr, data.data + pos, sizeof(fileT) );
	
	fileData = malloc(GetSize(*filePtr));
	
	pos += sizeof(fileT);
	memmove(fileData, data.data + pos, GetSize(*filePtr) );
	
	*fileData_ptr = fileData;	
		
}	

void
GetFileRemData(session_t data,fileT *filePtr)
{
	memmove(filePtr, data.data, sizeof(fileT) );
}

int 
CallFileAdd(session_t data)
{
	fileT file;
	string user; // Usado solo para agregar a los Logs
	int ret;
	pid_t pid;
	
	pid = atoi(data.msg);
    user = GetPIDToUserName(pid);
    
	LogAction(user, data.data, "Add");
    	
	return ret;
}	

int 
CallFileMod(session_t data)
{
	fileT file;
	string user; // Usado solo para agregar a los Logs
	int ret;
	pid_t pid;
	
	pid = atoi(data.msg);
    user = GetPIDToUserName(pid);
    
	LogAction(user, data.data, "Mod");
    	
	return ret;
}	

int 
CallDirDel(session_t data)
{
	int ret;
	
	rmdir(Concat(Concat(data.senderID,"/"),data.msg));
	
	return OK;
}	

int 
CallDirNew(session_t data)
{
	int ret;
	
	
	
	mkdir(Concat(Concat(data.senderID,"/"),data.msg),0777);
	
	return OK;
}	


fileT
CallFileTransfer(session_t data)
{
	fileT file;
	byte *fileData;    fflush(stdout);
	int ret;
	
	
	
	GetFileData(data,&file,&fileData);

    

    fflush(stdout);

    if( FileExists(file) ) {
	    FileRem(file);
	    
    }
	ret = FileAdd(file,fileData);

    

	//free(data.data);
	//free(fileData);
	
	return file;
}	

fileT * 
CallFileRem(session_t * dataPtr)
{
	fileT *file=malloc(sizeof(fileT));
	string user; // Usado solo para agregar a los Logs
	int ret;
	pid_t pid;
	
	pid = atoi(dataPtr->msg);
    user = GetPIDToUserName(pid);
    
    file = (fileT *)(dataPtr->data);
    
    /* Lo borro del sistema */
    FileRem(*file);
   
	LogAction(user, file->fName, "Del");
	
	return file;
}

/* Client -> Server: Client Exit */

int 
CallClientExit(session_t data)
{
	return  DisconnectUser(data.msg);		
}

/* Client -> Server: Cliente pide directorio */
static int
GetFileListSize( int nfiles, fileT *fileList )
{
	int size;
	int i;
	
	size = 0;
	for( i=0; i<nfiles; i++ ) {
		size += GetSize(fileList[i]);
	}
	
	return size;
}


static int
MakeDirPack(int nfiles, fileT * fileList,byte **dataBuffer,byte **pack)
{
	int size;
	int i;
	int pos;
	
	if( (*pack = malloc(size=sizeof(int)+sizeof(fileT)*nfiles+GetFileListSize(nfiles,fileList)) ) == NULL ) {
		return ERROR;
	}
	
	pos = 0;
	memmove(*pack + pos, &nfiles, sizeof(int));	
	pos += sizeof(int);
	
	for( i=0; i<nfiles; i++ ) {		
		memmove(*pack + pos, &(fileList[i]), sizeof(fileT));
		pos += sizeof(fileT);
		memmove(*pack + pos, dataBuffer[i], GetSize(fileList[i]));
		pos += GetSize(fileList[i]);
	}
	
	return size;	
}	

int
CallTransferDir(session_t * dataPtr)
{
	byte **dataBuffer;
	fileT *fileList;
	string dirPath;
	string userName;
	int nfiles;
	string aux;
    
	dirPath  = (*dataPtr).data;
	//userName = (*dataPtr).msg;

	/* armo el paquete respuesta */	
	(*dataPtr).opCode = SR_DIR_TRANS;
	aux = Concat(SERVER_PATH,dirPath);
	strcpy((*dataPtr).msg,aux);
	free(aux);
    
    if( (nfiles=ReqDir(userName, (*dataPtr).msg, &fileList, &dataBuffer)) == ERROR ) {
		return ERROR;
	}
	else {

		if( ((*dataPtr).dataSize = MakeDirPack(nfiles, fileList,dataBuffer,&((*dataPtr).data))) != ERROR ) {
			return OK;
		}
		else {
			return ERROR;
		}
	}
}


int 
CallDirReq(session_t *dataPtr)
{
    int usersxdir;
    string dirName;
    string userName;
    
    dirName  = (*dataPtr).data;
    userName = (*dataPtr).msg;

    (*dataPtr).opCode = SR_DIR_REQ_OK;

    /* Agrego el directorio a la lista de directorios sincronizables para el usuario */
    UserAddDir(userName, dirName);
	
    return OK;
}	


/* Client -> Server: Pedido de lista de directorios */

static int
MakeDirListPack( int ndirs, string *dirList, byte **dataBuffer )
{
	int pos;
	int size;
	int i;
	
	*dataBuffer = malloc(size=ndirs*MAX_DIR_NAME+sizeof(int));
	
	pos = 0;
	memmove(*dataBuffer, &ndirs, sizeof(int));
	pos += sizeof(int);
	
	for( i=0; i<ndirs; i++ ) {
		memmove(*dataBuffer+pos, dirList[i], MAX_DIR_NAME);
		pos += MAX_DIR_NAME;
	}
		
	return size;
}


int
CallDirList(session_t *dataPtr)
{
	string *out;
	int ndirs;
	int i;
	
	ndirs = ListAllSyncDirs( &out );
	
	(*dataPtr).opCode = SR_DIR_LST;
	
	(*dataPtr).dataSize = MakeDirListPack( ndirs, out, &((*dataPtr).data) );
	
	for( i=0; i<ndirs; i++ ) {
		free(out[i]);
	}
	free(out);	

	return OK;
}	
	

	
	



