#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define	TIMEOUT		1
#define	DEF_CONT	10
#define	SERVER_PORT	8000
#define	BUFSIZE	1500
#define	LISTEN	128
#define	TRUE	1
#define	FALSE	0
pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
typedef struct
{
	void * (*task)(void *);
	void * arg;
}task_t;
typedef struct
{
	int shutdown;
	int thread_max;
	int thread_min;
	int thread_busy;
	int thread_alive;
	pthread_t * tids;
	task_t * taskqueue;
	int queue_max;
	int queue_size;
	int queue_front;
	int queue_rear;
	int wait;
	pthread_t managerid;
	pthread_mutex_t lock;
	pthread_mutex_t arg_lock;
	pthread_cond_t not_full;
	pthread_cond_t not_empty;
}pool_t;

/*创建线程池*/
pool_t * Create_pool(int ,int ,int);
void Producer(pool_t *,void*(*)(void*),void*);
void * Consumer(void*);
void * Manager(void*);
int if_alive_thread(pthread_t);
int initSocket(void);
void * Jobs(void * arg);
