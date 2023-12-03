//
// Created by Ivan y David on 11/22/2023.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
pthread_cond_t cond_proveedor = PTHREAD_COND_INITIALIZER;
sem_t semaforoFichero, semaforoBuffer, semaforoLista, semaforoContadorBuffer;
Producto *buffer;
int contBuffer = 0;
ConsumidorInfo *listaConsumidores;

void proveedorFunc(void *data);

void consumidorFunc(void *data);

ConsumidorInfo *initListaProducto(ConsumidorInfo *lista);

ConsumidorInfo *agregarConsumidor(ConsumidorInfo *producto, int productosConsumidos, int *productosConsumidosPorTipo, int ID);

void facturador(SharedData* sharedData);

bool esTipoValido(char c);

bool esCadena(char *string);


int main(int argc, char *argv[]) {
    char *path = argv[1];
    int arg3 = atoi(argv[3]), arg4 = atoi(argv[4]), arg5 = atoi(argv[5]);
    SharedData sharedData;
    listaConsumidores = initListaProducto(listaConsumidores);
    FILE *file, *outputFile;


    // Verificación de la cantidad de argumentos
    if (argc != 6) {
        fprintf(stderr, "Error: Número incorrecto de argumentos.\n");
        return -1;
    }

    //Verificación parámetros
    if (esCadena(argv[3]) || arg3 <= 0 || arg3 > 5000) {
        fprintf(stderr, "Error: T debe ser un entero positivo menor o igual a 5000.\n");
        return -1;
    }
    if (esCadena(argv[4]) || arg4 <= 0 || arg4 > 7) {
        fprintf(stderr, "Error: P debe ser un entero positivo menor o igual a 7.\n");
        return -1;
    }
    if (esCadena(argv[5]) || arg5 <= 0 || arg5 > 1000) {
        fprintf(stderr, "Error: C debe ser un entero positivo menor o igual a 1000.\n");
        return -1;
    }
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

    sem_init(semaforoFichero, 0, 1);
    sem_init(semaforoBuffer, 0, 1);
    sem_init(semaforoLista, 0, 1);


    // Crear hilo del proveedor
    pthread_t proveedorThread;
    pthread_create(&proveedorThread, NULL, (void *) proveedorFunc, &sharedData);

    // Crear hilo del consumidor
    pthread_t consumidorThread;
    pthread_create(&consumidorThread, NULL, (void *) consumidorFunc, &sharedData);

    // Esperar a que los hilos terminen
    pthread_join(proveedorThread, NULL);
    pthread_join(consumidorThread, NULL);


    printf_s("Hola");

    // Facturador
    facturador(&sharedData);

    printf_s("Hola");


    // Destruir semáforos y liberar memoria
    sem_destroy(&semaforoFichero);
    sem_destroy(&semaforoBuffer);
    sem_destroy(&semaforoLista);
    fclose(outputFile);
    free(buffer);
    return 0;
}

void proveedorFunc(void *data) {
    SharedData *sharedData = (SharedData *) data;
    FILE *file, *outputFile;
    char c;
    int productosLeidos = 0, productosValidos = 0, productosInvalidos = 0, proveedorID = 0;
    TotalProductos totalProductos = {{0}};

    // Abrir el archivo de entrada del proveedor
    file = fopen(sharedData->ruta, "r");

    // Leer y procesar productos del archivo
    while ((c = fgetc(file)) != EOF) {
        productosLeidos++;

        if (esTipoValido(c)) {
            // Procesar productos válidos
            // Incluir semáforo de exclusión mutua para la escritura en el búfer
            sem_wait(&semaforoFichero);
            // Escribir en el búfer
            buffer[sharedData->in].tipo = c;
            buffer[sharedData->in].proveedorID = proveedorID;
            sharedData->in = (sharedData->in + 1) % sharedData->T;
            // Incrementar contador de productos válidos
            productosValidos++;
            // Actualizar registro de productos
            totalProductos.total[c - 'a']++;
            // Liberar semáforo de exclusión mutua
            sem_post(&semaforoFichero);
        } else {
            // Procesar productos inválidos
            productosInvalidos++;
        }
    }
    fclose(file); // Cerrar el archivo

    // Escribir resultados en el archivo de salida
    outputFile = fopen(sharedData->fichDestino, "w");
    if (outputFile == NULL) {
        fprintf(stderr, "Error al abrir el archivo de salida del proveedor %d.\n", sharedData->P);
        free(buffer);
        return;
    }

    fprintf(outputFile, "Proveedor: %d\n", sharedData->P);
    fprintf(outputFile, "Productos procesados: %d\n", productosLeidos);
    fprintf(outputFile, "Productos Inválidos: %d\n", productosInvalidos);
    fprintf(outputFile, "Productos Válidos: %d. De los cuales se han insertado:\n", productosValidos);

    for (char tipo = 'a'; tipo <= 'j'; tipo++) {
        fprintf(outputFile, "%d de tipo \"%c\".\n", totalProductos.total[tipo - 'a'], tipo);
    }

    // Cerrar el archivo de salida
    fclose(outputFile);

    // Avisar al consumidor que ha terminado
    pthread_cond_signal(&cond_proveedor);
}

void consumidorFunc(void *data) {
    SharedData *sharedData = (SharedData *) data;
    int consumidorID = 0, bandera = 0, numeroProductosConsumidosPorTipo['j' - 'a' + 1], numeroProductosConsumidos = 0;
    Producto productoConsumido;


    ConsumidorInfo* consumidor = (ConsumidorInfo*)malloc(sizeof(ConsumidorInfo));
    consumidor->consumidorID = consumidorID;
    consumidor->productosConsumidos = 0;
    memset(consumidor->productosConsumidosPorTipo, 0, sizeof(consumidor->productosConsumidosPorTipo));
    consumidor->siguiente = NULL;

    // Consumir productos del búfer
    while (bandera != 1) {

        // Leer del buffer
        sem_wait(semaforoContadorBuffer);
        sem_wait(semaforoBuffer);
        productoConsumido = buffer[contBuffer];
        if (contBuffer+1 >= sharedData->T || esTipoValido(productoConsumido.tipo)) {bandera = 1;}
        sem_post(semaforoBuffer);

        numeroProductosConsumidos++; // Incremento de contador general
        numeroProductosConsumidosPorTipo[productoConsumido.tipo - 'a']++; // Incremento de contador del tipo correspondiente

        //Se actualiza el contador del buffer
        contBuffer = (contBuffer + 1) % sharedData->T;
        sem_post(semaforoContadorBuffer);
    }

    // Escribe en la lista el producto leido del buffer (lentamente perdiendo la cordura)
    sem_wait(semaforoLista);
    listaConsumidores = agregarConsumidor(listaConsumidores, numeroProductosConsumidos, numeroProductosConsumidosPorTipo, productoConsumido.proveedorID); //hay que pasarle prodConsPorTipo
    sem_post(semaforoLista);

    // Salir de la función del consumidor
    pthread_exit(NULL);
}


void facturador(SharedData* sharedData) {
    FILE *outputFile;
    int arrayConsumidores[sharedData->C]['j' - 'a' + 1], productosPorContador[sharedData->C];

    printf_s("Hola");
    sem_wait(semaforoLista);

    // Agregar a la lista
/*    while (listaConsumidores != NULL) {
        printf("%c,%d\n", listaConsumidores->tipo, listaConsumidores->proveedorID);
        // Contadores de productos, uno por tipos y uno por consumidores
        arrayConsumidores[listaConsumidores->proveedorID][listaConsumidores->tipo - 'a']++;
        productosPorContador[listaConsumidores->proveedorID]++;
        listaConsumidores = listaConsumidores->siguiente;
    }*/
    outputFile = fopen(sharedData->fichDestino, "a");


    for (int i = 0; i < sharedData->C; ++i) {
        fprintf(outputFile, "Consumidor: %d\n", i);
        fprintf(outputFile, "Productos Consumidos: %d. De los cuales:\n", productosPorContador[i]);

        for (int j = 0; j < sizeof(arrayConsumidores[0]); ++j) {
            fprintf(outputFile, "Producto tipo \"%c\": %d\n", (char)(j+'a'), arrayConsumidores[i][j]);
        }

    }





}

bool esTipoValido(char c) {
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

ConsumidorInfo *initListaProducto(ConsumidorInfo *lista){
    lista = NULL;
    return lista;
}

ConsumidorInfo *agregarConsumidor(ConsumidorInfo *producto, int productosConsumidos, int *productosConsumidosPorTipo, int ID) {
    ConsumidorInfo *nuevoConsumidor;
    ConsumidorInfo *aux;
    nuevoConsumidor = (ConsumidorInfo *) malloc(sizeof(ConsumidorInfo));
    nuevoConsumidor->productosConsumidos = productosConsumidos;
    memcpy(nuevoConsumidor->productosConsumidosPorTipo, productosConsumidosPorTipo, sizeof(nuevoConsumidor->productosConsumidosPorTipo));
    nuevoConsumidor->consumidorID = ID;
    nuevoConsumidor->siguiente = NULL;
    if (producto == NULL){
        producto = nuevoConsumidor;
    } else {
        aux = producto;
        while (aux->siguiente != NULL) {
            aux = aux->siguiente;
        }
        aux->siguiente = nuevoConsumidor;
    }
    return producto;
}