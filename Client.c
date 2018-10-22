#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/poll.h>
#include<unistd.h>

#define MAXLINE 1024
#define MAX_SOCK 512

char *esc = "exit\n";
int readline(int, char *, int);

struct profile{ 
  char Name[20];
  int len;
} chat_usr;



int main(int argc, char *argv[])
{
  char buf[MAXLINE], line[MAXLINE], sendmsg[MAXLINE];
  char time_buf[257];
  int sd, n, size, nread;
  int pid = (int)getpid();

  struct sockaddr_in server_addr;
  struct pollfd job[2];

  if(argc < 4) {
    printf("Usage: %s [SERVER_ADDRESS] [TCP_PORT] [My_ID]\n",argv[0]);
    exit(1);
  }

  sprintf(chat_usr.Name,"%s", argv[3]);
  chat_usr.len = strlen(chat_usr.Name);

  // sockaddr_in 구조체 정의
  memset((char *)&server_addr,'\0',sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(atoi(argv[2]));
  server_addr.sin_addr.s_addr =  inet_addr(argv[1]);

  // id definition
  char id[chat_usr.len + 4];
  char id_check[20];

  while(1){

    // socket 정의
    if (( sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    { //return sd when success, or -1
      printf("Client : Can't open stream socket.\n");
      exit(1);
    }

    // connect 
    if((connect(sd,(struct sockaddr *)&server_addr,sizeof(server_addr))) == -1)
    {
      printf("Client : Can't connect to server.\n");
      close(sd);
      exit(1);
    } 

    printf("Enrolling ID...\n");

    // id send & check uniqueness.

    sprintf(id,"/id %s",chat_usr.Name);

    //printf("%s\n",id);
    id[strlen(id)] = '\0';
    send(sd,id,strlen(id)+1, 0);

    if(recv(sd,id_check,20,0) < 0) {
      perror("ID_recv : No response from the server");
      close(sd);
      exit(1);
    }
    printf("---id_check: %s\n",id_check);
    if(strncmp(id_check,"yes",3) == 0){
      printf("ID check Success.");
      break;
    }

    close(sd);
    printf("이미 존재하는 ID입니다. 다시 시도하십시오... ");
    scanf("%s", chat_usr.Name);

  } //ID check & connect while end


  // chat function

  // pollfd 구조체에 소켓 지시자를 할당
  job[0].fd = STDIN_FILENO;
  job[0].events = POLLIN;

  job[1].fd = sd;
  job[1].events = POLLIN;

  while(1) {
    nread = poll(job,2,-1); //job 에 대해서 무한정 대기
    if(nread > 0){

      if(job[1].revents == POLLIN){
	
	//Debug
	//printf("Job[1] poll event occur: %d\n",nread);
	//printf("Existing buf: %s\n",buf)
	
	if((n =recv(sd,buf,MAXLINE,0)) < 0){
	  perror("Recv");
	  close(sd);
	  exit(1);
	}

	//printf("Recv n: %d\n",n);
	//printf("buf:  %s\n",buf);

	else if (n == 0){  // 읽어 왔으나 아무것도 없음 --> EOF 가 왔기 때문. 
	  perror("Server is disconnected");
	  close(sd);
	  exit(1);
	}
	else {
	  printf("%s\n",buf);
	}
      }


      if(job[0].revents & POLLIN){
	//printf("Job[0] POLLIN event occur!\n");
	readline(0,sendmsg,MAXLINE);

	size = strlen(sendmsg);
	sendmsg[size] = '\0';
	
	// exit 명령어 확인 
	if(strncmp(esc,sendmsg,4) == 0){
	  printf("Closing the Chat... Goodbye!\n");
	  close(sd);
	  return 0;
	}
	send(sd, sendmsg, strlen(sendmsg) + 1, 0);
      }  
    } // end of if(nread > 0)
  } // end of while	
} // end of main





