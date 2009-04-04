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

/* Server -> Client: FileAdd/FileMod/FileRem */

void
GetFileData(session_t data,fileT *filePtr, byte *fileData)
{
	int pos;
	
	pos=0;
	memmove(filePtr, data.data + pos, sizeof(fileT) );
	
	pos += sizeof(fileT);
	if( (data.dataSize - sizeof(fileT) ) > 0 && fileData != NULL ) {
		memmove(fileData, data.data + pos, (data.dataSize - sizeof(fileT)) );	
	}	
}	

int 
CallFileAdd(session_t data)
{
	fileT file;
	byte *fileData;
	int ret;

	GetFileData(data,&file,fileData);		
	ret = FileAdd(file,fileData);	
	
	free(data.data);
	return ret;
}	

int 
CallFileMod(session_t data)
{
	fileT file;
	byte *fileData;
	int ret;

	GetFileData(data,&file,fileData);	
	
	/*Lo saco y vuelvo a insertar */
	FileRem(file);	
	ret = FileAdd(file,fileData);	
	
	return ret;	
}	

int 
CallFileRem(session_t data)
{
	fileT file;
	int ret;
	
	GetFileData(data,&file,NULL);	
	
	ret = FileRem(file);	
	
	return ret;
}


/* Server -> Client: Agregar directorio */
int 
CallDirAdd(session_t data)
{
	fileT *fileList;
	byte **dataBuffer;
	
//	GetDirData(data.data, &fileList, &dataBuffer);

	return DirAdd(data.msg, fileList, dataBuffer, 0);	
}	











