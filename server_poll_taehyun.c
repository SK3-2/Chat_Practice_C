#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/poll.h>
#include <stdbool.h>
#include <unistd.h>

#define MAXLINE	1024

#define MAX_SOCK 512
#define BUFFER_SIZE 100

char *escapechar = "exit\n";
int readline(int, char *, int);

struct ID {
	int fd;
	char id[24];
};
int num_chat = 0;

struct ID ID[MAX_SOCK];


int isExist(char *idc) {
	for (int i=1; i<num_chat; i++){
		if (strcmp(ID[i].id,idc)==0){ 
			return i;
		}
	}
	return 0;
}

void send_all(char *msg,int i){
	char sendmsg[MAXLINE];
	for (int j = 1; j <= num_chat; j++)
	{
		if(i==j) continue;		
		sprintf(sendmsg,"[%s] %s", ID[i].id, msg);
		send(ID[j].fd,sendmsg,strlen(sendmsg)+1,0);
	}	
}

int main(int argc, char *argv[])  {
	char 	rline[MAXLINE], my_msg[MAXLINE];
	char 	*start = "티맥스 대화방에 오신걸 환영합니다...\n";
	const char *yes = "yes\n";
	const char *no = "no\n";
	char     *end = "티맥스 대화방에서 나가셨습니다...\n";
	char     *temp;
	char sendmsg[MAXLINE];

	char	   id_array[24];
	int 	i, j, n;
	int 	s, client_fd, clilen;
	int	nfds;		

	// int 	err_cnt=0;
	fd_set	read_fds;
	int 	num_max = 0;

	struct pollfd client[MAX_SOCK];
	FILE *fp;

	struct sockaddr_in 	client_addr, server_addr;

	if(argc < 2)  {
		printf("실행방법 :%s 포트번호\n",argv[0]); 
		return -1;
	}

	printf("티맥스 대화방 서버 초기화 중....\n");

	if((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)  {
		printf("Server: Can't open stream socket.");   
		return -1;
	}

	memset((char *)&server_addr,0, sizeof(server_addr));  
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(atoi(argv[1]));     

	if (bind(s,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
		printf("Server: Can't bind local address.\n");
		return -1;
	}

	listen(s, 5);

	client[0].fd = s;
	client[0].events = POLLIN;
	client[0].revents = 0;

	for (i=1; i<MAX_SOCK; i++)
	{
		client[i].fd = -1;
		ID[i].fd = -1;
		strcpy(ID[i].id," ");
	}
	//printf("hello");
	while(1)
	{
		int nread = poll(client, MAX_SOCK, 1000);
		//printf("%s\n",rline);
		if (nread >0)
		{
			int err_cnt = 0;

			if (client[0].revents == POLLIN) {
				num_chat++;
				num_max++;
				clilen = sizeof(client_addr);
				client_fd = accept(s, (struct sockaddr *)&client_addr, &clilen);
				printf("%d번째 사용자 추가. fd:%d\n",num_chat, client_fd);

				// /id id
				if ((n = recv(client_fd,rline,MAXLINE,0)) <= 0 ){ 
					printf("wrong client\n");
					exit(1);
				}

				printf("recv_id\n");

				temp = rline;

				if (strncmp(temp,"/id",3) !=0) {
					printf("/id not received\n");
					close(client_fd);
					num_chat--;
					num_max--;
					continue;	  
				}

				printf("ID_check\n");
				// check id existance
				temp = rline+4;


				// client id if exists, close connect
				if(isExist(temp)){
					send(client_fd,no,strlen(no),0);
					printf("Ban\n");
					num_chat--;
					close(client_fd);
					continue;
				}


				// unique check 완료, 새 소켓 배정
				send(client_fd,yes,strlen(yes),0);

				strcpy(ID[num_chat].id,rline+4); // ID[num_chat] id에 client id 저장

				ID[num_chat].fd = client_fd;
				client[num_chat].fd = client_fd;
				client[num_chat].events = POLLIN;
				client[num_chat].revents = 0;	
				send(client_fd, start, strlen(start), 0);
			}  // end of server socket 

			// client sockets processing
			for (i = 1; i <= num_chat; i++)
			{
				switch (client[i].revents)
				{
					case 0:
						break;

					case POLLIN:
						// client disconneted
						printf("client pollin: %d\n",client[i].fd);
						if ((n = recv(client[i].fd, rline, MAXLINE,0)) == 0)
						{
							close(client[i].fd);
							client[i].fd = -1;
							client[i].revents = 0;
							client[i].events = 0;

							strcpy(ID[i].id," ");
							ID[i].fd = -1;

							for (j = 1; j <= num_chat; j++)
							{
								if(i==j) continue;
								send(client[j].fd,end,strlen(end),0);
							}
							printf("%s\n",end);
							num_chat--;
							break; 
						}

						// usual message process
						rline[n] ='\0';
						/*if (exitCheck_R(rline, escapechar, 5) ==1) {
							client[i].fd = -1;
							client[i].revents = 0;
							}*/
						// if @ 귓속말 else 전체채팅
						if (rline[0]=='@')
						{
							for(j=1;;j++)
							{
								if (rline[j] == ' ') break;
								id_array[j-1]=rline[j];
							}

							id_array[j-1] = '\0';


							printf("'%s'\n",id_array);
							printf("num_chat = %d\n", num_chat);

							// 상대방 id가 현재 등록되어 있으면 @id xxx에서
							// @id를 제외한 귓속말 내용 xxx와 자신의 id를 붙여 전송
							int index;
							if((index=isExist(id_array))>0){
								sprintf(sendmsg,"DM[%s] %s", ID[index].id, &rline[strlen(id_array)+2]);
								send(ID[index].fd,sendmsg,strlen(sendmsg)+1,0);
							}
							else{
								send(ID[i].fd,"not sent",10,0);
							}
						}
						// plain message
						else
						{
							send_all(rline, i);	
						}
						memset(rline, 0, MAXLINE); //rline 초기
						break;

					default:
						client[i].fd = -1;
						client[i].revents = 0;
				}//end of switch
			} //end of client search      
		} //end of poll event search
	} //end of server while loop

	return 0;
}

/*int exitCheck(char* rline,char* escapechar,int len)
	{
	int	i, max;
	char	*tmp;

	max = strlen(rline);	
	tmp = rline;
	for(i = 0; i<max; i++) {
	if (*tmp == escapechar[0]) {
	if(strncmp(tmp, escapechar, len) == 0)
	return 1;
	} else 
	tmp++;
	} 
	return -1;
	}*/
int exitCheck_R(char* rline,char* escapechar,int len)
{
	int	i, max;
	char	*tmp;

	max = strlen(rline);	
	tmp = strstr(rline,"exit");
	for(i = 0; i<max; i++) {
		if(strcmp(tmp, "exit") == 0)
			return 1;
	} 
	return -1;
}
