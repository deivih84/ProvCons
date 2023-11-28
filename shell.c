// David e Iván
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_COMMAND_LENGTH 800
#define MAX_ARGUMENTS 2

int main() {
    // Variables
    char command[MAX_COMMAND_LENGTH];
    int argument_count;
    char *token;
    size_t command_length;
    int bandera = 1;
    char **args; // Guardamos memoria dinámica
    char *arguments[MAX_ARGUMENTS + 1];
    pid_t pid;

    while (bandera != 0) {
        write(1, "minishell>> ", 12);
        if (read(1, command, sizeof(command)) == -1) {
            bandera = 0; // Salir si se llega al final del archivo de entrada
        }

        command_length = strlen(command);

        for(int i = 0; i < strlen(command); i++) {
            if(command[i] == '\n') {
                command[i] = '\0';
            }
        }

        // Dividir el comando por partes, que se guardaran en arguments
        token = strtok(command, " ");
        argument_count = 0; // Nº de argumentos
        while (token != NULL) {
            if (argument_count <= MAX_ARGUMENTS) {
                arguments[argument_count] = (char *)malloc(strlen(token) + 1); // Guardamos memoria dinámica

                if (arguments[argument_count] == NULL) {
                    fprintf(stderr,
                            "Error: No se pudo reservar memoria para el token.\n");
                    bandera = 0; // Se sale del bucle
                }

                // Copia el token en la memoria reservada
                strcpy(arguments[argument_count], token);
                argument_count++;

                token = strtok(NULL, " ");
            } else {
                fprintf(stderr, "Warning: Se han proporcionado demasiados argumentos. Se ignoraran los adicionales.\n");
                bandera = 0; // Se sale del bucle
            }
        }

        for (int i = 0; i < argument_count; i++) {
            //free(arguments[i]); // Liberamos el espacio de la memoria dinámica
        }
        argument_count--;

        if (!strcmp(arguments[0], "muestra")) { // Reconocer MUESTRA
            if (argument_count == 1 || argument_count == 0) {
                // Reservar memoria dinámica para args y sus elementos
                args = (char **)calloc(6, sizeof(char *));
                if (args == NULL) {
                    perror("Error al asignar memoria para args");
                    bandera = 0; // Se sale del bucle
                }

                // Configurar los argumentos para execv
                args[0] = "cat";
                args[1] = arguments[1];
                args[2] = NULL;

                if (argument_count == 0) {args[1] = NULL;}

                pid = fork(); // Crea un hijo para ejecutar el comando
                if (pid == -1) {
                    perror("Error al crear el proceso hijo");
                    bandera = 0; // Se sale del bucle
                } else if (pid == 0) {
                    // Ejecutar "cat" con el archivo proporcionado
                    execvp("/bin/cat", args);

                    // Si execv() tiene éxito, esta parte del código no se ejecutará
                    perror("Error al ejecutar cat");
                } else { // Proceso padre espera a que termine
                    wait(NULL);
                }
                free(args);
            } else {
                fprintf(stderr, "Warning: Invocación del comando 'muestra' con más de "
                                "un parámetro.\n");
            }
        }

        else if (!strcmp(arguments[0], "lista")) { // Reconocer LISTA
            if (argument_count <= 1) {
                // Reservar memoria dinámica para ls_args y sus elementos
                args = (char **)calloc(6, sizeof(char *));
                // Configurar los argumentos para execv
                args[0] = "ls";
                args[1] = arguments[1];
                args[2] = NULL;

                if (argument_count == 0) {args[1] = NULL;}

                pid = fork(); // Crea un hijo para ejecutar el comando
                if (pid == -1) {
                    perror("Error al crear el proceso hijo");
                    bandera = 0; // Se sale del bucle
                } else if (pid == 0) {
                    // Ejecutar "ls" con el archivo proporcionado
                    execvp("/bin/ls", args);

                    // Si execv() tiene éxito, esta parte del código no se ejecutará
                    perror("Error al ejecutar ls");
                } else { // Proceso padre espera a que termine
                    wait(NULL);
                }
                free(args);
            } else {
                fprintf(stderr,"Warning: Invocación del comando lista con más de un parámetro.\n");
            }
        }

        else if (!strcmp(arguments[0], "copia")) { // Reconocer COPIA
            if (argument_count == 2) {
                // Reservar memoria dinámica para copia_args y sus elementos
                args = (char **)calloc(6, sizeof(char *));
                if (args[0] == NULL) {
                    perror("Error al asignar memoria para copia_args");
                    bandera = 0; // Sale del bucle
                }
                // Configurar los argumentos para execv
                args[0] = "copia";
                args[1] = arguments[1];
                args[2] = arguments[2];
                args[3] = NULL;

                pid = fork(); // Crea un hijo para ejecutar el comando
                if (pid == -1) {
                    perror("Error al crear el proceso hijo");
                    bandera = 0; // Se sale del bucle
                } else if (pid == 0) {
                    // Ejecutar "copia" con el archivo proporcionado
                    execvp("./copia", args);

                    // Si execv() tiene éxito, esta parte del código no se ejecutará
                    perror("Error al ejecutar copia");
                } else { // Proceso padre espera a que termine
                    wait(NULL);
                }
                free(args);
            } else {
                fprintf(stderr, "Warning: El comando 'copia' requiere exactamente dos parámetros.\n");
            }
        } else if (!strcmp(arguments[0], "salir")) { // Reconocer SALIR
            printf("Saliendo del shell.\n");
            bandera = 0; // Se sale del bucle
        } else { // No es un comando de los listados
            fprintf(stderr, "Error. La orden comando no es un comando valido. Use muestra, copia o lista.\n");
        }

    }
    return 0;
}