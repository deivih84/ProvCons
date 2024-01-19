#include <stdio.h>
#include <stdlib.h>

// Estructura para un nodo de la lista
struct Nodo {
    int** matriz;
    int filas;
    int columnas;
    struct Nodo* siguiente;
};

// Función para agregar una matriz a la lista
void agregarMatriz(struct Nodo** cabeza, int** matriz, int filas, int columnas) {
    // Crea un nuevo nodo
    struct Nodo* nuevoNodo = (struct Nodo*)malloc(sizeof(struct Nodo));

    // Asigna la matriz al nuevo nodo
    nuevoNodo->matriz = matriz;
    nuevoNodo->filas = filas;
    nuevoNodo->columnas = columnas;

    // Establece el siguiente nodo como el actual inicio de la lista
    nuevoNodo->siguiente = *cabeza;

    // Actualiza la cabeza de la lista para que apunte al nuevo nodo
    *cabeza = nuevoNodo;
}

// Función para imprimir todas las matrices en la lista
void imprimirLista(struct Nodo* cabeza) {
    struct Nodo* actual = cabeza;

    while (actual != NULL) {
        printf("Matriz (%d x %d):\n", actual->filas, actual->columnas);
        for (int i = 0; i < actual->filas; i++) {
            for (int j = 0; j < actual->columnas; j++) {
                printf("%d ", actual->matriz[i][j]);
            }
            printf("\n");
        }
        printf("\n");

        actual = actual->siguiente;
    }
}

// Función para liberar la memoria de la lista
void liberarLista(struct Nodo* cabeza) {
    struct Nodo* actual = cabeza;
    struct Nodo* siguiente;

    while (actual != NULL) {
        siguiente = actual->siguiente;

        // Libera la memoria de la matriz
        for (int i = 0; i < actual->filas; i++) {
            free(actual->matriz[i]);
        }
        free(actual->matriz);

        // Libera la memoria del nodo
        free(actual);

        actual = siguiente;
    }
}

// Función para crear una matriz de ejemplo
int** crearMatriz(int filas, int columnas) {
    int** matriz = (int**)malloc(filas * sizeof(int*));
    for (int i = 0; i < filas; i++) {
        matriz[i] = (int*)malloc(columnas * sizeof(int));
        for (int j = 0; j < columnas; j++) {
            matriz[i][j] = i * columnas + j + 1;
        }
    }
    return matriz;
}

int main() {
    struct Nodo* cabeza = NULL;  // Inicializa la cabeza de la lista como NULL

    // Crea y agrega la primera matriz
    int filas1 = 3;
    int columnas1 = 3;
    int** matriz1 = crearMatriz(filas1, columnas1);
    agregarMatriz(&cabeza, matriz1, filas1, columnas1);

    // Crea y agrega la segunda matriz
    int filas2 = 2;
    int columnas2 = 2;
    int** matriz2 = crearMatriz(filas2, columnas2);
    agregarMatriz(&cabeza, matriz2, filas2, columnas2);

    // Crea y agrega la tercera matriz
    int filas3 = 4;
    int columnas3 = 4;
    int** matriz3 = crearMatriz(filas3, columnas3);
    agregarMatriz(&cabeza, matriz3, filas3, columnas3);

    // Imprime la lista con todas las matrices
    imprimirLista(cabeza);

    // Libera la memoria de la lista
    liberarLista(cabeza);

    return 0;
}
