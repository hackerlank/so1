#include "ipcSockets.h"
#include "ipcInterface.h"

char * clientPath = NULL;
char * serverPath = NULL;
int recibidos = 0;
int socketFD;
pthread_mutex_t rdmtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wrmtx=PTHREAD_MUTEX_INITIALIZER;
int IPCStarted = FALSE;
int isChildProcess = FALSE;

static byte * GetBlock(byte *org, size_t size, int index);

static string
MakeRDPath(key_t key)
{
    string path;
    if((path = calloc(MAX_DIR_NAME, sizeof(char)))==NULL)
	return NULL;

    sprintf(path,"%s%d_rd",COMM_DIR,(int)key);

    return path;
}

static string
MakeWRPath(key_t key)
{
    string path;
    if((path = calloc(MAX_DIR_NAME, sizeof(char)))==NULL)
	return NULL;

    sprintf(path,"%s%d_wr",COMM_DIR,(int)key);

    return path;
}

int InitIPC(key_t key)
{
    int status = OK;

    clientPath=MakeRDPath(key);
    serverPath=MakeWRPath(key);

    struct sockaddr_un server;
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, serverPath);
    unlink(server.sun_path);
    if( (socketFD=socket(AF_UNIX,SOCK_DGRAM,0))==-1 )
    {
	printf("A comerlaaaaaaa!!!\n");
	return ERROR;
    }
    if (bind(socketFD, (struct sockaddr *)&server, sizeof(struct sockaddr_un)) == -1) 
    {
	printf("Murio\n");
	return ERROR;
    }

    IPCStarted = TRUE;
    isChildProcess = TRUE;
    return status;
}

int InitReadIPC(key_t key)
{
    int status = OK;

    clientPath=MakeRDPath(key);
    serverPath=MakeWRPath(key);

    struct sockaddr_un server;
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, serverPath);
    unlink(server.sun_path);
    if( (socketFD=socket(AF_UNIX,SOCK_DGRAM,0))==-1 )
    {
	printf("A comerlaaaaaaa!!!\n");
	return ERROR;
    }
    if (bind(socketFD, (struct sockaddr *)&server, sizeof(struct sockaddr_un)) == -1) 
    {
	printf("Murio\n");
	return ERROR;
    }

    IPCStarted = TRUE;
    isChildProcess = TRUE;
    return status;
}

int InitWriteIPC(key_t key)
{
    int status = OK;

    clientPath=MakeRDPath(key);
    serverPath=MakeWRPath(key);

    struct sockaddr_un server;
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, serverPath);
    unlink(server.sun_path);
    if( (socketFD=socket(AF_UNIX,SOCK_DGRAM,0))==-1 )
    {
	printf("A comerlaaaaaaa!!!\n");
	return ERROR;
    }
    if (bind(socketFD, (struct sockaddr *)&server, sizeof(struct sockaddr_un)) == -1) 
    {
	printf("Murio\n");
	return ERROR;
    }

    IPCStarted = TRUE;
    isChildProcess = TRUE;
    return status;
}

static int
GetTotalPackets(size_t size)
{
	return (int)(size / PACKET_SIZE + 1) ;
}

int 
WriteIPC(key_t key, void * data, size_t size)
{
    pthread_mutex_lock(&wrmtx);
    InitWriteIPC(key);
    int status;
    headerIPC_t header;
	byte *block;
	int bytesLeft;	
	
	header.totalPackets = GetTotalPackets(size);
	
	int npacket   = 1;
	bytesLeft = size;
	int i;

	struct sockaddr_un client;
	client.sun_family = AF_UNIX;
	strcpy(client.sun_path, clientPath);
	
	for( i=0; i < header.totalPackets; i++ ) {
		header.nPacket = npacket;
		header.size = PACKET_SIZE;
		
		status=sendto(socketFD,&header,sizeof(headerIPC_t),0,(struct sockaddr*)&client,sizeof(struct sockaddr_un));
		if(status != ERROR)
		{
			block = GetBlock(data, header.size, npacket-1);
			status=sendto(socketFD,block,header.size,0,(struct sockaddr*)&client,sizeof(struct sockaddr_un));
			free(block);
			
			if( status == ERROR )
                        {
                            pthread_mutex_unlock(&wrmtx);
                            return ERROR;
                        }
				
		}
		
		bytesLeft -= PACKET_SIZE;
		npacket++;
	}
    pthread_mutex_unlock(&wrmtx);
    return status;
}

byte *
ReadIPC(key_t key)
{
        pthread_mutex_lock(&rdmtx);
        InitReadIPC(key);
	int status = OK;
	headerIPC_t header;
	byte * data=NULL;
	int nPacketsRead;
	int pos;
	byte *aux;
	
	printf("Descargando paquetes...");
	
	nPacketsRead=0;
	pos=0;
	do{
		status=recvfrom(socketFD,&header,sizeof(headerIPC_t),0,NULL,0);
		if(status > 0)
		{	
			data = realloc(data, pos + header.size );
			aux = malloc(header.size);			
			status=recvfrom(socketFD,aux,header.size,0,NULL,0);
			
			memmove(data+pos, aux, header.size);
			pos += header.size;
			nPacketsRead++;
			
			if(status > 0)
			{
				status = OK;
				recibidos++;
			}
			else
			{
				status = ERROR;
			}			  
		}
		else
		{
			status = ERROR;
		}
	}while( status != ERROR && nPacketsRead < header.totalPackets );
	
	printf("recibidos: %d\n", nPacketsRead);
	pthread_mutex_unlock(&rdmtx);
	return status == ERROR ? NULL: data ;
}

void
CloseIPC(void)
{
    /*Liberar recursos...*/
}

static byte *
GetBlock(byte *org, size_t size, int index)
{
	byte *block;
	
	if( (block=malloc(size)) == NULL ) {
		return NULL;
	}
	
	return memcpy(block, org+index*size, size);
}



