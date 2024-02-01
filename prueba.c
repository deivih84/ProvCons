#include <pthread.h>
#include <stdio.h>

// Función que será ejecutada por el hilo
void *threadFunction(void *arg) {
    // Realizar un casting del puntero a void al tipo de datos esperado
    char *arg1 = ((char **)arg)[0];
    char *arg2 = ((char **)arg)[1];
    FILE *file = ((FILE **)arg)[2];

    // Ahora puedes trabajar con los datos como deseas
    printf("Argumento 1: %s\n", arg1);
    printf("Argumento 2: %s\n", arg2);

    // Ejemplo de lectura de archivo
    if (file != NULL) {
        // Realizar operaciones con el archivo aquí
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    pthread_t myThread;

    // Verificar que haya suficientes argumentos
    if (argc < 3) {
        fprintf(stderr, "Error: Se requieren al menos dos argumentos.\n");
        return 1;
    }

    // Crear un array de punteros a void con los datos que quieres pasar
    void *threadArgs[] = { argv[1], argv[2], fopen("mi_archivo.txt", "r") };

    // Crear el hilo y pasarle el array como argumento
    if (pthread_create(&myThread, NULL, threadFunction, (void *)threadArgs) != 0) {
        fprintf(stderr, "Error al crear el hilo.\n");
        return 1;
    }

    // Esperar a que el hilo termine
    if (pthread_join(myThread, NULL) != 0) {
        fprintf(stderr, "Error al esperar al hilo.\n");
        return 1;
    }

    // Cerrar el archivo si fue abierto
    if (threadArgs[2] != NULL) {
        fclose((FILE *)threadArgs[2]);
    }

    return 0;
}
