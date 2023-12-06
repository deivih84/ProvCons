//
// Created by Ivan y David on 11/22/2023.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_COMMAND_LENGTH 800
#define MAX_ARGUMENTS 5

typedef struct {
    char tipo;
    int proveedorID;
} Producto;

// Estructura para mantener el registro del total de productos de cada tipo
typedef struct {
    int total['j' - 'a' + 1];
} TotalProductos;

// Estructura para almacenar la información del consumidor
typedef struct nodo {
    int consumidorID;
    int productosConsumidos;
    int productosConsumidosPorTipo['j' - 'a' + 1];
    struct nodo *siguiente;
} ConsumidorInfo;

// Estructura para compartir datos entre proveedor y consumidor
typedef struct {
    char *ruta;
    char *fichDestino;
    int T;
    int P;
    int C;
    int in;               // Índice de escritura en el búfer
    int out;              // Índice de lectura en el búfer
    TotalProductos totalProductos; // Registro del total de productos
} SharedData;

// Variables GLOBALES :)
sem_t semaforoFichero, semaforoBuffer, semaforoLista, semaforoContadorBuffer;
Producto *buffer;
int contBuffer = 0;
ConsumidorInfo *listaConsumidores;

void proveedorFunc(SharedData *data);

void consumidorFunc(SharedData *data);

ConsumidorInfo *initListaProducto(ConsumidorInfo *lista);

ConsumidorInfo *agregarConsumidor(ConsumidorInfo *nodo, int productosConsumidos, int *productosConsumidosPorTipo, int ID);

void facturadorFunc(SharedData* sharedData);

bool esTipoValido(char c);

bool esCadena(char *string);

//printf("hola1");fflush(NULL); // PRINT PARA DEBUG

int main(int argc, char *argv[]) {
    char *path = argv[1];
    int arg3, arg4, arg5;
    SharedData sharedData;
    listaConsumidores = initListaProducto(listaConsumidores);
    FILE *file, *outputFile;
    pthread_t proveedorThread, consumidorThread, facturadorThread;


    // Verificación de la cantidad de argumentos
    if (argc != 6) {
        fprintf(stderr, "Error: Número incorrecto de argumentos.\n");
        return -1;
    }

    //Verificación parámetros

    //Verificar si los parámetros pasados son válidos o no. Si no, se pasa -1 para salir en el próximo if
    arg3 = (!esCadena(argv[3])) ? atoi(argv[3]) : -1;
    arg4 = (!esCadena(argv[4])) ? atoi(argv[4]) : -1;
    arg5 = (!esCadena(argv[5])) ? atoi(argv[5]) : -1;

    if (arg3 <= 0 || arg3 > 5000) {
        fprintf(stderr, "Error: T debe ser un entero positivo menor o igual a 5000.\n");
        return -1;
    }
    if (arg4 <= 0 || arg4 > 7) {
        fprintf(stderr, "Error: P debe ser un entero positivo menor o igual a 7.\n");
        return -1;
    }
    if (arg5 <= 0 || arg5 > 1000) {
        fprintf(stderr, "Error: C debe ser un entero positivo menor o igual a 1000.\n");
        return -1;
    }

    //AQUÍ HABRÁ QUE MODIFICAR COSAS PARA MÁS DE UN PROVEEDOR
    sprintf(path, "%s\\proveedor%d.dat", argv[1], 0);
    file = fopen(path, "r");
    if (file == NULL) {
        fprintf(stderr, "Error al abrir el archivo de entrada del proveedor %d.\n", sharedData.P);
    }


    sharedData.ruta = path; // Ruta de los archivos de entrada
    sharedData.fichDestino = argv[2]; // Nombre del fichero destino.
    sharedData.T = arg3; // Tamaño del búfer circular.
    sharedData.P = arg4; // Número total de proveedores.
    sharedData.C = arg5; // Número total de clientes.

    // Crear estructuras de datos compartidas
    sharedData.in = 0;
    sharedData.out = 0;
    buffer = malloc(sharedData.T * sizeof(Producto));
    if (buffer == NULL) {
        free(buffer);
        fprintf(stderr, "Error al asignar memoria para el búfer compartido.\n");
        return -1;
    }

    //Prueba apertura fichDestino
    outputFile = fopen(sharedData.fichDestino, "w");
    if (outputFile == NULL) {
        fprintf(stderr, "Error al abrir el archivo de salida.\n");
        fclose(outputFile);
        free(buffer);
        return -1;
    }

    sem_init(&semaforoFichero, 0, 1);
    sem_init(&semaforoBuffer, 0, 1);
    sem_init(&semaforoLista, 0, 1);
    sem_init(&semaforoContadorBuffer, 0, 1);


    // Crear hilo del proveedor
    pthread_create(&proveedorThread, NULL, (void *) proveedorFunc, &sharedData);
    printf("%s", "Hilo Proveedor lanzado.\n");

    // Crear hilo del consumidor
    pthread_create(&consumidorThread, NULL, (void *) consumidorFunc, &sharedData);
    printf("%s", "Hilo Consumidor lanzado.\n");


    // Esperar a que los hilos terminen
    pthread_join(proveedorThread, NULL);
    pthread_join(consumidorThread, NULL);


    // Crear hilo del facturadorFunc
    pthread_create(&facturadorThread, NULL, (void *) facturadorFunc, &sharedData);
    printf("%s", "Hilo Facturador lanzado.\n");

    pthread_join(facturadorThread, NULL);


    // Destruir semáforos y liberar memoria
    sem_destroy(&semaforoFichero);
    sem_destroy(&semaforoBuffer);
    sem_destroy(&semaforoLista);
    sem_destroy(&semaforoContadorBuffer);

    fclose(outputFile);
    free(buffer);
    return 0;
}

void proveedorFunc(SharedData *sharedData) {
    FILE *file, *outputFile;
    char c;
    int productosLeidos = 0, productosValidos = 0, productosInvalidos = 0, proveedorID = 0;
    TotalProductos totalProductos = {{0}};

    // Abrir el archivo de entrada del proveedor
    file = fopen(sharedData->ruta, "r");

    // Leer y procesar productos del archivo
    while ((c = (char) fgetc(file)) != EOF) {
        productosLeidos++;

        if (esTipoValido(c)) {
            // Incluir semáforo para escritura en el búfer
            sem_wait(&semaforoBuffer);
            // Escribir en el búfer
            buffer[sharedData->in].tipo = c;
            buffer[sharedData->in].proveedorID = proveedorID;
            sharedData->in = (sharedData->in + 1) % sharedData->T;

            sem_post(&semaforoBuffer);
            // Incrementar contador de productos válidos
            productosValidos++;
            // Actualizar registro de productos
            totalProductos.total[c - 'a']++;
        } else {
            // Procesar productos inválidos
            productosInvalidos++;
        }
        //buffer[sharedData->in + 1].tipo = '7'; //Fin del contenido del buffer
    }
    fclose(file); // Cerrar el archivo
    sem_post(&semaforoFichero);

    // Escribir resultados en el archivo de salida
    outputFile = fopen(sharedData->fichDestino, "w");
    if (outputFile == NULL) {
        fprintf(stderr, "Error al abrir el archivo de salida del proveedor %d.\n", 0);
        free(buffer);
        return;
    }

    fprintf(outputFile, "Proveedor: %d.\n", proveedorID);
    fprintf(outputFile, "   Productos procesados: %d.\n", productosLeidos);
    fprintf(outputFile, "   Productos Inválidos: %d.\n", productosInvalidos);
    fprintf(outputFile, "   Productos Válidos: %d. De los cuales se han insertado:\n", productosValidos);

    for (char tipo = 'a'; tipo <= 'j'; tipo++) {
        fprintf(outputFile, "     %d de tipo \"%c\".\n", totalProductos.total[tipo - 'a'], tipo);
    }

    // Cerrar el archivo de salida
    fclose(outputFile);
}

void consumidorFunc(SharedData *sharedData) {
    int consumidorID = 0, bandera = 0, numeroProductosConsumidosPorTipo['j' - 'a' + 1], numeroProductosConsumidos = 0;
    Producto productoConsumido;

    // Incializar numeroProductosConsumidosPorTipo[] //No sabes lo que hay en la memoria cuando vas a escribir.
    for (int i = 0; i <= 9; ++i) {
        numeroProductosConsumidosPorTipo[i] = 0;
    }

    // Consumir productos del búfer
    while (bandera != 1) {

        // Leer del buffer
        sem_wait(&semaforoContadorBuffer);
        sem_wait(&semaforoBuffer);
        productoConsumido = buffer[contBuffer];

        sem_post(&semaforoBuffer);

        numeroProductosConsumidos++; // Incremento de contador general
        numeroProductosConsumidosPorTipo[productoConsumido.tipo - 'a']++; // Incremento de contador del tipo correspondiente

        //Se actualiza el contador del buffer

        sem_wait(&semaforoBuffer);

        contBuffer = (contBuffer + 1) % sharedData->T;
        bandera = (contBuffer >= sharedData->T || !esTipoValido(buffer[contBuffer + 1].tipo)) ? 1 : 0;

        sem_post(&semaforoBuffer);
        sem_post(&semaforoContadorBuffer);
    }

    // Escribe en la lista el producto leido del buffer (lentamente perdiendo la cordura)
    sem_wait(&semaforoLista);
    listaConsumidores = agregarConsumidor(listaConsumidores, numeroProductosConsumidos,numeroProductosConsumidosPorTipo, consumidorID); //hay que pasarle prodConsPorTipo
    sem_post(&semaforoLista);
}


void facturadorFunc(SharedData* sharedData) {
    FILE *outputFile;
    int i = 0;

    sem_wait(&semaforoFichero);
    outputFile = fopen(sharedData->fichDestino, "a");
    sem_wait(&semaforoLista);
    // Agregar a la lista
    while (listaConsumidores != NULL) {
        // Contadores de productos, uno por tipos y uno por consumidores

        fprintf(outputFile, "\nCliente consumidor: %d\n", i);

        fprintf(outputFile, "  Productos consumidos: %d. De los cuales:\n", listaConsumidores->productosConsumidos);
        for (int j = 0; j < ('j' - 'a' + 1); ++j) {
            fprintf(outputFile, "     Producto tipo \"%c\": %d\n", (char) (j + 'a'),
                    listaConsumidores->productosConsumidosPorTipo[j]);
        }
        listaConsumidores = listaConsumidores->siguiente;
        i++;
    }
    sem_post(&semaforoFichero);

}


bool esTipoValido(char c) { // Devuelve True si está entre a y j (incluidas)
    return (c >= 'a' && c <= 'j');
}


bool esCadena(char *cadena) {
    for (int i = 0; i < strlen(cadena); ++i) {
        if (!isdigit(cadena[i])) {
            return true; // Devuelve 1 si no es una cadena de dígitos
        }
    }
    return false; // Devuelve 0 si es una cadena de dígitos
}


ConsumidorInfo *initListaProducto(ConsumidorInfo *lista) {
    lista = NULL;
    return lista;
}


ConsumidorInfo *agregarConsumidor(ConsumidorInfo *nodo, int productosConsumidos, int productosConsumidosPorTipo[], int ID) {
    ConsumidorInfo *nuevoConsumidor;
    ConsumidorInfo *aux;
    nuevoConsumidor = (ConsumidorInfo *) malloc(sizeof(ConsumidorInfo));
    nuevoConsumidor->productosConsumidos = productosConsumidos;

    // Se copia el array de productos consumidos por tipo
    memcpy(nuevoConsumidor->productosConsumidosPorTipo, productosConsumidosPorTipo,sizeof(nuevoConsumidor->productosConsumidosPorTipo));

    nuevoConsumidor->consumidorID = ID;
    nuevoConsumidor->siguiente = NULL;
    if (nodo == NULL) {
        nodo = nuevoConsumidor;
    } else {
        aux = nodo;
        while (aux->siguiente != NULL) {
            aux = aux->siguiente;
        }
        aux->siguiente = nuevoConsumidor;
    }
    return nodo;
}