


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/select.h>
#include <signal.h>

#include <syslog.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include "queue.h" // queue taken from FreeBSD 10
#include <pthread.h>
#include <time.h>
#include <math.h>


#define PORT "9000"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connections queue will hold
#define MAX_BUFFER 64000
#define MAX_REQUESTS 20
#define TS_PERIOD 10
//const char *FILE_NAME = "file_out.txt";
const char *FILE_NAME = "/var/tmp/aesdsocketdata";
//volatile int child_pids[MAX_REQUESTS] = {0};

//volatile int child_end_flag = 0;
volatile int parent_end_flag = 0;
volatile int child_end_signal = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* threadfunc(void* thread_param);
/**
 * This structure should be dynamically allocated and passed as
 * an argument to your thread using pthread_create.
 * It should be returned by your thread so it can be freed by
 * the joiner thread.
 */
struct thread_data{
    /*
     * TODO: add other values your thread will need to manage
     * into this structure, use this structure to communicate
     * between the start_thread_obtaining_mutex function and
     * your thread implementation.
     */
    pthread_t tid;
    void (*callback)(void *); 
    int wait_to_obtain_ms;
    int wait_to_release_ms;
    pthread_mutex_t *lock;
    void *recv_buf;
    int sockfd;
    int out_wfd;
    int child_end_flag;
    /**
     * Set to true if the thread completed with success, false
     * if an error occurred.
     */
    bool thread_done;
    bool thread_complete_success;
} thread_data_cl;

typedef struct node
{
    struct thread_data *thread_data;
    TAILQ_ENTRY(node) nodes;
} node_t;



// This typedef creates a head_t that makes it easy for us to pass pointers to
// head_t without the compiler complaining.
typedef TAILQ_HEAD(head_s, node) head_t;

//head_t **phead;


//TAILQ_INIT(&q_head); // initialize the head

void callback_child_end_signal(void *thread_raw) {
	
    child_end_signal = 1;
}

static int scan_join_complete_threads(head_t *head)
{

  struct node * e = NULL;
    struct node * next = NULL;


   printf("Got child end signal - Doing a scan join complete thread\n");
   TAILQ_FOREACH_SAFE(e, (head), nodes, next)
   {	
	struct thread_data *tdata =  e->thread_data;
	if (NULL == tdata)
	{
		printf("scan join thread data null error\n");
		return -1;
	}
	if (tdata->thread_done)
	{
		printf("Doing join on thread: %lu\n",tdata->tid);
   		pthread_join(tdata->tid, NULL);
		free(tdata->recv_buf);
		TAILQ_REMOVE(head, e, nodes);
		free(e);
		e = NULL;
    	}
   }

}

/******* Signal handlers **********/
#if 1
void parent_handler(int signo)
{
    printf("Parent :%d sigint or sigterm : Received signal:%d\n", getpid(), signo);
    syslog(LOG_INFO, "Caught signal, exiting");
    if ( (signo == SIGINT) || (signo == SIGTERM) )
	parent_end_flag = 1;
    printf("Set end flag for all threads\n");
	// Set child flags to end
	//
    printf("Parent SigTerm: Exiting \n");
	// _exit(0);
}

void parent_term_handler(int signo)
{

    printf("Parent:%d  sigterm : Received signal:%d\n", getpid(), signo);
    parent_handler(signo); 
}


struct timeval get_next_timer_timeout(double time_taken) {
    struct timeval tv;

    tv.tv_sec = TS_PERIOD - round(time_taken) > 0 ? TS_PERIOD - (int)round(time_taken):0;
// Wait 10 seconds max
    tv.tv_usec = 0;
    return tv;
}
#endif
//head_t q_head;

static int insert_queue(head_t *head, struct thread_data *thrd)
{

	struct node * e = malloc(sizeof(struct node));
	if (e == NULL)
	{
            fprintf(stderr, "malloc failed");
            exit(EXIT_FAILURE);
        }
        e->thread_data = thrd;
        TAILQ_INSERT_TAIL(head, e, nodes);
        e = NULL;

}
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

bool start_thread_obtaining_mutex(struct thread_data **thrd_data, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms, int sockfd, int outfd)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    struct thread_data *params = malloc(sizeof(struct thread_data));
    if (NULL == params || NULL == mutex ) 
    {
	    return false;
    }
    params->lock = mutex;
    params->wait_to_obtain_ms = wait_to_obtain_ms;
    params->wait_to_release_ms = wait_to_release_ms;
    params->thread_complete_success = false;
    params->thread_done = false;
    params->recv_buf = malloc(MAX_BUFFER);
    params->sockfd = sockfd;
    params->out_wfd = outfd;
    params->child_end_flag = 0;
    params->callback = callback_child_end_signal;

	
    if (NULL == params->recv_buf)
    {
	perror("thread buffer malloc");
	free(params);
	return false;
    }
    int rc = pthread_create(&(params->tid), NULL, threadfunc, (void *)params); 
    if (rc != 0) {
            printf("\nThread can't be created :[%s]", strerror(rc)); 
	    free(params->recv_buf);
	    free(params);
    	    return false;
    }
//    *thread = params->tid; 
    *thrd_data = params;
    printf("Thread id:%lu created succesfully\n", (pthread_t)(params->tid));

    return true;
    
}
bool send_message(int sockfd, int outfd2)
{

	char read_buf[MAX_BUFFER];
	long rpos = 0;
	int rlen = 1;
	printf("send line back\n");
	int eof = 0;
	FILE *file;
	bool final_res = 0;
	file = fopen(FILE_NAME, "rb");
        if (file == NULL) {
        	perror("Error opening file");
        	return EXIT_FAILURE;
	}
	int outfd = fileno(file);
/*
        while (fgets(read_buf, MAX_BUFFER, file) != NULL) {
        // Print the line (fgets retains the newline character '\n' if it fits)
	  printf("Send line from file'%s'\n", read_buf);
	  if (-1 == send(sockfd, read_buf, rlen, 0) ) 
	  {
               	perror("server: send");
		final_res = -1;
		break;
	  }

	}
*/

	while (eof == 0) // || rlen > 0) 
	{
		   //read_file_offset_and_send(outfd, rpos, 
	  int size = MAX_BUFFER;
	  rlen = read(outfd, read_buf, size);
	  if (-1 == rlen)
	  {
		  perror("server: read file messages error" );
		  final_res = -1;
		  break;
	  }
	  else if (rlen == 0) {
		  eof = 1;
		  break;
	  }
	  else if (size != rlen)
	  {
		printf("read smaller chunk :%d, from line size:%d\n",rlen, size);
		eof = 1;
	  }
		  //read_buf[rlen-1]=10;
	  printf("Send line from file'%s'\n", read_buf);
	  if (-1 == send(sockfd, read_buf, rlen, 0) ) 
	  {
               	perror("server: send");
		final_res = -1;
		break;
	  }
		  rpos += rlen;
	}  // End of file read and send
	   
	return final_res;

}

int write_with_timestamp(int outfd)
{

    time_t t;
    struct tm *tmp;
    char timestr[200];
    
    t = time(NULL);
    tmp = localtime(&t);
    if (tmp == NULL) {
       perror("localtime");
       exit(EXIT_FAILURE);
    }

    int tslen = strftime(timestr, sizeof(timestr),"timestamp:%a, %d %b %Y %T %z\n", tmp);

    if (tslen == 0) {
               fprintf(stderr, "strftime returned 0");
               exit(EXIT_FAILURE);
    }

    int tslen2 = write( outfd, timestr, tslen);
    return tslen2;

}
bool receive_save_message(int sockfd, char *buf, int outfd )
{

    ssize_t len = 0;
    static char *line_read = (char *)NULL;
    printf("server: receiving client messages\n");
    char *curLine;
    long count = 0;
    int res = 0;
    int final_res = 0;
    long wpos = 0; //offset;
    long rpos = 0; //offset;

    
    int start_pack = 1;
    int end_pack = 0;

    curLine = buf;
    do 
    {
    len = recv(sockfd, (void *)buf, MAX_BUFFER, 0);


    printf ("server, recived msg len: %d\n",(int)len);
    if (len == -1)
    {
	  final_res = -1;	
	  break;
    }

    if (len == 0)
    {
	final_res = 0;
	break;
    }

    for (char * nextLine = strchr(curLine, '\n') ; len > 0 && end_pack==0; nextLine = strchr(curLine, '\n') )
    {
	if (start_pack) {
	// Seek to the end of the file
		wpos = lseek(outfd, 0, SEEK_END);
		rpos = 0; //wpos; 
		start_pack = 0;
	 }

	 if ((nextLine == NULL) && (len >0))
	 {
		int olen = write( outfd, curLine, len); 
		if (olen != len)
		{
		  perror("server: write messages to log error" );
		  return -1;
		}
		wpos += olen;
		len -= olen;
	 }
	 else {

    		end_pack = 1;
		int msgLen = nextLine -curLine+1;
		printf("msgLen: %d\n", msgLen);
		char temp = curLine[msgLen-1];
		assert (temp == '\n' );
		  //curLine[msgLen-1]=0;
	   	printf("Got curLine='%s'\n", curLine);
		//curLine[msgLen-1]=temp;
		int olen = write( outfd, curLine, msgLen); 
 		if (olen != msgLen)
		{
		    perror("server: write messages to log error" );
		    return -1;
		}
		wpos += msgLen;
		len -= msgLen;
		printf("Complete packet receive - return to thread main");
		return true;
	/*
	        printf("wpos:%d, rpos: %d\n",(int)wpos, (int)rpos);
		if (rpos != wpos ) {
		   printf("server: seek rpos: %d\n",(int)rpos);
		   res = lseek(outfd, rpos, SEEK_SET);
		   if ( -1 == res)
		   {
			perror("server: seek error");
			return -1;
		   }

		}
		*/
		 //while (rpos < offset) 
/*	
		int rlen = 1;
		printf("send line back\n"	);
		while (rpos < wpos) // || rlen > 0) 
		{
		   //read_file_offset_and_send(outfd, rpos, 
		  int size = (wpos - rpos) > MAX_BUFFER ? MAX_BUFFER: wpos - rpos;
		  rlen = read(outfd, read_buf, size);
		  if (-1 == rlen)
		  {
			  perror("server: read file messages error" );
			  final_res = -1;
			  break;
		  }
		  if (size != rlen)
		  {
			printf("read smaller chunk :%d, from line size:%d\n",rlen, size);
		  }
		  //read_buf[rlen-1]=10;
		  printf("Send line from file'%s'\n", read_buf);
		  if (-1 == send(sockfd, read_buf, rlen, 0) ) 
		  {
                	perror("server: send");
			final_res = -1;
			break;
		  }
		  rpos += rlen;
		} // end of while rpos < wpos
*/
	 } //end of else (eol reading

		curLine = nextLine+1;
    } // end of for loop full packet until eol and resend(\n)
  } while (end_pack != 1);

  return 0;

}

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    //

    struct thread_data* thread_args = (struct thread_data *) thread_param;
    //printf("thread:%d sleep start :%d \n",(int)thread_func_args->tid,thread_func_args->wait_to_obtain_ms );
    usleep(thread_args->wait_to_obtain_ms*1000);
    //printf("thread:%d wakeup and trying to get mutex \n",(int)thread_func_args->tid);

    do {
	if (0 != pthread_mutex_lock(thread_args->lock))
	{
	   return 0;
	}
	/********* get packet from client *************/
	printf("thread:%lu wakeup and got mutex \n",thread_args->tid);


	bool res = receive_save_message(thread_args->sockfd, thread_args->recv_buf, thread_args->out_wfd );
	if (!res) 
	{
    		thread_args->thread_done = true;
    		thread_args->callback(thread_param);
		perror("thread receive_save_message");
		return 0;
	}

    /********* end of get packet from client *************/
    	usleep(thread_args->wait_to_release_ms*1000);
    	if (0 != pthread_mutex_unlock(thread_args->lock))
    	{
    	    thread_args->thread_done = true;
    	    thread_args->callback(thread_param);
	    return 0;
    	}
        printf("thread:%lu release mutex \n",thread_args->tid);
    //thread_func_args->callback(thread_param);
    //free(thread_param);
        printf("Trying to send message<<\n"); 
	res = send_message(thread_args->sockfd, thread_args->out_wfd );
	if (!res) 
	{
    	    	thread_args->thread_done = true;
    		thread_args->callback(thread_param);
		perror("thread send_message");
		return 0;
	}


    } while (!thread_args->child_end_flag); // end of main loop
    thread_args->thread_done = true;
    thread_args->thread_complete_success = true; 
    thread_args->callback(thread_param);
    return thread_param;
}



int main(int argc, char *argv[])
{
    // listen on sockfd, new connection on new_fd
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address info
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    int out_wfd;
    int pid_daemon;

    bool daemon_mode = false;
    int opt;
    static bool first_loop = false;

    signal(SIGINT,parent_handler);
    signal(SIGTERM, parent_term_handler);


    printf("Trying to init queue");
    head_t q_head;
    TAILQ_INIT(&q_head); // initialize the head

    //*phead = &(q_head);
    
    while ((opt = getopt(argc, argv, "d")) != -1) {
        switch (opt) {
        case 'd': daemon_mode = true; break;
        default:
        }
    }
    printf("Running Server, daemon mode:%d",daemon_mode); 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        //fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        syslog(LOG_ERR, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
//            perror("server: socket");
//            continue;
	      syslog(LOG_ERR, "server: socket error");
  	      return -1;	      
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            //perror("setsockopt");
            //exit(1);
	    syslog(LOG_ERR, "server: setsockopt error");
  	    return -1;	      
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            //perror("server: bind");
            //continue;
	    syslog(LOG_ERR, "server: bind error");
	    return -1;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
	
    if (daemon_mode) {
	pid_daemon = fork();
	if (pid_daemon < 0) {
	        // Handle error
        	exit(EXIT_FAILURE);
    	}
	if (pid_daemon > 0) {
	        // Parent process, exit
        	exit(EXIT_SUCCESS);
    	}
    }
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

//    sa.sa_handler = sigchld_handler; // reap all dead processes
//    sigemptyset(&sa.sa_mask);
//    sa.sa_flags = SA_RESTART;
//    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
//        perror("sigaction");
//        exit(1);
//    }
    // Opening the file in read mode
    //fptr = fopen("filename.txt", "w+");
    //int out_wfd = fileno(fptr);
        //fptr2 = fopen("filename.txt", "r");
	//int out_rfd = fileno(fptr2;
    out_wfd = open(FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out_wfd == -1)
        return errno;


    printf("server: waiting for connections...\n");
    long offset = 0;
    struct timespec start, end;
    double time_taken  = 5;

    while(!parent_end_flag) {  // main accept() loop
	fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);		       //

	 // 1. Calculate the timeout dynamically for this iteration
        struct timeval timeout = get_next_timer_timeout(time_taken);
	printf("Setting timeout to:%d , last loop time:%f\n",(int)timeout.tv_sec, time_taken);
   	 // Get starting timestamp
        timespec_get(&start, TIME_UTC);



        // 2. Call select
        int activity = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

	if (activity < 0) {
            perror("select error");
            break;
        } else if (activity == 0) {
            printf("Timer expired! Generate time stamp.\n");
	    //TODO
	    //
	    //
	    if (0 != pthread_mutex_lock(&lock))
	    {
	   	return 0;
	    }
	    //if (!first_loop)
	    write_with_timestamp(out_wfd); 
	    first_loop = true;
	    if (0 != pthread_mutex_unlock(&lock))
	    {
	   	return 0;
	    }
	    if (child_end_signal) {
		    child_end_signal = 0;
	            scan_join_complete_threads(&q_head);
	    }
	    else
		    time_taken = 0;
            //continue;
        }

        // Check if the listening socket has a pending connection
	if (FD_ISSET(sockfd, &read_fds)) 
	{
  	      sin_size = sizeof their_addr;
	      new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
	            &sin_size);
              if (new_fd == -1) {
      		      perror("accept");
	            continue;
        	}

	        inet_ntop(their_addr.ss_family,
        	    get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
	        printf("server: got connection from %s\n", s);
		syslog(LOG_INFO, "server: Accepted connection from %s\n",s);
		int index = 0;
		struct thread_data *thrd_data = NULL;
		int wait_to_obtain_ms = 20;
		int wait_to_release_ms = 20;
		bool res = start_thread_obtaining_mutex(&thrd_data, &lock,
			wait_to_obtain_ms, wait_to_release_ms, new_fd, out_wfd);

		if (!res || thrd_data == NULL)
		{
    			perror("Thread create ");
	        	exit; // TODO - add gracefull exit
	    	}
	
		insert_queue(&q_head, thrd_data); 
		timespec_get(&end, TIME_UTC);
		time_taken = (end.tv_sec - start.tv_sec) + 
                       (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    	}
    } //End of main while loop 
      
   /****** Clean up ******/
    close(sockfd);
    struct node * e = NULL;
    struct node * next = NULL;
    head_t *phead = &q_head;
    //TAILQ_FOREACH(e, (*phead), nodes)
    TAILQ_FOREACH(e, phead, nodes)
    {
       //printf("%c", e->c);
    	struct thread_data *tdata =  e->thread_data;
	tdata->child_end_flag = 1;
    }
    printf("Main cleaning up : wait all thread to end and close up\n");
	//TODO
	
    e = NULL;
    next = NULL;

    TAILQ_FOREACH_SAFE(e, phead, nodes, next)
    {
           //printf("%c", e->c);
    	struct thread_data *tdata =  e->thread_data;
	pthread_join(tdata->tid, NULL);
	free(tdata->recv_buf);
	TAILQ_REMOVE(phead, e, nodes);
	free(e);
	e = NULL;
    }

    int status = remove(FILE_NAME);
    if (status == 0) {
       	printf("File '%s' deleted successfully.\n", FILE_NAME);
    } else {
       	perror("Error deleting file"); // Prints an error message based on errno
    }




}

	


