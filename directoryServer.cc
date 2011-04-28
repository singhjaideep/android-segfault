
const char * usage =
"                                                               \n"
"HTTP-server:                                                \n"
"                                                               \n"
"Simple server program that shows how to use socket calls       \n"
"in the server side.                                            \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   myhttpd <f|t|p> <port>                                       \n"
"                                                               \n"
"Where 1024 < port < 65536.             \n"
"                                                               \n"
"                                                               \n";


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

int QueueLength = 5;
int t_flag = 0;
int f_flag = 0;
int status;
char **usrname;
char **userIP;
int usrtime[10];
int ucount;
time_t t;

// Processes time request
void sendIP(int fd);
void sendData(int fd);
void addUser(char *, int fd);
void processTimeRequest( int socket );
void requestPath(char *, char *);
void* userCheck( int );
static void sigchld_handler(int s);
int
main( int argc, char ** argv )
{
  userIP = (char**)malloc(sizeof(char*)*10);
  usrname = (char**)malloc(sizeof(char*)*10);
  ucount = 0;


  int flag = 0;
  char *portInit = "6159";
  // Print usage if not enough arguments
  if ( argc == 1 ) {
    flag = 1;
    //fprintf( stderr, "%s", usage );
    //exit( -1 );
  }
  else if( argc == 2) {
    if(strcmp(argv[1],"-f")==0 || strcmp(argv[1],"-t")==0 || strcmp(argv[1],"-p")==0){
    }
    else {
      flag = 1;
      portInit = argv[1];
    }
  }
  else if( argc == 3 ) {
    portInit = argv[2];
  }
  else {
    fprintf( stderr, "%s", usage );
    exit( -1 );
  }
  // Get the port from the arguments
  int port = atoi( portInit );
  printf("Server Port: %d\n", port); 
   
  // Set the IP address and port for this server
  struct sockaddr_in serverIPAddress; 
  memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_addr.s_addr = INADDR_ANY;
  serverIPAddress.sin_port = htons((u_short) port);
  
  // Allocate a socket
  int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
  if ( masterSocket < 0) {
    perror("socket");
    exit( -1 );
  }

  // Set socket options to reuse port. Otherwise we will
  // have to wait about 2 minutes before reusing the sae port number
  int optval = 1; 
  int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
		       (char *) &optval, sizeof( int ) );
   
  // Bind the socket to the IP address and port
  int error = bind( masterSocket,
		    (struct sockaddr *)&serverIPAddress,
		    sizeof(serverIPAddress) );
  if ( error ) {
    perror("bind");
    exit( -1 );
  }
  
  // Put socket in listening mode and set the 
  // size of the queue of unprocessed connections
  error = listen( masterSocket, QueueLength);
  if ( error ) {
    perror("listen");
    exit( -1 );
  }
  struct sigaction act;
  memset(&act, 0, sizeof(act));
  act.sa_handler = sigchld_handler;
  //sigemptyset(&act.sa_mask);
  //act.sa_flags = SA_RESTART;

  if(sigaction(SIGCHLD, &act, NULL) < 0){
    perror("Sigaction Failed");
    exit(1);
  }
  if(flag == 1){
    printf("Server Mode: Iterative\n");
  }
  else if(strcmp(argv[1],"-f") == 0){
    printf("Server Mode: Fork\n");
  }
  else if(strcmp(argv[1],"-t") == 0){
    printf("Server Mode: Thread\n");
  }
  int x = 0;
  //pthread_create(NULL, NULL, (void * (*)(void *))userCheck, (void*)x);
  
  pthread_attr_t attr;
  pthread_attr_init( &attr );
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

        signal(SIGCHLD, SIG_IGN);
  while ( 1 ) {
    // Accept incoming connections
    struct sockaddr_in clientIPAddress;
    int alen = sizeof( clientIPAddress );
    int slaveSocket = accept( masterSocket,
			      (struct sockaddr *)&clientIPAddress,
			      (socklen_t*)&alen);
    
    char *a;
    a = inet_ntoa(clientIPAddress.sin_addr);
    //printf("\n\n%s\n\n",a);
    if ( slaveSocket < 0 ) {
      perror( "accept" );
      exit( -1 );
    }
    else if( flag == 0){
      if( strcmp(argv[1],"-f") == 0 ){
        f_flag = 1;
        int ret  = fork();
        if(ret == 0){
          processTimeRequest( slaveSocket );
          exit(0);
        } 
        close(slaveSocket);
      }
      else if( strcmp(argv[1],"-t") == 0){
        t_flag = 1;
        pthread_t *th = (pthread_t*)malloc(sizeof(pthread_t));
        pthread_create(th, &attr, (void * (*)(void *))processTimeRequest, (void *)slaveSocket); 
        pthread_detach(*th);
      }
      else{
        processTimeRequest( slaveSocket );
      }
    }
    else{
      processTimeRequest( slaveSocket );
      close (slaveSocket);
    }

    // Process request.
    //processTimeRequest( slaveSocket );

    // Close socket
    if(flag == 0 && strcmp(argv[1],"-t")!=0){	
      close( slaveSocket );
    }
  }
  
}

void *userCheck (int tmp) { 
  int i = 0;
  while ( 1 ) {
    time_t current = time(NULL);
    for(i = 0; i < ucount; i++){
      if(current - usrtime[i] > 5){
//        printf("removing: %s\n",usrname[i]);
//        printf("current time: %d, User time: %d",current,usrtime[i]);
        int j = 0;
        for( j = i; j < ucount - 1; j++){
          usrname[j] = usrname[j+1];
          userIP[j] = userIP[j+1];
          usrtime[j] = usrtime[j+1];
        }
        ucount--;
      }
    }
  }
}

void *showUser (int tmp){
  int i = 0;
  while(1){
  printf("\n\n -- Online Users\n");
  for(i=0;i<ucount;i++){
    printf("     %d. %s\n",i+1,usrname[i]);    
  }
  printf("\n\n");
  sleep(5000);
  }
  
}

void
processTimeRequest( int fd )
{
  // Buffer used to store the name received from the client
  const int MaxRequest = 10240;
  char request[ MaxRequest + 1 ];
  int requestLength = 0;
  int n;

  unsigned char newChar;

  unsigned char lastChar = 0;

  //
  // The client should send <name><cr><lf>
  // Read the name of the client character by character until a
  // <CR><LF> is found.
  //
  int rcount = 0;    
  while ( requestLength < MaxRequest &&
	  ( n = read( fd, &newChar, sizeof(newChar) ) ) > 0 ) {

//     printf("%c",newChar);

    if ( lastChar == '\015' && newChar == '\012' ) {
      // Discard previous <CR> from name
      requestLength--;
      break;
    }

    if( newChar == '\n'){
      requestLength--;
      break;
     }
    if(rcount >= 0){	
      request[ requestLength ] = newChar;
//      printf("%d,%c,",requestLength,request[requestLength]);
      requestLength++;
    }
    else{
      requestLength = 0;
    }
    rcount++;

    lastChar = newChar;
  }

  // Add null character at the end of the string
  request[ requestLength ] = 0;
 // printf("\n%d: %s\n",requestLength,request);

  //if(strcmp(request,"-2")==0){
  if(request[0] == '-' && request[1] == '2'){
  //  printf("Sending data\n\n");
    int i =0;
    for(i = 0;i < ucount;i++){
      if(strcmp(request + 2,usrname[i]) == 0){
        usrtime[i] = time(NULL);
        //printf("%s time: %d\n",usrname[i],usrtime[i]);
        break;
      }
    }
    sendData(fd);
  }
  else if(strcmp(request,"-3")==0){
   // printf("Sending IP\n");
    sendIP(fd);
  }
  else{
    //printf("Adding User\n\n");
    addUser(request, fd);
  }
}

void
sendIP( int fd ){
  const int MaxRequest = 10240;
  char request[ MaxRequest + 1 ];
  int requestLength = 0;
  int n;

  unsigned char newChar;

  unsigned char lastChar = 0;
  int rcount = 0;    
  while ( requestLength < MaxRequest &&
	  ( n = read( fd, &newChar, sizeof(newChar) ) ) > 0 ) {

//     printf("%c",newChar);

    if ( lastChar == '\015' && newChar == '\012' ) {
      // Discard previous <CR> from name
      requestLength--;
      break;
    }

    if( newChar == '\n'){
      requestLength--;
      break;
     }
    if(rcount >= 0){	
      request[ requestLength ] = newChar;
//      printf("%d,%c,",requestLength,request[requestLength]);
      requestLength++;
    }
    else{
      requestLength = 0;
    }
    rcount++;

    lastChar = newChar;
  }

  // Add null character at the end of the string
  request[ requestLength ] = 0;
//  printf("\nIP request for(%d): %s\n",requestLength,request);

  int i =0;
  for(i = 0;i < ucount;i++){
    if(strcmp(request,usrname[i]) == 0){
      write( fd, userIP[i], strlen(userIP[i]));
      break;
    }
  }

  //free(request);
  //write( fd, "got it \n", 8);
  close (fd);
}

void
addUser(char * r, int fd){

  int uflag = 1;

  if(ucount==0){
    usrname[ucount] = (char*)malloc(sizeof(char) * (strlen(r) + 1));
    memcpy(usrname[ucount], r, strlen(r) + 1);
 //   printf("Added: %s ", usrname[ucount]);
  }
  else{
    int i = 0;
    for(i = 0;i<ucount;i++){
      if(strcmp(usrname[i],r) == 0){
        uflag = 0;
        break;
      }
    }
    if(uflag != 0){
    usrname[ucount] = (char*)malloc(sizeof(char) * (strlen(r) + 1));
    memcpy(usrname[ucount], r, strlen(r) + 1);
  //  printf("on %s\n", usrname[ucount]);
    }
  }
  
  free(r);

  const int MaxRequest = 10240;
  char request[ MaxRequest + 1 ];
  int requestLength = 0;
  int n;

  unsigned char newChar;

  unsigned char lastChar = 0;
  int rcount = 0;    
  while ( requestLength < MaxRequest &&
	  ( n = read( fd, &newChar, sizeof(newChar) ) ) > 0 ) {

//     printf("%c",newChar);

    if ( lastChar == '\015' && newChar == '\012' ) {
      // Discard previous <CR> from name
      requestLength--;
      break;
    }

    if( newChar == '\n'){
      requestLength--;
      break;
     }
    if(rcount >= 0){	
      request[ requestLength ] = newChar;
 //     printf("%d,%c,",requestLength,request[requestLength]);
      requestLength++;
    }
    else{
      requestLength = 0;
    }
    rcount++;
    lastChar = newChar;
  }

  // Add null character at the end of the string
  request[ requestLength ] = 0;
  //printf("UserIP: %s\n\n",request);
  
  if(uflag == 1){
  userIP[ucount] = (char*)malloc(sizeof(char) * (strlen(request)));
  memcpy(userIP[ucount], request + 1, strlen(request));
  //free(request);
 // printf("Added IP: %s\n\n", userIP[ucount]);
  }
  char *login = "Login Successful\n";
  if(uflag == 1){ 
    usrtime[ucount] = time(NULL);
    ucount++;
    write( fd,login,strlen(login));
  }
  else{
    write( fd, "username not available\n",24);
  }
  close(fd);
}

void sendData( int fd ){

  char * test = "\n";
  char * end = "-1 \n";
  try{
    int ij = 0;
    int conFlag = 0;
    //while(true){
 //   printf("ucount: %d\n",ucount);
    for(ij = 0;ij<ucount;ij++){
      conFlag = write( fd,usrname[ij],strlen(usrname[ij]));
      write( fd,test,strlen(test));
      if(conFlag == -1){
        break;
      }
    }
      if(conFlag == -1){
      //  break;
      }
    write( fd,end,strlen(end));
    
    //}
  }
  catch(int e){
    printf("error\n");
  }
//  printf("end of thread"); 
  if(t_flag == 1 || f_flag ==1){
    close (fd);
    //pthread_exit((void*)status);
  }
  if(t_flag == 1){
    //printf("th test\n\n\n");
  //  pthread_exit((void *)status);
  }
}

static void
sigchld_handler (int s){
  printf("test");
  while(waitpid(-1,NULL,WNOHANG) > 0){

  }
}
