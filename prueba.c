#include <stdio.h>
#include <pthread.h>

// Estructura para almacenar datos que se pasan a los hilos
struct ThreadData {
    int num;
    char c;
    FILE *fich;
    int id;
};

// Función que realizará cada hilo
void *miFuncion(void *arg) {
    struct ThreadData *data = (struct ThreadData *)arg;

    // Realiza operaciones utilizando los datos pasados
    // ...

    // Ejemplo: escribir en el fichero
    fprintf(data->fich, "Hilo %d escribió: %c%d\n", data->id, data->c, data->num);

    pthread_exit(NULL);
}

int main() {
    FILE *fichero = fopen("miarchivo.txt", "w");
    // Crea hilos y pasa los datos
    pthread_t hilo1, hilo2;
    if (fichero == NULL) {
        fprintf(stderr, "Error al abrir el archivo.\n");
        return 1;
    }

    // Crea datos para cada hilo
    struct ThreadData data1 = {42, 'A', fichero, 0};
    struct ThreadData data2 = {73, 'B', fichero, 1};

    pthread_create(&hilo1, NULL, miFuncion, (void *)&data1);
    pthread_create(&hilo2, NULL, miFuncion, (void *)&data2);

    // Espera a que los hilos terminen
    pthread_join(hilo1, NULL);
    pthread_join(hilo2, NULL);

    // Cierra el fichero después de que los hilos hayan terminado
    fclose(fichero);

    return 0;
}
