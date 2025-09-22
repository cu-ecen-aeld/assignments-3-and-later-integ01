/*
** server.c -- a stream socket server demo
*/

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
#include <signal.h>

#include <syslog.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#define PORT "9000"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connections queue will hold
#define MAX_BUFFER 64000
#define MAX_REQUESTS 20

const char *FILE_NAME = "file_out.txt";
volatile int child_pids[MAX_REQUESTS] = {0};

volatile int child_end_flag = 0;
volatile int parent_end_flag = 0;

void sigchld_handler(int s)
{
    (void)s; // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    int pid = 0;
    printf("Parent SigChld: \n");
    while((pid = waitpid(-1, NULL, WNOHANG)) > 0)
    {
    	printf("Parent SigChld: got end of pid:%d\n", pid);
	int index = 0;

	while (child_pids[index]!=pid && (index < MAX_REQUESTS))
	{// printf("%d: %d, ", index, child_pids[index]);
		index++; }
	if ( index < MAX_REQUESTS ) {
		printf("Parent SigChld, clearing pid: %d at index:%d\n",child_pids[index], index);
		child_pids[index] = 0;
	}

    }
    int index = 0;
    if (parent_end_flag) {
        while (child_pids[index]== 0 && (index < MAX_REQUESTS))
	{ index++; }
    	if ( index == MAX_REQUESTS ) {
	    printf("Parent SigChld: Exiting \n");
	    int status = remove(FILE_NAME);
	    if (status == 0) {
        	printf("File '%s' deleted successfully.\n", FILE_NAME);
	    } else {
        	perror("Error deleting file"); // Prints an error message based on errno
	    }
	    _exit(0);
	}
    }

    errno = saved_errno;
}


void child_handler(int signo)
{
    printf("Child sigUser1 : Received signal\n");
    if (signo == SIGUSR1)
	child_end_flag = 1;

//    (void)signo;
}

void parent_handler(int signo)
{
    printf("Parent :%d sigint or sigterm : Received signal:%d\n", getpid(), signo);
    syslog(LOG_INFO, "Caught signal, exiting");
    if ( (signo == SIGINT) || (signo == SIGTERM) )
	parent_end_flag = 1;
    	printf("Set end flag \n");

	for (int i = 0; i< MAX_REQUESTS; i++)
	{
		//printf("Search pids to kill %d:%d ", i, child_pids[i]);
		if (child_pids[i] > 0)
		{
			printf("Sending kill SIGUSR1 to child:%d \n", child_pids[i]);
			kill(child_pids[i],SIGUSR1);
		}
	
	}

	printf("Parent SigChld: Check for last child and close up\n");
	int index = 0;
        while (child_pids[index]== 0 && (index < MAX_REQUESTS))
	{ index++; }
    	if ( index == MAX_REQUESTS ) {
	    int status = remove(FILE_NAME);
	    if (status == 0) {
        	printf("File '%s' deleted successfully.\n", FILE_NAME);
	    } else {
        	perror("Error deleting file"); // Prints an error message based on errno
	    }

	    printf("Parent SigTerm: Exiting \n");

	    _exit(0);
	}

//    (void)signo;
}

void parent_term_handler(int signo)
{

    printf("Parent:%d  sigterm : Received signal:%d\n", getpid(), signo);
    parent_handler(signo); 
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int lock_file_writer(struct flock *lock, int wfd) 
{

    long offset = lseek(wfd, 0, SEEK_END);

    lock->l_type = F_WRLCK;
    lock->l_whence = SEEK_SET;
    lock->l_start = offset+1;
    lock->l_len = 0;
    if (fcntl(wfd, F_SETLKW, lock) == -1) {
	perror("Error to get Wr lock");
        close(wfd);
        return errno;
    }
    return 0;
}

int lock_file_reader(struct flock *lock, int wfd, long offset, long len) 
{

    lock->l_type = F_RDLCK;
    lock->l_whence = SEEK_SET;
    lock->l_start = offset;
    lock->l_len = len;
    if (fcntl(wfd, F_SETLKW, lock) == -1) {
	perror("Error to get Rd lock");
        close(wfd);
        return errno;
    }
    return 0;
}

int unlock_file(struct flock *lock, int wfd) 
{	
	lock->l_type = F_UNLCK;
	if (fcntl(wfd, F_SETLK, lock) == -1)
	{ perror("Lock error"); 
	  /* handle error */
	       return -1;	
	}
	return 0;
}

long receive_save_messages2(int sock_fd, char *buf, int outfd, long offset )
{
	int flags = 0;
	ssize_t len = 0;
	static char *line_read = (char *)NULL;
	printf("server: receiving client messages\n");
	char *curLine;
	long count = 0;
	long wpos = offset;
	long rpos = offset;
	long block_pos = offset;
	char read_buf[MAX_BUFFER];
	int res = 0;
	int final_res = 0;
	struct flock lock;
	//TODO - open a read descriptor to the file
	// and send message from file 
	
	int start_pack = 1;
	curLine = buf;
	do {
		len = recv(sock_fd, (void *)buf, MAX_BUFFER, 0); //MSG_WAITALL);
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
		for (char * nextLine = strchr(curLine, '\n') ; len > 0; nextLine = strchr(curLine, '\n') )
		{
		// Block until writer lock available
		  if (start_pack) {
			res = lock_file_writer(&lock, outfd);
			if (res != 0) {
				perror("File lock error");
				exit(0);
			}
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
		  else 
		  {
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
			  if (!start_pack)
			  {
				res = unlock_file(&lock, outfd);
				if (res == -1)
					return -1;
				start_pack=1;
			  }

 
	 		 // Check if package is not in current buffer block 

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
					  //while (rpos < offset) 
			  int rlen = 1;
			  printf("send line back\n"	);
			  res = lock_file_reader(&lock, outfd, rpos, wpos-rpos);
	                  if (res != 0) {
			     perror("File Read lock error");
			     exit(0);
		          }

			  while (rpos < wpos) // || rlen > 0) 
			  {
				//read_file_offset_and_send(outfd, rpos, 
				if (1) { //(rpos < offset) {
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

				  if (-1 == send(sock_fd, read_buf, rlen, 0) ) 
				  {
                			perror("server: send");
					final_res = -1;
					break;
				  }
				  rpos += rlen;
				}
			  }
  			  res = unlock_file(&lock, outfd);
			  if (res == -1)
				return -1;

			  //if (-1 == send(sock_fd, curLine, msgLen, 0) ) 
			  //{
                	//	perror("server: send");
		//		return -1;
		//	  }

			 // rpos += msgLen;
			 // printf("wpos:%d, rpos: %d\n",(int)wpos, (int)rpos);
		 	 curLine = nextLine+1;
		  	} // end of else 
		} //end of for
		
	} while (!child_end_flag);
	 if (!start_pack)
	  {
		lock.l_type = F_UNLCK;
		if (fcntl(outfd, F_SETLK, &lock) == -1)
		{ perror("Lock Release Error"); 
		  /* handle error */ 
		}
	  }

	return final_res;
}

//int main(void)
int main(int argc, char *argv[])
{
    // listen on sock_fd, new connection on new_fd
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
//    FILE* fptr;
//
    bool daemon_mode = false;
    int opt;
    
    while ((opt = getopt(argc, argv, "d")) != -1) {
        switch (opt) {
        case 'd': daemon_mode = true; break;
        default:
        }
    }
    //printf("Running Server, daemon mode:%d",daemon_mode); 
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

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    // Opening the file in read mode
    //fptr = fopen("filename.txt", "w+");
    //int out_wfd = fileno(fptr);
        //fptr2 = fopen("filename.txt", "r");
	//int out_rfd = fileno(fptr2;
    out_wfd = open(FILE_NAME, O_RDWR | O_CREAT, 0666);
    if (out_wfd == -1)
        return errno;


    printf("server: waiting for connections...\n");
    long offset = 0;
    while(!parent_end_flag) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
            &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);
	syslog(LOG_INFO, "server: Accepted connection from %s\n",s);

	int index = 0;
	while (child_pids[index]!=0 && (index < MAX_REQUESTS))
       	    { index++; }
	if ( index == MAX_REQUESTS ) {
		printf("Error - Reached max number of requests ignoring this packet\n");
		continue;
	}
	child_pids[index] =fork();
        if (child_pids[index]== 0) 
	{ // this is the child process
            close(sockfd); // child doesn't need the listener
	    printf("Child start - allocation buffer\n");
    	    void *recv_buf = malloc (MAX_BUFFER);
            signal(SIGUSR1,child_handler);
            int fd;
            ssize_t nbyte;


	    if (NULL == recv_buf)
	    {
		perror("server: malloc");
	    }

	    int res = receive_save_messages2(new_fd, (char *)recv_buf, out_wfd,0);
	    printf("child pid:%d, receive exit status: %d\n",getpid(), res);
           // if (send(new_fd, "Hello, world!", 13, 0) == -1)
           //     perror("send");
           // close(new_fd);
	   //
	    free(recv_buf);
            exit(0);
        }
	else {  // Parent section
	    	
	    signal(SIGINT,parent_handler);
	    signal(SIGTERM, parent_term_handler);

	    close(new_fd);  // parent doesn't need this
	    //sleep(1);
	    if (parent_end_flag) {
		    printf(">>Got Parent end flag\n");
	    for (int i = 0; i< MAX_REQUESTS; i++)
	    {
		printf("Search pids to kill %d:%d ", i, child_pids[i]);
		if (child_pids[i] > 0)
		{
			printf("Sending kill SIGUSR1 to child:%d \n", child_pids[i]);
			kill(child_pids[i],SIGUSR1);
			child_pids[i]=0;
		}
	
	    }
	    }

	}  //end of paernt section
    }

    return 0;
}


