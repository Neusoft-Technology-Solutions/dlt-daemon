/**
 * @licence app begin@
 * Copyright (C) 2012  BMW AG
 *
 * This file is part of GENIVI Project Dlt - Diagnostic Log and Trace console apps.
 *
 * Contributions are licensed to the GENIVI Alliance under one or more
 * Contribution License Agreements.
 *
 * \copyright
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed with
 * this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *
 * \author Alexander Wenzel <alexander.aw.wenzel@bmw.de> BMW 2011-2012
 *
 * \file dlt_shm.c
 * For further information see http://www.genivi.org/.
 * @licence end@
 */


/*******************************************************************************
**                                                                            **
**  SRC-MODULE: dlt_shm.c                                                     **
**                                                                            **
**  TARGET    : linux                                                         **
**                                                                            **
**  PROJECT   : DLT                                                           **
**                                                                            **
**  AUTHOR    : Alexander Wenzel Alexander.AW.Wenzel@bmw.de                   **
**                                                                            **
**  PURPOSE   :                                                               **
**                                                                            **
**  REMARKS   :                                                               **
**                                                                            **
**  PLATFORM DEPENDANT [yes/no]: yes                                          **
**                                                                            **
**  TO BE CHANGED BY USER [yes/no]: no                                        **
**                                                                            **
*******************************************************************************/

/*******************************************************************************
**                      Author Identity                                       **
********************************************************************************
**                                                                            **
** Initials     Name                       Company                            **
** --------     -------------------------  ---------------------------------- **
**  aw          Alexander Wenzel           BMW                                **
*******************************************************************************/

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

#if !defined(__QNXNTO__)
#include <sys/shm.h>
#endif

#if !defined(_MSC_VER)
#include <unistd.h>
#include <syslog.h>
#endif

#include <dlt_shm.h>
#include <dlt_common.h>

void dlt_shm_print_hex(char *ptr,int size)
{
    int num;

    for (num=0;num<size;num++)
    {
		if((num%16)==15)
			printf("%.2x\n",((unsigned char*)ptr)[num]);
		else
			printf("%.2x ",((unsigned char*)ptr)[num]);
    }
    printf("\n");
}

void dlt_shm_pv(int id,int operation)
{
   #if !defined(__QNXNTO__)
	static struct sembuf semaphor;
	
	semaphor.sem_op = operation;
	semaphor.sem_flg = SEM_UNDO;
	
	if(semop(id, &semaphor,1) == -1) {
		dlt_log(LOG_ERR,"SHM: semop");
	}
   #endif
}

int dlt_shm_init_server(DltShm *buf,int key,int size) {
   #if !defined(__QNXNTO__)
	struct shmid_ds shm_buf;
	unsigned char *ptr;

	// Init parameters
	buf->shmid = 0;
	buf->semid = 0;

    // Create the segment.
    if ((buf->shmid = shmget(key, size, IPC_CREAT | 0666)) < 0) {
        dlt_log(LOG_ERR,"SHM: shmget");
        return -1; /* ERROR */
    }

	// get the size of shm
	if (shmctl(buf->shmid,  IPC_STAT, &shm_buf))
	{
		dlt_log(LOG_ERR,"SHM: shmctl");
        return -1; /* ERROR */
	}	

    // Now we attach the segment to our data space.
    if ((ptr = shmat(buf->shmid, NULL, 0)) == (unsigned char *) -1) {
        dlt_log(LOG_ERR,"SHM: shmat");
        return -1; /* ERROR */
    }
	
	// Init semaphore
	if( (buf->semid = semget(DLT_SHM_SEM,1,S_IRWXU|S_IRWXG|S_IRWXO|IPC_CREAT|IPC_EXCL)) == -1 ) {
		if( (buf->semid = semget(DLT_SHM_SEM,1,S_IRWXU|S_IRWXG|S_IRWXO|IPC_EXCL)) == -1 ) {
			dlt_log(LOG_ERR,"SHM: semget");
			return -1; /* ERROR */
		}
	}
	if( semctl(buf->semid,0,SETVAL,(int)1) == -1 ) {
        dlt_log(LOG_ERR,"SHM: semctl");
        return -1; /* ERROR */
	}
	
	// init buffer
	dlt_buffer_init_static_server(&(buf->buffer),ptr,shm_buf.shm_segsz);

	return 0; /* OK */
   #else
   return -1;
   #endif
}

int dlt_shm_init_client(DltShm *buf,int key) {
   #if !defined(__QNXNTO__)
	struct shmid_ds shm_buf;
	unsigned char *ptr;

	// init parameters
	buf->shmid = 0;
	buf->semid = 0;

    // Create the segment.
    if ((buf->shmid = shmget(key, 0, 0666)) < 0) {
        dlt_log(LOG_ERR,"SHM: shmget");
        return -1; /* ERROR */
    }

	// get the size of shm
	if (shmctl(buf->shmid,  IPC_STAT, &shm_buf))
	{
	    dlt_log(LOG_ERR,"SHM: shmctl");
            return -1; /* ERROR */
	}	

	// Now we attach the segment to our data space.
	if ((ptr = shmat(buf->shmid, NULL, 0)) == (unsigned char *) -1) {
		dlt_log(LOG_ERR,"shmat");
		return -1; /* ERROR */
	}
        	
	// Init semaphore
	if( (buf->semid = semget(DLT_SHM_SEM,0,0)) == -1 ) {
            dlt_log(LOG_ERR,"SHM: semget");
            return -1; /* ERROR */
	}

	// init buffer
	dlt_buffer_init_static_client(&(buf->buffer),ptr,shm_buf.shm_segsz);
    
	return 0; /* OK */
#else
   return -1;
#endif
}

void dlt_shm_info(DltShm *buf)
{
	dlt_buffer_info(&(buf->buffer));
}

void dlt_shm_status(DltShm *buf)
{
	dlt_buffer_status(&(buf->buffer));
}

int dlt_shm_get_total_size(DltShm *buf)
{
	return dlt_buffer_get_total_size(&(buf->buffer));
}

int dlt_shm_get_used_size(DltShm *buf)
{
	int ret;

	/* check if buffer available */
	if(!buf->buffer.mem)
		return -1;

	DLT_SHM_SEM_GET(buf->semid);
	ret =  dlt_buffer_get_used_size(&(buf->buffer));
	DLT_SHM_SEM_FREE(buf->semid);

	return ret;
}

int dlt_shm_get_message_count(DltShm *buf)
{
	return dlt_buffer_get_message_count(&(buf->buffer));
}

int dlt_shm_push(DltShm *buf,const unsigned char *data1,unsigned int size1,const unsigned char *data2,unsigned int size2,const unsigned char *data3,unsigned int size3)
{
	int ret;

	/* check if buffer available */
	if(!buf->buffer.mem)
		return -1;

	DLT_SHM_SEM_GET(buf->semid);
	ret =  dlt_buffer_push3(&(buf->buffer),data1,size1,data2,size2,data3,size3);
	DLT_SHM_SEM_FREE(buf->semid);

	return ret;
}

int dlt_shm_pull(DltShm *buf,unsigned char *data, int max_size)
{
	int ret;

	/* check if buffer available */
	if(!buf->buffer.mem)
		return -1;

	DLT_SHM_SEM_GET(buf->semid);
	ret =  dlt_buffer_pull(&(buf->buffer),data,max_size);
	DLT_SHM_SEM_FREE(buf->semid);

	return ret;
}

int dlt_shm_copy(DltShm *buf,unsigned char *data, int max_size)
{
	int ret;

	/* check if buffer available */
	if(!buf->buffer.mem)
		return -1;

	DLT_SHM_SEM_GET(buf->semid);
	ret =  dlt_buffer_copy(&(buf->buffer),data,max_size);
	DLT_SHM_SEM_FREE(buf->semid);

	return ret;
}

int dlt_shm_remove(DltShm *buf)
{
	int ret;

	/* check if buffer available */
	if(!buf->buffer.mem)
		return -1;

	DLT_SHM_SEM_GET(buf->semid);
	ret =  dlt_buffer_remove(&(buf->buffer));
	DLT_SHM_SEM_FREE(buf->semid);

	return ret;
}

int dlt_shm_free_server(DltShm *buf) {
		
#if !defined(__QNXNTO__)

	if(shmdt(buf->buffer.shm)) {
        dlt_log(LOG_ERR,"SHM: shmdt");
        return -1; /* ERROR */
    }

	if(shmctl(buf->shmid,IPC_RMID,NULL) == -1) {
        dlt_log(LOG_ERR,"SHM: shmdt");
        return -1; /* ERROR */
	}

	if(semctl(buf->semid,0,IPC_RMID,(int)0) == -1) {
        dlt_log(LOG_ERR,"SHM: shmdt");
        return -1; /* ERROR */
	}
#endif

	// Reset parameters
	buf->shmid = 0;
	buf->semid = 0;

	return dlt_buffer_free_static(&(buf->buffer));
		
}

int dlt_shm_free_client(DltShm *buf) {

#if !defined(__QNXNTO__)

	if(shmdt(buf->buffer.shm)) {
        dlt_log(LOG_ERR,"SHM: shmdt");
        return -1; /* ERROR */
    }
#endif

	// Reset parameters
	buf->shmid = 0;
	buf->semid = 0;
		
	return dlt_buffer_free_static(&(buf->buffer));
}
