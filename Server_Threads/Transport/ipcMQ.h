#ifndef __IPC_MQ_H__
#define __IPC_MQ_H__

/* System Includes
*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include "../Lib/defines.h"
#include "Transport.h"


/* Defines de los Fifo's en particular
*/

#define __DEFAULT_MQ_MODE__ 0777

#define PACKET_SIZE 1000

/* Funciones
*/

int InitMainIPC(void);

int InitIPC(key_t key);

int WriteIPC(key_t key, void * data, size_t size);

byte * ReadIPC(key_t key);

void CloseIPC(void);

#endif