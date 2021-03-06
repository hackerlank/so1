#include "inotify.h"

#define EVENT_SIZE  (sizeof (struct inotify_event))
#define BUF_LEN        (1024 * (EVENT_SIZE + 16))
#define IS_DIR_MASK 0x40000000
#define PARAMS IN_DELETE | IN_DELETE_SELF | IN_CREATE | IN_MOVED_FROM | IN_MOVED_TO | IN_MODIFY

char *bk_path_client;


/*IN_MODIFY*/
typedef struct dirWD_T{
    int wd;
    char * path;
}dirWD_T;

typedef struct resp_T{
    char path[MAX_DIR_NAME];
    int isDir;
    int opCode;
}resp_T;

extern char * bk_path;
extern char name[MAX_LINE];

resp_T * read_events (int fd,listADT list,int * lastCookie,int* lastMask);

dirWD_T * MakeDirWD(int wd,char * path);

int NotifyServer(pid_t pid, key_t key, resp_T * resp, char name[MAX_LINE]);

int
AddNewDir(int fd, char * pathAux,listADT list)
{
    char *auxS;
    int cant,i,wd;
    dirWD_T * aux;
    char ** paths;
    
    printf("fd=(%d) - pathAux=(%s)\n",fd,pathAux);
    /*FALTA: Agregar a la estructura de busqueda*/
    wd = inotify_add_watch (fd, pathAux,PARAMS );

    if(wd<0)
    {
	if(errno==EACCES)
	    printf("Read access to the given file is not permitted. \n");
	else if(errno==EBADF)
	    printf("The given file descriptor is not valid. . \n");
	else if(errno==EFAULT)
	    printf("pathname points outside of the process's accessible address space.  \n");
	else if(errno==EINVAL)
	    printf("The given event mask contains no legal events; or fd is not an inotify file descriptor.  \n");
	else if(errno==ENOMEM)
	    printf("Insufficient kernel memory was available.\n");
	else
	    printf("The user limit on the total number of inotify watches was reached or the kernel failed to allocate a needed resource.\n");
	
	return ERROR;
    }
    aux=MakeDirWD(wd,pathAux);
    printf("WD %d\n",wd);
    Insert(list,aux);
    /*free(aux);*/
    cant=DirPathList(pathAux, &paths);
    /*free(pathAux);*/
    for(i=0;i<cant-1;i++)
    {
	auxS=paths[i];
	if(auxS!=NULL)
	    AddNewDir(fd,auxS ,list);
        /*free(auxS);*/
    }
    /*free(paths);*/
    return OK;
}

int
Compare(dirWD_T * elem1,dirWD_T * elem2)
{
    if(elem1->wd>elem2->wd)
        return 1;
    else
    {
        if(elem1->wd<elem2->wd)
            return -1;
        else
            return 0;
    }
}

void
FreeElement(dirWD_T * elem)
{
    free(elem->path);
    free(elem);
}

dirWD_T *
MakeDirWD(int wd,char * path)
{
    dirWD_T * resp;
    if( (resp=malloc(sizeof(struct dirWD_T)))==NULL )
    {
        WritePrompt ("Error: inotify_init");
        return NULL;
    }
    resp->wd=wd;
    resp->path=CreateString(path);
    return resp;
}

int
inotifyWatcher(process_t process)
{    
    int lastCookie=0;
    int lastMask=0;
    int fd;
    int ret=0;
    int error=0;
    int status = __INOTIFY_DISABLE__;
    key_t key;
    char * pathAux, * serverPath;
    char signal = __INOTIFY_NO_DATA__;
    resp_T * resp;
    listADT list;
    list=Newlist( (int (*)(void*,void*))Compare,(void (*)(void*))FreeElement);
    if(list==NULL)
    {
        fprintf(stderr,"Error al crear la lista.\n");
        return ERROR;
    }
    
    /* Se espera el OK de que ya esta creada la carpeta
    *  a vigilar.
    */

    printf("Voy a vigilar --%s--\n",process.dir);
	fflush(stdout);
    
    
    while(signal == __INOTIFY_NO_DATA__)
    {
      usleep(__POOL_WAIT__);
      signal = ReadINotifyMsg();
    }
    status = __INOTIFY_ENABLE__;
    /* No reventemos el micro
    */
   // usleep(500000);
    /* Se inicia inotify. 
    */
    fd = inotify_init();
    if (fd < 0)
    {
        printf ("Error: inotify_init\n");
        return ERROR;
    }

    /*Agregar directorio y subdirectorios al vigilador.
    */
    pathAux=Concat(bk_path_client,process.dir);
    //serverPath = Concat(bk_path, process.dir);//DEPRECATED
    //key = ftok(serverPath, __DEFAULT_PID__);//DEPRECATED

    printf("Voy a vigilar --%s--\n",pathAux);
	fflush(stdout);
	
    ret=AddNewDir(fd,pathAux,list);
    if(ret==ERROR)
    {
      close(fd);
      return ERROR;
    }
    while (1)
    {
            printf("Leo evento.\n");
            printf("===========\n");
            resp=read_events(fd,list,&lastCookie,&lastMask);

	        if( resp->opCode==BORRAR ) {
	            printf("Borrar\n");
	            fflush(stdout);
	        }
	        else if( resp->opCode==CREAR ) {
	            printf("Crear\n");
	            fflush(stdout);
	        }
	        else if( resp->opCode==MODIFICAR ) {
	            printf("Modificar\n");
	            fflush(stdout);
	        }
	        else
	            error=1;
            
            /* Aviso al servidor de la modificacion
            */
	    if(!error)
	    {
		signal = ReadINotifyMsg();
		if(signal == __INOTIFY_DISABLE__ || signal == __INOTIFY_ENABLE__)
		{
		    status = signal;
		}
		if(signal == __INOTIFY_EXIT__)
		{
		    exit(EXIT_SUCCESS);
		}
		printf("Recibi la senial %c\n", signal);
		if(status != __INOTIFY_DISABLE__) {
		    fflush(stdout);   
		    ret = NotifyServer(process.pid, key, resp, name);
	    }
            
        }
	
	    error=0;
    }
    if(pathAux != NULL)
        free(pathAux);
    FreeList(list);
    return OK;
}

int
print_mask_info ( unsigned int mask , int isDir,struct inotify_event * event,int fd,char * pathName,listADT list)
{
    printf("(%x) ", mask);
    char * pathAux;
    int ret=0;
    char * aux;
    dirWD_T * data;

    switch (mask)
    {
        case IN_ACCESS:        		printf ("File was accessed (read)\n");
        break;
        case IN_ATTRIB:           	printf ("Metadata changed\n");
        break;
        case IN_CLOSE_WRITE: 		printf ("File opened for writing was closed\n");
        break;
        case IN_CLOSE_NOWRITE:    	printf ("File not opened for writing was closed\n");
        break;
        case IN_CREATE:        		printf ("File/directory created in watched directory\n");
					ret=IN_CREATE;
                                        if(isDir)
                                        {
                                            aux=Concat(pathName,"/");
                                            pathAux=Concat(aux,event->name);
                                            free(aux);
                                            /*printf("%s\n",pathAux);*/
                                            AddNewDir(fd,pathAux,list);
                                            free(pathAux);
                                        }
        break;
        case IN_DELETE:        		printf ("File/directory deleted from watched directory\n");
					ret=IN_DELETE;
					/*if(isDir)
					{
					    aux=Concat(pathName,"/");
                                            pathAux=Concat(aux,event->name);
                                            free(aux);
					    data=MakeDirWD(wd,pathAux);
					    Delete(list,data);
					    free(pathAux);
					}*/
        break;
        case IN_DELETE_SELF:    	printf ("Watched file/directory was itself deleted\n");
					ret=IN_DELETE_SELF;
					/*if(isDir)
					{
					    aux=Concat(pathName,"/");
                                            pathAux=Concat(aux,event->name);
                                            free(aux);
					    data=MakeDirWD(wd,pathAux);
					    Delete(list,data);
					    free(pathAux);
					}*/
        break;
        case IN_MODIFY:        		printf ("File was modified\n");
					ret=IN_MODIFY;
        break;
        case IN_MOVE_SELF:    		printf ("Watched file/directory was itself moved\n");
        break;
        case IN_MOVED_FROM:    		printf ("File moved out of watched directory\n");
					ret=IN_MOVED_FROM;
					/*if(isDir)
					{
					    aux=Concat(pathName,"/");
                                            pathAux=Concat(aux,event->name);
                                            free(aux);
					    data=MakeDirWD(wd,pathAux);
					    Delete(list,data);
					    free(pathAux);
					}*/
        break;
        case IN_MOVED_TO:    		printf ("File moved into watched directory\n");
					ret=IN_MOVED_TO;
                                        if(isDir)
                                        {
                                            aux=Concat(pathName,"/");
                                            pathAux=Concat(aux,event->name);
                                            free(aux);
                                            /*printf("%s\n",pathAux);*/
                                            AddNewDir(fd,pathAux,list);
                                            free(pathAux);
                                        }
        break;
        case IN_OPEN:        		printf ("File was opened\n");
        break;
        default:        		printf ("Error\n");
    }
    
    return ret;
}

char *
GetNewPath(struct inotify_event * event,listADT list)
{
    char * resp=NULL;
    dirWD_T * aux;
    int ret;
    int wdAux=event->wd;
    SetBegin(list);
    ret=GetData(list,(void **)&aux);
    while(ret==1 && aux->wd!=wdAux)
    {
        ret=GetData(list,(void **)&aux);
    }
    if(aux->wd==wdAux)
	resp=aux->path;
    return resp;
}

resp_T *
read_events (int fd,listADT list,int * lastCookie,int* lastMask)
{
    char buf[BUF_LEN];
    char * aux,*pathAux;

    int len, i = 0,ret;
    unsigned int mask;
    resp_T * resp=malloc(sizeof(struct resp_T));
    struct inotify_event *event;
    int isDir=0;
    if(resp==NULL)
	return NULL;
    WritePrompt("1");
    len = read (fd, buf, BUF_LEN);
    WritePrompt("2");
    event = (struct inotify_event *) &buf[i];
    if((event->name)[0]!='.')
    {
	if (len <= 0)
	{
	    printf ("Error al leer evento.\n");
	    return NULL;
	}
	WritePrompt("3");
	while (i < len)
	{
	    printf("Proceso evento.\n");   
	    event = (struct inotify_event *) &buf[i];
	    mask=event->mask;
	    if((mask&IS_DIR_MASK)==IS_DIR_MASK)
	    {
		mask=mask^IS_DIR_MASK;
		isDir=1;
	    }
	    aux=GetNewPath(event,list);
	    printf("(%s)\n",aux);
	    ret=print_mask_info ( mask , isDir,event,fd,aux,list);
	    /*free(aux);*/
	    pathAux=Concat(aux,"/");
	    pathAux=Concat(pathAux,event->name);
	    
	    printf("Last cookie %d\n",*lastCookie);
	    printf("Cookie %d\n",event->cookie);
	    printf("Last Mask %x\n",*lastMask);
	    printf("Mask %x\n",mask);
	    printf("Path entero del archivo: (%s)\n",pathAux);
	    
	    if (event->len)
	    {
		printf ("name=%s\n", event->name);
	    }
	    
	    /*if(*lastCookie==event->cookie && *lastMask==40 && mask==80)
	    {
	    }
	    else if( *lastCookie==event->cookie && *lastMask==40 && mask!=80)
	    {
	    }
	    else
	    {*/
	    if(ret==IN_MODIFY)
		    resp->opCode=MODIFICAR;
	    else if(ret==IN_CREATE || ret==IN_MOVED_TO)
		    resp->opCode=CREAR;
	    else if(ret==IN_DELETE || ret==IN_DELETE_SELF || ret==IN_MOVED_FROM)
		    resp->opCode=BORRAR;
	    else
		    resp->opCode=ERROR;
	    
	    resp->isDir=isDir;
	    strcpy(resp->path,pathAux);
	    /*}*/
	    *lastCookie=event->cookie;
	    *lastMask=mask;
	    printf ("\n");

	    i += EVENT_SIZE + event->len;
	    isDir=0;
	}
    }
    else
	resp->opCode=ERROR;
	
    return resp;
}

int
NotifyServer(pid_t pid, key_t key, resp_T * resp, char name[MAX_LINE])
{
    fileT file;
    char * path, * fileName;
    int status;
    
    printf("Comunicacion con el servidor OK\n");
    fflush(stdout);

    path = GetPathFromBackup(resp->path);
    fileName = GetFileName(resp->path);
    printf("*de *%s* saque *%s* y el fileName *%s*\n", resp->path, path, fileName);
    fflush(stdout);
    
    file = NewFileT(path, fileName);
    
    while(InitCommunication(__DEFAULT_PID__) == ERROR)
      usleep(__POOL_WAIT__);
    
    printf("Comunicacion con el servidor OK\n");
    fflush(stdout);
     switch(resp->opCode)
    {
       case BORRAR:
       
            if( resp->isDir ) {
            
                SendDirDel(path,fileName,pid);
                break;
            }
            
            printf("Mando pedido de borrar archivo\n");
            fflush(stdout);
            SendFileDelTransferSignal(pid, file,getpid());
            printf("OK\n");
            fflush(stdout);            
            break;
        case CREAR:
        
            if( resp->isDir ) {
            
                SendDirNew(path,fileName,pid);
                break;
            }
               
            printf("Mando pedido de agregar archivo\n");
            fflush(stdout);
            SendFileAddTransferSignal(pid, file,getpid());
            printf("OK\n");
            
            fflush(stdout);            
            printf("Le voy a tratar de mandar los datos usando el pid %d\n",getpid());
            fflush(stdout);
            
            sleep(1);
            
            /* Me conecto al servidor de demanda */
            while(InitCommunication(getpid()) == ERROR)
                usleep(__POOL_WAIT__);
            printf("Le mando el archivo!!!\n"); 
            fflush(stdout);   
            /* Le mando el archivo */
            SendFile(file, pid);   
            break;
        case RENAME:
            break;
        case MODIFICAR:
        
            printf("Mando pedido de modificacion\n");
            fflush(stdout);
            SendFileModTransferSignal(pid, file,getpid());
            printf("OK\n");
            fflush(stdout);            
            printf("Le voy a tratar de mandar los datos usando el pid %d\n",getpid());
            fflush(stdout);
            
            sleep(1);
            
            /* Me conecto al servidor de demanda */
            while(InitCommunication(getpid()) == ERROR)
                usleep(__POOL_WAIT__);
            printf("Le mando el archivo!!!\n"); 
            fflush(stdout);   
            /* Le mando el archivo */
            SendFile(file, pid);   
            
            break;
        default:
            status =  OK;
            break;
    }
    return status;
}


void 
ReadBkPathClient(FILE *fd)
{
	int c;
	int pos;
	char *path;

	path = malloc(150);	

	pos=0;
	while(fgetc(fd) != '\n');
	while((c=fgetc(fd)) != '#') {
		path[pos] = c;
		pos++;
	}

	bk_path_client = path;
}

	
	

int
InitNotify(void)
{
	FILE *fd;
	int ret;

	if( (fd=fopen("config","r")) == NULL ) {
		printf("El archivo config es inexistente\n");
		ret = ERROR;
	}
	else {
		ReadBkPathClient(fd);
		ret = OK;
	}

	return ret;
}
	


























