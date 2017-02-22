#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <string.h>
#include <time.h>

#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <errno.h>

char* dir_cgroup = "/home/jmjung/cgroup";
char* dir_small = "/home/jmjung/mnt/small";
char* dir_large = "/home/jmjung/mnt/large";

#define SMALL_FILE_SIZE 512*1024*1024
#define LARGE_FILE_SIZE 1024*1024*1024

char path[80];

int g_state = 0;
char* g_err_string;
int nthreads;
typedef enum
{
	NONE,
	READY,
	RUN,
	END,
	ERROR,
} thread_status_t;

thread_status_t thread_status[16] = {0, };

pthread_mutex_t thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t thread_cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t thread_cond2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t thread_cond3 = PTHREAD_COND_INITIALIZER;

void clearState(void)
{
	g_state = NONE;
	g_err_string = "No error";
}
void setState(int state, char* err_str)
{
	printf("%s: %d\n", __func__, state);
	if (g_state != ERROR) {
		g_state = state;
		if (state == ERROR) {
			g_err_string = err_str;
		}
	}
}
void wait_thread_status(int threadn, thread_status_t stat, pthread_cond_t* cond)
{
	int ret = 0;
	int i;
	int wait = 0;
	return;
	ret = pthread_mutex_lock(&thread_lock);
	if (ret<0) {
		perror("pthread_mutex_lock failed");
		setState(ERROR, "pthread_mutex_lock_failed");
		return;
	}
	while (1) {
		if (threadn<0) {
			for (i=0; i<nthreads; i++) {
				if (thread_status[i] != stat) {
					wait = 1;
				}
			}
			if (wait)
				wait = 0;
			else
				break;
		} else {
			if (thread_status[threadn] == stat)
				break;
		}
		ret = pthread_cond_wait(cond, &thread_lock);
		if (ret<0) {
			perror("pthread_cond_wait failed");
			setState(ERROR, "pthread_cond_wait_failed");
			return;
		}
	}
	ret = pthread_mutex_unlock(&thread_lock);
	if (ret<0) {
		perror("pthread_mutex_unlock failed");
		setState(ERROR, "thread_mutex_unlock_failed");
		return;
	}
	return;
}

void signal_thread_status(int threadn, thread_status_t stat, pthread_cond_t* cond)
{
	int ret = 0;
	int i;
	return;
	ret = pthread_mutex_lock(&thread_lock);
	if (ret < 0) {
		perror("pthread_mutex_lock failed");
		setState(ERROR, "pthread_mutex_lock failed");
		return;
	}

	if (threadn < 0) {
		for (i=0; i<nthreads; i++) {
			thread_status[i] = stat;
		}
		ret = pthread_cond_broadcast(cond);
		if (ret < 0) {
			perror("pthread_cond_broadcast failed");
			setState(ERROR, "pthread_cond_broadcast failed");
			return;
		}
	} else {
		thread_status[threadn] = stat;
		ret = pthread_cond_signal(cond);
		if (ret < 0) {
			perror("pthread_cond_signal failed");
			setState(ERROR, "pthread_cond_signal failed");
			return;
		}
	}
	ret = pthread_mutex_unlock(&thread_lock);
	if (ret < 0) {
		perror("pthread_mutex_unlock failed");
		setState(ERROR, "pthread_mutex_unlock failed");
		return;
	}
	return;
}

void *run_thread(void*);
void lib_mkdir(char* path)
{
	//error handle
	int ret = mkdir(path, 0777);
	if (ret == -1)
		perror("fail: mkdir");
	else	
		printf("directoy exists\n");

}
void env_setup()
{
	int i;
	int ret;
	int ngroup = 4;
	//create file
	lib_mkdir(dir_cgroup);
	for (i=0; i<ngroup; i++) {
		sprintf(path, "%s/%s%d", dir_cgroup, "group", i);
		lib_mkdir(path);
		printf("%s/%s%d\n", dir_cgroup, "group", i);
	}
#ifdef INIT_FILE
	lib_mkdir(dir_small);
	lib_mkdir(dir_large);

	ret = mount("none", dir_small, "tmpfs", " ");
	if (ret == -1)
		perror("");
#endif
	/*
	mount();
	system("mount -t tmpfs small");
	mount();

	system("mkdir small/g1");
	system("mkdir small/g2");
	system("mkdir small/g3");
	system("mkdir small/g4");
	create();
*/

}
void env_reset()
{
	//umount
	rmdir("small");
	rmdir("large");
	//free
	umount(dir_small);
}


int main(int argc, char* argv[]) {
	int i,j;
	int ret;
	int status;

	pthread_t* threads;
	int* done;


	int nthread;

	if (argc == 1) {
		printf("Default setting\n");
		nthreads = 4;
	}


	//env_setup();

	threads = (pthread_t*)malloc(sizeof(pthread_t)*nthreads);
	//thread_status = (thread_status_t*)malloc(sizeof(thread_status_t)*nthreads);
	done = (int*)malloc(sizeof(int)*nthreads);
	

	for (i=0; i<nthreads; i++)	{
		done[i]=0;
		ret=pthread_create(&threads[i], NULL, &run_thread, (void*)i);
		if(ret<0) {
			perror("pthread_create failed");			
		}
		//printf("%d, %d\n", i, threads[i]);
	}
	setState(READY, NULL);
	wait_thread_status(-1, READY, &thread_cond1);
	setState(RUN, NULL);	
	//printf("Let's start\n");
	signal_thread_status(-1, RUN, &thread_cond2);
	wait_thread_status(-1, END, &thread_cond3);
	//What???
	i=0;
	for (j=0; j<nthreads; j++){
		
		//printf("%d\n", i);
		//printf("%d\n", j);
		//i++;
	//done[i] = 1;
		ret = pthread_join(threads[j], (void**)&status);
		if (ret == 0) {
			printf("Completed join with thread %d status = %d\n", j, status);
		}
		else {
			printf("ERROR: return code from pthread_join() is %d, thread %d\n", ret, j);
		}
	
		
	}
	setState(END, NULL);
	free(threads);
	//free(thread_status);
	free(done);
	return 0;
	
}
void init_cgroup()
{
}
void cgroup_attach(int pid, int id)
{
	char cmd[80];
	char path[40] = "/home/jmjung/cgroup/group/g";
	if (id<0)
		sprintf(cmd, "echo %d > /home/jmjung/cgroup/group/tasks", pid);
	else
		sprintf(cmd, "echo %d > %s%d/tasks", pid, path, id);
	system(cmd);
}
#define LARGE_CHUNK 4096*64
void access_file(void *addr, int len, int type, int iter, int arg) 
{
	int i,j;
	int fd;
	char* base = (char*)addr;
	char* buf;
	time_t start_time, current_time, expire_time;
	expire_time = 60;
	unsigned long count = 0;
	//return;
	//type = 0;
	if (type == 0x1) {
		//sprintf(path, "/home/jmjung/large%d",arg);
		//fd = open(path, O_CREAT|O_RDWR);
		//ftruncate(fd, len);
		//buf=malloc(LARGE_CHUNK);
		//memset(buf, 0, LARGE_CHUNK);
		start_time = time(NULL);
		while (1) {
			current_time = time(NULL);
			if(current_time - start_time >= expire_time)
				break;

		//}
		//for (i=0; i<iter; i++) {
			for (j=0; j<len; j=j+4096) {
				current_time = time(NULL);
				if(current_time - start_time >= expire_time)
					break;
				base[j] = j;
				count++;
			}
			/*
			for (j=0; j<len-2*LARGE_CHUNK; j=j+LARGE_CHUNK) {
				current_time = time(NULL);
				if(current_time - start_time >= expire_time)
					break;

				lseek(fd, j, SEEK_SET);
				write(fd, buf, LARGE_CHUNK);
				//lseek(fd, j+LARGE_CHUNK, LARGE_CHUNK);
				read(fd, buf, LARGE_CHUNK);
				count=count+LARGE_CHUNK/4096;
			}*/
		//}
		}
		//free(buf);
		
	}
	/*
		for (i=0; i<iter; i++) {
			for (j=0; j<len; j=j+4096) {
				base[j] = j;
			}
		}
	}
	*/
	else if (type == 0x0) {
		start_time = time(NULL);
		//iter=iter*1024*1024;
		while(1) {
			current_time = time(NULL);
			if(current_time - start_time >= expire_time)
				break;


		//for (i=0; i<iter; i++) {
			for (j=0; j<len; j=j+(4096)) {
				current_time = time(NULL);
				if(current_time - start_time >= expire_time)
					break;


				base[rand()%len] = j;
				count++;
			}
		//}
		}
	}
	if (type == 0)
		printf("Small Page Count %d\n", count);
	else
		printf("Large Page Count %d\n", count);
}
void run_work(int arg)
{
	int len;
	int fd;
	pid_t pid = getpid();
	void* addr;
	printf("%s arg = %d pid = %d\n", __func__, arg, pid);
	
	//cgroup_create();

	//cgroup_attach(pid, "");
	if(arg & 0x1) {
		len = LARGE_FILE_SIZE;
		//cgroup_attach(pid, (arg/2)+1);

		cgroup_attach(pid, 4);
		//cgroup_attach(pid, -1);
		addr = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
		//addr = 1;

	} else {
		len = SMALL_FILE_SIZE;
		cgroup_attach(pid, (arg/2)+1);
		//cgroup_attach(pid, -1);
		//cgroup_attach(pid, -1);
		
		addr = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
	}
	if(!addr) {
		printf("mmap error\n");
		return;
	}
	printf("Access Files\n");
	access_file(addr, len, arg & 0x1, 100, arg);
	//populate


	munmap(addr, len);
//	if (arg&0x1)
//		printf("Large File Work end\n");
//	else
//		printf("Small File Work end\n");

}
void run_process(int arg)
{
	pid_t pid;
	int status;
	int ret;
		
	pid=fork();
	if (pid > 0) {
		
		//parent process
		printf("waiting child in pid %d\n", pid);
		ret = waitpid(pid, &status, NULL);
		if(ret < 0) {
			perror("waidpid failed");
		}
	}
	else if (pid == 0) {
		//child process
		run_work(arg);
		exit(0);
	}
	else {
		perror("fork error : ");
		exit(1);
	}
}
void *run_thread(void* arg)
{
	int i;
	printf("thread: %d, %d\n", (int)arg, getpid());

	signal_thread_status(-1, READY, &thread_cond1);
	printf("wait\n");
	wait_thread_status(-1, RUN, &thread_cond2);
	run_process((int)arg);
	//while (!done[(int)arg]) {
	//	for (i=0; i<10000; i++) {
			
	//	}
	//	printf("thread: %d, result = %e\n", (int)arg, result);
	//}
	signal_thread_status(-1, END, &thread_cond3);
	pthread_exit((void*)0);				
}

