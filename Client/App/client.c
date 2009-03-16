/*
 *  client.c
 *  tpSO1
 *
 *  Created by Damian Dome on 3/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "client.h"



static string *
MakeFilesArray(fileT *files, int nfiles)
{
	string *array;
	int i;
	
	if( (array=malloc(sizeof(string)*nfiles)) == NULL )
		return NULL;

	for( i=0; i<nfiles; i++ ) {
		array[i] = Concat(files[i].path,files[i].fName);
	}

	return array;
}

static int
FilesHandler()
{
	string *auxArray;	
	fileT * fileList;
	int idFile;
	
	//Hasta que no se salga del programa
	//el problema es que no puedo detectar cuando se agregan nuevos archivos
	//para vigilar
	while(1) {
		fileList = InitFiles();
		idFile = FilesHasChanged(auxArray=MakeFilesArray(fileList, 5), 5);
		if( idFile != -1 )
			printf("HAGO ALGO CON EL ARCHIVO QUE CAMBIO%d\n",idFile);
		free(auxArray);
	}
	
	return OK;
}

int
InitClientApp()
{
	fileT *fileList;

	switch(fork()) {
		case 0: 
			return FilesHandler();
		default:
			ClientPrompt();
			wait(NULL); //tengo que esperar que termine el hijo antes creado
	}		
	return OK;
}

int
ClientPrompt()
{
	return OK;
}

fileT *
InitFiles()
{
	int i;
	
	//Por ahora tenemos 5 fijos
	//creados en ejecucion
	//deberiamos levantarlos al iniciar, de alguna manera global.
	fileT *array;	
	
	array = malloc(sizeof(fileT)*5);
	
	for( i=0; i<5; i++ ) {
		array[i].idFile = i;
		array[i].path = "/SO/";
	}
	array[0].fName = "Prueba1";
	array[1].fName = "Prueba2";
	array[2].fName = "Prueba3";
	array[3].fName = "Prueba4";
	array[4].fName = "Prueba5";		
	for( i=0; i<5; i++ ) {
		CreateFile(array[i].path, array[i].fName);
	}
	
	return array;		
}