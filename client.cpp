/*
*
* Chatroom
* Codigo tomado de: Andrew Herriot - License: Public Domain
*
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>


#include <sys/socket.h>
#include <arpa/inet.h>


#ifndef CHATROOM_UTILS_H_
#define CHATROOM_UTILS_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include "payload.pb.h"


//color codes
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define RESET "\033[0m"

//Enum of different messages possible.
typedef enum
{
  CONNECT,
  DISCONNECT,
  GET_USERS,
  SET_USERNAME,
  PUBLIC_MESSAGE,
  PRIVATE_MESSAGE,
  TOO_FULL,
  USERNAME_ERROR,
  SUCCESS,
  ERROR

} message_type;


//message structure
typedef struct
{
  message_type type;
  char username[21];
  char data[256];

} message;

//structure to hold client connection information
typedef struct connection_info
{
  int socket;
  struct sockaddr_in address;
  char username[20];
} connection_info;


// Removes the trailing newline character from a string.
void trim_newline(char *text);

// discard any remaining data on stdin buffer.
void clear_stdin_buffer();

#endif


void trim_newline(char *text)
{
  int len = strlen(text) - 1;
  if (text[len] == '\n')
{
      text[len] = '\0';
  }
}

void clear_stdin_buffer()
{
  int c;
  while((c = getchar()) != '\n' && c != EOF)
    /* discard content*/ ;
}

// get a username from the user.
void get_username(char *username)
{
  while(true)
  {
    printf("Ingrese un nombre de usuario :) : ");
    fflush(stdout);
    memset(username, 0, 1000);
    fgets(username, 22, stdin);
    trim_newline(username);
    
    Payload payload;
    payload.set_sender(username);

    if(strlen(username) > 20)
    {
      // clear_stdin_buffer();
      puts("El nombre de usuario tiene que ser mas corto.");

    } else {
      break;
    }
  }
}

//send local username to the server.
void set_username(connection_info *connection)
{
  message msg;
  msg.type = SET_USERNAME;
  strncpy(msg.username, connection->username, 20);

  if(send(connection->socket, (void*)&msg, sizeof(msg), 0) < 0)
  {
    perror("Error al enviar mensaje");
    exit(1);
  }
}

void stop_client(connection_info *connection)
{
  close(connection->socket);
  exit(0);
}

//initialize connection to the server.
void connect_to_server(connection_info *connection, char *address, char *port)
{

  while(true)
  {
    get_username(connection->username);

    //Create socket
    if ((connection->socket = socket(AF_INET, SOCK_STREAM , IPPROTO_TCP)) < 0)
    {
        perror("No se pudo crear un socket");
    }

    connection->address.sin_addr.s_addr = inet_addr(address);
    connection->address.sin_family = AF_INET;
    connection->address.sin_port = htons(atoi(port));

    //Connect to remote server
    if (connect(connection->socket, (struct sockaddr *)&connection->address , sizeof(connection->address)) < 0)
    {
        perror("Error al hacer la conexion.");
        exit(1);
    }

    set_username(connection);

    message msg;
    ssize_t recv_val = recv(connection->socket, &msg, sizeof(message), 0);
    if(recv_val < 0)
    {
        perror("recv fallido");
        exit(1);

    }
    else if(recv_val == 0)
    {
      close(connection->socket);
      printf("El usuario: \"%s\" ya esta en uso, porfavor use otro nombre.\n", connection->username);
      continue;
    }

    break;
  }


  puts("Conectado al servido.");
  puts("Escriba /help para ayuda.");
  Payload register_payload;
  register_payload.set_sender(connection->username);
}


void handle_user_input(connection_info *connection)
{
  char input[255];
  fgets(input, 255, stdin);
  trim_newline(input);

  if(strcmp(input, "/q") == 0 || strcmp(input, "/quit") == 0)
  {
    stop_client(connection);
  }
  else if(strcmp(input, "/l") == 0 || strcmp(input, "/list") == 0)
  {
    message msg;
    msg.type = GET_USERS;

    if(send(connection->socket, &msg, sizeof(message), 0) < 0)
    {
        perror("Error al enviar mensaje");
        exit(1);
    }
  }
  else if(strcmp(input, "/h") == 0 || strcmp(input, "/help") == 0)
  {
    puts("/quit or /q: Salir del programa.");
    puts("/help or /h: Muestra informacion del programa.");
    puts("/list or /l: Muestra la lista de usuarios en el chat.");
    puts("/m <usuario> <mensaje> Enviar un mensaje privado.");
  }
  else if(strncmp(input, "/m", 2) == 0)
  {
    message msg;
    msg.type = PRIVATE_MESSAGE;

    char *toUsername, *chatMsg;

    toUsername = strtok(input+3, " ");

    if(toUsername == NULL)
    {
      puts(KRED "El formato para enviar un mensaje privado es: /m <usuario> <mensaje>" RESET);
      return;
    }

    if(strlen(toUsername) == 0)
    {
      puts(KRED "Tiene que ingresar nombre de usuario para mensaje privado." RESET);
      return;
    }

    if(strlen(toUsername) > 20)
    {
      puts(KRED "El nombre de usuario tiene que ser mas corto." RESET);
      return;
    }

    chatMsg = strtok(NULL, "");

    if(chatMsg == NULL)
    {
      puts(KRED "Tiene que ingresar un mensaje para enviar al usuario." RESET);
      return;
    }

    //printf("|%s|%s|\n", toUsername, chatMsg);
    strncpy(msg.username, toUsername, 20);
    strncpy(msg.data, chatMsg, 255);

    if(send(connection->socket, &msg, sizeof(message), 0) < 0)
    {
        perror("Error al enviar mensaje");
        exit(1);
    }

  }
  else //regular public message
  {
    message msg;
    msg.type = PUBLIC_MESSAGE;
    strncpy(msg.username, connection->username, 20);

    // clear_stdin_buffer();

    if(strlen(input) == 0) {
        return;
    }

    strncpy(msg.data, input, 255);

    //Send some data
    if(send(connection->socket, &msg, sizeof(message), 0) < 0)
    {
        perror("Error al enviar mensaje");
        exit(1);
    }
  }



}

void handle_server_message(connection_info *connection)
{
  message msg;

  //Receive a reply from the server
  ssize_t recv_val = recv(connection->socket, &msg, sizeof(message), 0);
  if(recv_val < 0)
  {
      perror("recv fallido");
      exit(1);

  }
  else if(recv_val == 0)
  {
    close(connection->socket);
    puts("Servidor desconectado.");
    exit(0);
  }

  switch(msg.type)
  {

    case CONNECT:
      printf(KYEL "%s se ha conectado." RESET "\n", msg.username);
    break;

    case DISCONNECT:
      printf(KYEL "%s se ha desconectado." RESET "\n" , msg.username);
    break;

    case GET_USERS:
      printf("%s", msg.data);
    break;

    case SET_USERNAME:
      //TODO: implement: name changes in the future?
    break;

    case PUBLIC_MESSAGE:
      printf(KGRN "%s" RESET ": %s\n", msg.username, msg.data);
    break;

    case PRIVATE_MESSAGE:
      printf(KWHT "De %s:" KCYN " %s\n" RESET, msg.username, msg.data);
    break;

    case TOO_FULL:
      fprintf(stderr, KRED "El chatroom esta muy lleno de personas, intente mas tarde." RESET "\n");
      exit(0);
    break;

    default:
      fprintf(stderr, KRED "Error en el mensaje." RESET "\n");
    break;
  }
}

int main(int argc, char *argv[])
{
  connection_info connection;
  fd_set file_descriptors;

  if (argc != 3) {
    fprintf(stderr,"Ingresar: %s <IP> <port>\n", argv[0]);
    exit(1);
  }

  connect_to_server(&connection, argv[1], argv[2]);

  //keep communicating with server
  while(true)
  {
    FD_ZERO(&file_descriptors);
    FD_SET(STDIN_FILENO, &file_descriptors);
    FD_SET(connection.socket, &file_descriptors);
    fflush(stdin);

    if(select(connection.socket+1, &file_descriptors, NULL, NULL, NULL) < 0)
    {
      perror("Seleccion fallida.");
      exit(1);
    }

    if(FD_ISSET(STDIN_FILENO, &file_descriptors))
    {
      handle_user_input(&connection);
    }

    if(FD_ISSET(connection.socket, &file_descriptors))
    {
      handle_server_message(&connection);
    }
  }

  close(connection.socket);
  return 0;
}
