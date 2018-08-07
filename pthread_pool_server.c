#include <pool.h>
pool_t * Create_pool(int max,int min,int quemax)
{
		int err;
		pool_t * pool;
		if((pool = (pool_t *)malloc(sizeof(pool_t)))==NULL)
				return NULL;
		pool->thread_max=max;
		pool->thread_min=min;
		pool->queue_max=quemax;
		pool->shutdown=TRUE;
		pool->thread_busy=0;
		pool->thread_alive=0;
		pool->queue_size=0;
		pool->queue_front=0;
		pool->queue_rear=0;
		pool->wait=0;
		if((pool->tids = (pthread_t *)malloc(max * sizeof(pthread_t)))==NULL)
				return NULL;
		memset(pool->tids,0,max*sizeof(pthread_t));
		if((pool->taskqueue = (task_t *)malloc(quemax * sizeof(task_t)))==NULL)
				return NULL;
		if(pthread_mutex_init(&pool->lock,NULL)!=0||pthread_mutex_init(&pool->arg_lock,NULL)!=0||pthread_cond_init(&pool->not_full,NULL)!=0||pthread_cond_init(&pool->not_empty,NULL)!=0)
		{
				printf("init lock or cond error...\n");
				exit(0);
		}
		for(int i=0;i<DEF_CONT;i++)
		{
				if((err=pthread_create(&pool->tids[i],NULL,Consumer,(void*)pool))>0)
				{
						printf("thread_pool init error creat thread:%s\n",strerror(err));
						return NULL;
				}
				printf("PoolInit:Creating Consumer threadtid:0x%x\n",(unsigned int )pool->tids[i]);
				pool->thread_alive++;
		}
		pthread_create(&pool->managerid,NULL,Manager,(void*)pool);
		printf("Creating Manager threadtid:0x%x\n",(unsigned int )pool->managerid);
		printf("Create thread pool Success..\n");
		return pool;	
}
/*生产者工作*/
void Producer(pool_t * p,void*(*jobs)(void*),void*arg)
{
		pool_t * pool = (pool_t*)p;
		pthread_mutex_lock(&pool->lock);
		while(pool->shutdown==TRUE && pool->queue_max==pool->queue_size){
				pthread_cond_wait(&pool->not_full,&pool->lock);
		}
		if(pool->shutdown == FALSE){
				pthread_mutex_unlock(&pool->lock);
				/*线程退出处理*/
		}
		/*行不行不一定?*/
		if(pool->taskqueue[pool->queue_rear].arg!=NULL)
		{
				free(pool->taskqueue[pool->queue_rear].arg);
				pool->taskqueue[pool->queue_rear].arg=NULL;
		}
		pool->taskqueue[pool->queue_rear].task=jobs;
		pool->taskqueue[pool->queue_rear].arg=arg;
		pool->queue_rear = (pool->queue_rear+1)%pool->queue_max;
		pool->queue_size++;
		/*生产者唤醒任意一个消费者消费任务*/
		pthread_cond_signal(&pool->not_empty);
		pthread_mutex_unlock(&pool->lock);
}
void * Consumer(void * p)
{
		task_t tmpts;
		pool_t * pool = (pool_t *)p;
		while(TRUE)
		{
				pthread_mutex_lock(&pool->lock);
				while(pool->shutdown==TRUE && pool->queue_size == 0)
				{
						pthread_cond_wait(&pool->not_empty,&pool->lock);
						if(pool->wait>0&&pool->thread_alive>pool->thread_min){
								pool->wait--;
								pool->thread_alive--;
								pthread_mutex_unlock(&pool->lock);
								pthread_exit(NULL);
						}
				}
				if(pool->shutdown==FALSE){
						pthread_mutex_unlock(&pool->lock);
						pthread_exit(NULL);
				}
				tmpts.task = pool->taskqueue[pool->queue_front].task;
				tmpts.arg = pool->taskqueue[pool->queue_front].arg;
				pool->queue_front = (pool->queue_front+1)%pool->queue_max;
				pool->queue_size--;
				pthread_cond_signal(&pool->not_full);	
				pool->thread_busy++;
				pthread_mutex_unlock(&pool->lock);
				(*tmpts.task)(tmpts.arg);
				pthread_mutex_lock(&pool->lock);
				pool->thread_busy--;
				pthread_mutex_unlock(&pool->lock)	;
		}
		pthread_exit(NULL);
}
void * Manager(void * p)
{
		pool_t * pool = (pool_t *)p;
		int alive,busy,size;
		int i;
		int add=0;
		printf("Manager thread Starting..\n");
		while(pool->shutdown){
				/*读取线程池阀值,当前任务数,忙数,试探存活,计算闲数*/
				pthread_mutex_lock(&pool->lock);
				alive = pool->thread_alive;
				busy = pool->thread_busy;
				size = pool->queue_size;
				pthread_mutex_unlock(&pool->lock);
				//1.扩容:任务数大于闲数
				if(size > (alive-busy)||(float)busy/alive * 100 >=(float)50&&alive<pool->thread_max)
				{
						for(i=0;i<pool->thread_max&&add<DEF_CONT;i++)
						{
								if(pool->tids[i]==0||!if_alive_thread(pool->tids[i]))
								{
										pthread_create(&pool->tids[i],NULL,Consumer,(void*)pool);
										add++;
										pthread_mutex_lock(&pool->lock);
										pool->thread_alive++;
										pthread_mutex_unlock(&pool->lock);
								}
						}	

				}
				//2.忙数倍数小于闲数,缩减
				if((busy*2) < (alive-busy) && alive > pool->thread_min)
				{
						pthread_mutex_lock(&pool->lock);
						pool->wait=DEF_CONT;
						pthread_mutex_unlock(&pool->lock);
						for(i=0;i<DEF_CONT;i++)
								pthread_cond_signal(&pool->not_empty);
				}
				sleep(TIMEOUT);
				printf("%d %d %d %d\n",pool->thread_max,pool->thread_min,pool->queue_size,alive);
				printf("Manager:pool_info busy:%d  alive:%d  idle:%d  busy/alive:%.2f  alive/all:%.2f\n",busy,alive,alive-busy,((float)busy/alive)*100,((float)alive/pool->thread_max)*100);
		}
		pthread_exit(NULL);
}
int if_alive_thread(pthread_t tid)
{
		if((pthread_kill(tid,0))!=0){
				if(errno == ESRCH)
						return FALSE;
		}
		return TRUE;
}
int initSocket(void)
{
		int sockfd;
		struct sockaddr_in serveraddr;
		bzero(&serveraddr,sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port=htons(SERVER_PORT);
		serveraddr.sin_addr.s_addr=htonl(INADDR_ANY);
		sockfd = socket(AF_INET,SOCK_STREAM,0);
		bind(sockfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr));
		listen(sockfd,LISTEN);
		return sockfd;
}
/*用户工作*/
void * jobs(void * arg)
{
		struct sockaddr_in clientaddr;
		int size = sizeof(clientaddr);
		long sockfd = (long)arg;
		int clientfd,len;
		char buf[BUFSIZE];
		printf("thread_id:0x%x Accpeting...\n",(unsigned int)pthread_self());
		pthread_mutex_lock(&lock);
		clientfd = accept(sockfd,(struct sockaddr *)&clientaddr,&size);
		pthread_mutex_unlock(&lock);
		int j;
		while((len = read(clientfd,buf,sizeof(buf)))>0)
		{
				j=0;
				while(len > j){
						buf[j]=toupper(buf[j]);
						j++;
				}
				write(clientfd,buf,len);
				bzero(buf,sizeof(buf));
		}
		if(len == 0){
				close(clientfd);

		}
}
int main(void)
{
		pool_t * p;
		int epfd;
		int ready;
		long serverfd = initSocket();
		struct epoll_event tmpev;
		struct epoll_event evearr[1];
		epfd = epoll_create(10);
		tmpev.data.fd =serverfd;
		tmpev.events=EPOLLIN; 
		//EPOLLET:边缘触发 EPOLLLT:水平触发
		tmpev.events|=EPOLLET;
		epoll_ctl(epfd,EPOLL_CTL_ADD,serverfd,&tmpev);
		p = Create_pool(300,10,200);
		while(p->shutdown)
		{
				ready = epoll_wait(epfd,evearr,1,-1);
				if(evearr[0].data.fd == serverfd)
						Producer(p,jobs,(void*)serverfd);
		}
		close(serverfd);
		return 0;
}
