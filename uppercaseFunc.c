#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>


#define MAX 8092
#define QUE 4096
int sum;
void *runner(void *param);
void *recv(void *param);
void recv_set(int data,int wait);
void get_input();

pthread_t *tid;
int thread_waiting=0;
int thread_num; //thread의 갯수
int notify =0; //thread에 알리기
pthread_mutex_t lock; //mutex lock
int line_scanner=1; // line 읽은지 확인여부
int *recv_index; //각 쓰레드의 큐에 어떤 인덱스에 집어넣을지
int *read_index; // 리드할 때 큐에 어떤 부분을 읽을지
int *recv_count; // 큐에 얼마만큼의 정보가 남았는
char print_queue[100][QUE][MAX]; // thread_num, 16 ,MAX
char total_line[MAX]; //한줄 받아오기
char save[MAX];
char line[MAX];
pthread_mutex_t recv_lock; //mutex lock

long long quit_count;
long long line_count;
int quit;

int main(int argc, char *argv[]){

	pthread_attr_t attr; //thread 특성	
	
	int i; //for문돌리는 용도
	
	int status; //creat thread 성공여부 
	
	pthread_mutex_t lock; //mutex lock

	quit_count=0;
	line_count=0;
	quit=0;
	if(argc!=3){
			printf("error");
			return 0;		
		}
	
	if(strcmp(argv[1],"-n")!=0)
		{
			printf("error");
			return 0;		
		}
	if(atoi(argv[2])<=0)	
		{
			printf("error");
			return 0;		
		}
	//printf("argv[2] : %s ",argv[2]);
	thread_num=atoi(argv[2]); // thread 몇개 만드는지 받아놓기
	pthread_mutex_init(&lock,NULL);

//	print_queue=(char ***)malloc(sizeof(char**)*(thread_num)); //thread_num만큼 생성 
	read_index=(int *) malloc(sizeof(int )*(thread_num));
	recv_index=(int *) malloc(sizeof(int )*(thread_num)); //recv read 쓰레드마다
	recv_count=(int *) malloc(sizeof(int )*(thread_num));
	for(i=0;i<thread_num;i++){
		read_index[i]=0;
		recv_index[i]=0;
		recv_count[i]=0;
	}
	
	tid=(pthread_t*)malloc(sizeof(pthread_t)*(thread_num+1)); //thread id 를 위한 배열 생성 
	for(i=0;i<thread_num;i++){
			
		
		pthread_attr_init(&attr);
		
		status=pthread_create(&tid[i],&attr,runner,NULL);
		
		if(status<0)
		{
			perror("thread creat error : ");
			exit(0);
		}
		
	} // thread 만들기
	
	pthread_attr_init(&attr); 
	pthread_create(&tid[i],&attr,recv,argv[1]);
	
	get_input();	
	

	for (i=thread_num;i<=thread_num;i++){
		pthread_join(tid[i],NULL);	
	}//thread 기다리기
    
    
}
void *runner(void *param){  // 혹시 16사이즈 넘어가면 recv_count 신경써줘야되
	pthread_t key;
	
	//char duplicate_line[MAX];
	int i;
	int waiting; //read count 등 현재 waiting thread 확 에 써줄나

	key=pthread_self();
	
	//printf("key 는 %ld\n",key);
	while(1){
		//printf("thread_waiting %d\n",thread_waiting);
    		while((key!=tid[thread_waiting])||notify==0);

		waiting=thread_waiting;
		//printf("gd%d\n%s\n",waiting, line);
		
		memset(print_queue[waiting][recv_index[waiting]], 0, MAX);
		strcpy(print_queue[waiting][recv_index[waiting]],line); // 받는 인덱스 정해주기 	
		
		pthread_mutex_lock(&lock);	
			thread_waiting=(thread_waiting+1)%(thread_num);	
			line_scanner=1; // thread가 빨리 복구 시켜줘야 읽을수 있다.
			notify=0;
		pthread_mutex_unlock(&lock);
		
		while(recv_count[waiting] >= QUE-1)
		{
		}

		for(i=0;i<MAX;i++){
			if(print_queue[waiting][recv_index[waiting]][i] >= 65 && print_queue[waiting][recv_index[waiting]][i] <= 90){
				print_queue[waiting][recv_index[waiting]][i]=print_queue[waiting][recv_index[waiting]][i]+32;			
			}

			else if( print_queue[waiting][recv_index[waiting]][i]=='\n'|| print_queue[waiting][recv_index[waiting]][i] == 0){
				print_queue[waiting][recv_index[waiting]][i]=print_queue[waiting][recv_index[waiting]][i];				
				break;
			}
		
		}
		//printf("%d\n%s\n",waiting, print_queue[waiting][recv_index[waiting]]);
		//pthread_mutex_lock(&lock);
		recv_index[waiting]=(recv_index[waiting]+1)%(QUE-1); // recv index를 올려준다.
		recv_set(recv_count[waiting]+1,waiting); // 받은 것의 count 를 해준다 . read 시 --
		//pthread_mutex_unlock(&lock);
		
	}
	pthread_exit(0);
    
}

void *recv(void *param){
    

	int wait=0;
	
	
	while(1){
		//sleep(2);
		while(recv_count[wait]==0){
			if(quit==1&&quit_count==line_count) exit(0);
		}
		//printf("%d %d\n", recv_index[wait], read_index[wait]);
		//printf("%d[%d] : %s\n", wait, read_index[wait], print_queue[wait][read_index[wait]]);
		printf("%s",  print_queue[wait][read_index[wait]]);
		quit_count++;	
		read_index[wait]=(read_index[wait]+1)%(QUE-1);
		//pthread_mutex_lock(&lock);
		recv_set(recv_count[wait]-1,wait);
		//pthread_mutex_unlock(&lock);
		wait=(wait+1)%thread_num;
		
		
			
	}
	pthread_exit(0);    
    
}
void recv_set(int data,int wait){
		pthread_mutex_lock(&recv_lock);
		//printf("%d : %d\n", wait, data);
		recv_count[wait]=data;
		pthread_mutex_unlock(&recv_lock);


}
void get_input(){
	int index=0,size,i=0;
	char input[MAX+1];
	char temp;
	char sendLine[MAX+1];
	
	
	while(1){	
		size=read(0,input,MAX);
		if(size==0) {quit=1;break;}
		
		while(1){
			while(line_scanner<1);
			temp=input[i++];
			//printf("%d temp:%c\n",i, temp);
			if(temp==0){
				//printf("loop end\n");
				i=0;
				memset(input, 0, MAX);
				break;
			}else if(temp=='\n'){

				sendLine[index++]='\n';
				sendLine[index]=0;
				strcpy(line,sendLine);
				line_count++;
				pthread_mutex_lock(&lock);
					line_scanner=0; // thread가 빨리 복구 시켜줘야 읽을수 있다.
					notify=1; // thread 한테 가져가라고 알림이;
				pthread_mutex_unlock(&lock);

                	    	index=0;

			}else {
				sendLine[index++]=temp;			
			}	
		} 
		
	}

}
