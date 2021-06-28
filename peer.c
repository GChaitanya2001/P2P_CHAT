#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<netdb.h>
#include<netinet/in.h> 
#include<arpa/inet.h> 
#include<errno.h> 
#include<fcntl.h>
#include<sys/stat.h> 
#include<sys/time.h> 
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/select.h>
#include<stdbool.h>
#include<pthread.h>

#define BUFF_SIZE 1024
#define TIMEOUT 600

typedef struct peer
{
	char name[15];
	char ip[15];
	char port[5];
  int flag;
  long prev_conntime;
}peer;

typedef struct msg{
	char name[15];
	char text[BUFF_SIZE];
}msg;

peer *p;
int n;
 
void *timeout(void *param){
  int i;
  while(1)
  {
    for(i=0;i<n; i++)
    {
      if(p[i].flag!= -1){
        if((time(NULL)-p[i].prev_conntime)>TIMEOUT)
        {
            printf("\n%s Timeout. Closing connection ...\n", p[i].name);
            p[i].flag = -1;
        }
      }
    }
  }
  pthread_exit(0);
}

char* get_buff(char* content, char* user)
{
	int len = strlen(content), i=0, p=0, m=0;
	bool done = false;
	char person[15], msg[BUFF_SIZE-16];
	for(i = 0; i<len; i++)
	{
		if(!done)
		{
			if(content[i] == '/')
			{
				    done = true;
            msg[m] = content[i];
            m++;
            continue;
			}
			person[p] = content[i];
			p++;
		}
		else
		{
			msg[m] = content[i];
			m++;
		}
	}
	memcpy(content, user, strlen(user));
	strcat(content, msg);
  int size = (int)strlen(user) + (int)strlen(msg);
  content[size] = '\0';
  return content;
}

void exit_now(void)
{
	printf("\nDo you want to leave?[Y/N] : \t");
	char st;
	scanf("%c", &st);
	if(st == 'Y' || st=='y')
	{
		printf("\nThank you!!\n");
		exit(1);
	}
	return;
}

int main(int argc, char **argv)
{
   int i=0;
   FILE *fptr;
   //Reading the static user_info table
   fptr = fopen("user_info.txt", "r");
   fscanf(fptr, "%d", &n);
   p = (peer*)malloc(n*sizeof(peer));
   while(i<n)
   {
   	  fscanf(fptr, "%s", p[i].name);
   	  fscanf(fptr, "%s", p[i].ip);
   	  fscanf(fptr, "%s", p[i].port);
      printf("User %d: %s, %s, %s\n", i+1, p[i].name, p[i].ip, p[i].port);
      p[i].flag = -1;
      i++;
   }
   fclose(fptr);
   //Defining variables required
   fd_set rd, temp;
   int portno, maxfd, sock_s, sock_c, j, res, res1;
   socklen_t len;
   char buff[BUFF_SIZE + 1], content[BUFF_SIZE], cont[BUFF_SIZE];
   struct timeval tv;
   struct sockaddr_in servaddr, servaddr_2;
   pthread_t tid;

   if(argc != 2)
   {
   	 printf("\n=> CORRECT USAGE: %s, port", argv[0]);
   	 exit(1);
   }

   //converting user port given as arg to integer
   portno = atoi(argv[1]);
   //server sock
   sock_s = socket(AF_INET, SOCK_STREAM, 0);

   //Only port number must be sent as argument
   if(sock_s < 0)
   {
   	 printf("\n Server socket creation FAILED\n");
   	 exit(1);
   }

   int val = 1;
   //setting parameters
   setsockopt(sock_s, SOL_SOCKET, SO_REUSEADDR, (const void*)&val, sizeof(int));
   bzero((char *) &servaddr, sizeof (servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servaddr.sin_port = htons((unsigned short) portno);

   //Binding 
   if(bind(sock_s, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
   {
      printf("\nBinding FAILED\n");
      exit(1);
   }
   //Listening for connections
   if(listen(sock_s, 5) < 0)
   {
   	  printf("\nListen FAILED");
   	  exit(1);
   }
   printf("\nServer Running.....");
   char* user;
   int check,len1;
   check=0;
   //extracting username from the port number
   for(i=0; i<n; i++)
   {
   	  //printf("\n%d", atoi(p[i].port));
      if((!strcmp(p[i].ip, "127.0.0.1")) && (atoi(p[i].port) == portno))
      {
      	printf("\n\033[1;3mHi %s, Welcome to p2p!! Enjoy chating.\033[0m\n", p[i].name);
      	len1 = (int)strlen(p[i].name);
      	user = (char*)malloc((len1 + 1)*sizeof(char));
        memcpy(user, p[i].name, (size_t)(len1));
        user[len1] = '\0';
        check = 1;
        break;
      }
   }
   fflush(stdout);
   //If the user is not there, then exit
   if(!check)
   {
   	  perror("\nYour name is not registered. Please register");
   	  exit(1);
   }
   //Initializes the file descriptor set 'rd' to have zero bits for all file descriptors
   FD_ZERO(&rd);
   //Sets the bit for sock_s in the file descriptor set 'rd'
   FD_SET(sock_s, &rd);
   //Sets the bit for stdin in the file descriptor set 'rd'
   FD_SET(0, &rd);
   maxfd = sock_s;
   //timeout for select
   tv.tv_sec = 3;
   tv.tv_usec = 0;
   bool exit_flag = false;
   // start timeout thread
   pthread_create(&tid,NULL,timeout,NULL);
   int index;
   while(1)
   {
   	 memcpy(&temp, &rd, sizeof(temp));
   	 res = select(maxfd + 1, &temp, NULL, NULL, &tv);
   	 //If timeout occured(as assigned in timeout variable tv)
   	 if(res==0)
   	 {
        if(exit_flag == true) exit_now();
   	 }
   	 //Error in selecting
   	 else if(res < 0 && errno != EINTR)
   	 {
   	 	  printf("\nError while selecting: %s\n", strerror(errno));
   	 }
   	 //If atleast one fd is ready
     else if(res > 0)
     {
     	/*check whether the bit for sock_s is set in the fdset 'temp' 
     	  i.e, check if the user has some connection to accept*/
     	if(FD_ISSET(sock_s, &temp))
     	{
       		len = sizeof(servaddr);
       		sock_c = accept(sock_s, (struct sockaddr*) &servaddr, &len);
          for(i=0;i<n;i++)
          {
                if(!strcmp(p[i].ip,inet_ntoa(servaddr.sin_addr)))
                {
                      index = i;
                      break;
                }
          }
       		if(sock_c < 0)
       		{
       			printf("\nError in accept: %s\n", strerror(errno));
       		}
       		else
       		{
               if(index==-1){
                  printf("Unknown user attempting to connect...denied\n");
                }
                else
                {
                  p[index].prev_conntime = time(NULL);       
                  p[index].flag=1 ;
                  FD_SET(sock_c, &rd);
                  maxfd = (maxfd < sock_c)?sock_c : maxfd;
                } 
       		}
       		FD_CLR(sock_s, &temp);
     	}
        /*check whether the bit for sock_s is set in the 
        fdset 'temp' i.e, when the user is about to send a msg*/
     	if(FD_ISSET(0, &temp))
     	{
         memset(&content[0], 0, sizeof(content));
     		//read the content from stdin
     		if(read(0, content, BUFF_SIZE) < 0)
     		{
     			printf("\nERROR in reading!!");
          exit(1);
     		}
     		//If the msg entered is any of the following then exit from application
     		if(!strcmp(content, "exit\n") || !strcmp(content, "Exit\n") || !strcmp(content, "EXIT\n") || !strcmp(content, "quit\n")|| !strcmp(content, "QUIT\n") || !strcmp(content, "Quit\n"))
     		{
     			exit_flag = true;
     			continue;
     		}
     		int sockfd, portno2, num;
     		struct hostent* server;
     		char *hostname, *hostport;
     		msg m;
     		int size2 = (int)strlen(content);
     		bool done = false;
     		int k, a=0, b=0;
     		/*Get the receiver name and corresponding msg from content 
     		taken from stdin which is in the form of <receiver>/<msg>*/
     		for(k = 0; k<size2; k++)
     		{
       			if(done == false)
       			{
         				if(content[k] == '/')
         				{
         					done = true;
         					continue;
         				}
         				(m.name)[a++] = content[k];
       			}
       			else
       			{
         				if(content[k] == '\0' || content[k] == '\n')
         				{
         					 break;
         				}
         				else
         				{
    	     				(m.text)[b++] = content[k];
         			  }
       			}
     		}
     		m.name[a++] = '\0';
     		m.text[b++] = '\0';
     		bool avail = false;
     		/*Get the name, portno of the receiver accordingly*/
     		for(i = 0; i<n; i++)
     		{
     			if(strcmp(p[i].name, m.name)==0)
     			{
     				int len2 = (int)strlen(p[i].ip);
            hostname = (char*)malloc((len2+1) * sizeof(char));
            memcpy(hostname, (const char*)p[i].ip,(size_t)len2);
            hostname[len2] = '\0';
            len2 = (int)strlen(p[i].port);
            hostport = (char*)malloc((len2+1) * sizeof(char));
            memcpy(hostport, (const char*)p[i].port,(size_t)len2);
            hostport[len2] = '\0';
            portno2 = atoi(hostport);
            p[i].prev_conntime = time(NULL);
            p[i].flag = 1;
            avail = true;
            break;
     			}
     		}
     		//If no such receiver exists, continue
     		if(!avail) continue;
        else
        {
  	     		//send the msg to the receiver
            printf("Sending msg to %s:%d\n", hostname, portno2);
            fflush(stdout);
  	     		sockfd = socket(AF_INET, SOCK_STREAM, 0);
  	     		if(sockfd < 0)
  	     		{
  	     			perror("\nSocket creation FAILED\n");
  	     			exit(1);
  	     		}
  	     		//get the receiver details
  	     		server = gethostbyname(hostname);
  	     		if(server == NULL)
  	     		{
  	     			fprintf(stderr, "\nERROR, No such host with name %s\n", hostname);
  	     			exit(0);
  	     		}
  	     		bzero((char*)&servaddr_2, sizeof(servaddr_2));
  	     		servaddr_2.sin_family = AF_INET;
  	     		bcopy((char*) server -> h_addr, (char*) &servaddr_2.sin_addr.s_addr, server->h_length);
  	     		servaddr_2.sin_port = htons(portno2);
  	     		//Connect to receiver 
  	     		if(connect(sockfd, (struct sockaddr*) &servaddr_2, sizeof(servaddr_2)) < 0)
  	     		{
  	     			perror("\nConnection Failed\n");
  	     			exit(1);
  	     		}
  	     		//get the content to be sent(<sender>/<content>)
  	     		strcpy(cont, get_buff(content, user));
  	     		num = write(sockfd, cont, strlen(cont));
            //printf("\nDone");
  	     		if(num < 0)
  	     		{
  	     			perror("\nERROR!! Socket writing failed....\n");
  	     			exit(1);
  	     		}
  	     		close(sockfd);
  	     		continue;
       	  }
      }

     	//Receive the msgs from other users, from all sockets available
     	for(j = 1; j<= maxfd; j++)
     	{
     		//See if 'j' is available
     		if(FD_ISSET(j, &temp))
     		{
          p[i].prev_conntime = time(NULL);
     			do
     			{
     				res1 = read(j, buff, BUFF_SIZE);
     			}while(res1 == -1 && errno == EINTR);
     			//If something is received
     			if(res1 > 0)
     			{
     				buff[res1] = '\0';
     				msg m1;
		     		int size1 = (int)strlen(buff);
		     		bool done1 = false;
		     		int k1, a1=0, b1=0;
		     		//Get the sender name and content from msg received
		     		for(k1 = 0; k1<size1; k1++)
		     		{
		     			if(done1== false)
		     			{
		     				if(buff[k1] == '/')
		     				{
		     					done1 = true;
		     					continue;
		     				}
		     				m1.name[a1] = buff[k1];
		     				a1++;
		     			}
		     			else
		     			{
		     				if(buff[k1] == '\0' || buff[k1] == '\n')
		     				{
		     					break;
		     				}
		     				else
		     				{
			     				m1.text[b1] = buff[k1];
			     				b1++;
		     			  }
		     			}
		     		}
		     		m1.name[a1++] = '\0';
		     		m1.text[b1++] = '\0';
		     		printf("\n=>%s : %s\n", m1.name, m1.text);
     			}
     			//If empty msg is received
     			else if(res1 == 0)
     			{
            p[i].flag = -1;
     				close(j);
     				FD_CLR(j, &rd);
     			}
     			//Error
     			else
     			{
     				printf("\nError while receiving : %s", strerror(errno));
     			}
     		}
     	}
    }
   } 
   pthread_join(tid,NULL);
   return 0;
}