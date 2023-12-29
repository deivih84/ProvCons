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
#define MAX_PROVEEDORES 7
#define MAX_CONSUMIDORES 1000

typedef struct {
    char tipo;
    int proveedorID;
} Producto;

// Estructura para mantener el registro del total de productos de cada tipo
typedef struct {
    int total['j' - 'a' + 1];
} TotalProductos;

// Estructura para almacenar la información del consumidor
typedef struct nodo { //CAMBIAR, HACE FALTA LA INFO DE CADA PRODUCTOR EN UN ARRAY, no un nodo por productor
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
sem_t semaforoFichero, semaforoBuffer, semaforoLista, semaforoProdBuffer, semaforoConsBuffer, hayDato, CONSUMIDORTERMINADO;
Producto *buffer;
char *path, *fichDest;
int itProdBuffer = 0, itConsBuffer = 0, contProdsAcabados = 0, tamBuffer, nProveedores, nConsumidores;
ConsumidorInfo *listaConsumidores;

// Declaración de funciones :D
void proveedorFunc(const int *arg);
void consumidorFunc(const int *arg);
ConsumidorInfo *initListaProducto(ConsumidorInfo *lista);
ConsumidorInfo *agregarConsumidor(ConsumidorInfo *nodo, int productosConsumidos, int productosConsumidosPorTipo[nProveedores][10], int ID);
void facturadorFunc(SharedData* sharedData);
bool esTipoValido(char c);
bool esCadena(char *string);


int main(int argc, char *argv[]) {
    path = strdup(argv[1]);
    fichDest = strdup(argv[2]);
    printf("ññññ %s ññññ",fichDest);
    SharedData sharedData;
    listaConsumidores = initListaProducto(listaConsumidores);
    int *arg[MAX_PROVEEDORES];
    FILE *file, *outputFile;
    pthread_t proveedorThread[nProveedores], consumidorThread[nConsumidores], facturadorThread;

    // Verificación de la cantidad de argumentos
    if (argc != 6) {
        fprintf(stderr, "Error: Número incorrecto de argumentos.\n");
        return -1;
    }

    //Verificación parámetros

    //Verificar si los parámetros pasados son válidos o no. Si no, se pasa -1 para salir en el próximo if
    tamBuffer = (!esCadena(argv[3])) ? atoi(argv[3]) : -1;
    nProveedores = (!esCadena(argv[4])) ? atoi(argv[4]) : -1;
    nConsumidores = (!esCadena(argv[5])) ? atoi(argv[5]) : -1;

    if (tamBuffer <= 0 || tamBuffer > 5000) {
        fprintf(stderr, "Error: T debe ser un entero positivo menor o igual a 5000.\n");
        return -1;
    }
    if (nProveedores <= 0 || nProveedores > MAX_PROVEEDORES) {
        fprintf(stderr, "Error: P debe ser un entero positivo menor o igual a %d.\n", MAX_PROVEEDORES);
        return -1;
    }
    if (nConsumidores <= 0 || nConsumidores > MAX_CONSUMIDORES) {
        fprintf(stderr, "Error: C debe ser un entero positivo menor o igual a %d.\n", MAX_CONSUMIDORES);
        return -1;
    }

    //AQUÍ HABRÁ QUE MODIFICAR COSAS PARA MÁS DE UN PROVEEDOR
    //sprintf(path, "%s\\proveedor%d.dat", argv[1], 0);
//    file = fopen(path, "r");
//    if (file == NULL) {
//        fprintf(stderr, "Error al abrir el archivo de entrada del proveedor %d.\n", sharedData.P);
//    }


    sharedData.ruta = path; // Ruta de los archivos de entrada
    sharedData.fichDestino = argv[2]; // Nombre del fichero destino.
    sharedData.T = tamBuffer; // Tamaño del búfer circular.
    sharedData.P = nProveedores; // Número total de proveedores.
    sharedData.C = nConsumidores; // Número total de clientes.


    // Crear estructuras de datos compartidas
    sharedData.in = 0;
    sharedData.out = 0;

    buffer = malloc((tamBuffer) * sizeof(Producto)); // CALLOOOOOC
    if (buffer == NULL) {
        free(buffer);
        fprintf(stderr, "Error al asignar memoria para el búfer compartido.\n");
        return -1;
    }

    //Prueba apertura de TODOS los ficheros para proveedores
    for (int i = 0; i < nProveedores; i++) {
        sprintf(path, "%s\\proveedor%d.dat", argv[1], i);
        file = fopen(path, "r");
        if (file == NULL) {
            fprintf(stderr, "Error al abrir el archivo de entrada del proveedor %d.\n", i);
            fclose(file);
            free(buffer);
            return -1;
        }
        fclose(file);
    }
    // Creado o limpieza del fichero de salida.
    path = strdup(argv[1]);
    fichDest = strdup(argv[2]);

    file = fopen(fichDest, "w");
    if (file == NULL) {
        fprintf(stderr, "Error al abrir el archivo salida.");
        fclose(file);
        free(buffer);
        return -1;
    }
    fclose(file);

    fclose(file);

    sem_init(&semaforoFichero, 0, 1);
    sem_init(&semaforoBuffer, 0, 1);
    sem_init(&semaforoLista, 0, 1);
    sem_init(&semaforoProdBuffer, 0, 1);
    sem_init(&semaforoConsBuffer, 0, 1);
    sem_init(&hayDato, 0, 0);


    // Crear hilos proveedor
    for (int i = 0; i < nProveedores; i++) {
        printf("!!%d!!", i);
        pthread_create(&proveedorThread[i], NULL, (void *) proveedorFunc, &i);
        printf("%s", "Hilo Proveedor lanzado.\n");
    }


    // Crear hilos consumidor
    for (int i = 0; i < nConsumidores; i++) {
        printf("??%d??", i);
        pthread_create(&consumidorThread[i], NULL, (void *) consumidorFunc, &i);
        printf("%s", "Hilo Consumidor lanzado.\n");
    }


    // Esperar a que los hilos terminen
    for (int i = 0; i < nProveedores; ++i) {
        pthread_join(proveedorThread[i], NULL);
        printf("             FiNP          ");
    }

    for (int i = 0; i < nConsumidores; ++i) {
        pthread_join(consumidorThread[i], NULL);
        printf("            FInC          ");
    }


    // Crear hilo del facturadorFunc
    pthread_create(&facturadorThread, NULL, (void *) facturadorFunc, &sharedData);
    pthread_join(facturadorThread, NULL);


    printf("%s", "Hilo Facturador lanzado.\n");



    // Destruir semáforos y liberar memoria
    sem_destroy(&semaforoFichero);
    sem_destroy(&semaforoBuffer);
    sem_destroy(&semaforoLista);
    sem_destroy(&semaforoProdBuffer);
    sem_destroy(&semaforoConsBuffer);
    sem_destroy(&hayDato);

    free(buffer);
    return 0;
}

void proveedorFunc (const int *arg) {
    FILE *file, *outputFile;
    bool bandera = true;
    char c, *fichPath = calloc(255, sizeof(char));
    int productosLeidos = 0, productosValidos = 0, productosNoValidos = 0, proveedorID = *arg;
    TotalProductos totalProductos = {{0}};



    // Abrir el archivo de entrada del proveedor
//    snprintf(fichPath, 255, "%s\\proveedor%d.dat", path, 0);
    sprintf(fichPath, "%s\\proveedor%d.dat", path, proveedorID);

    printf("%s  ", fichPath);
    file = fopen(fichPath, "r");

    if (file == NULL){ printf("AAAAAAA");}

    // Leer y procesar productos del archivo
    while (bandera) {
        c = (char) fgetc(file);

        if (esTipoValido(c)) {
            printf(" _%c%d_ ", c, proveedorID);

            // Semáforos para escritura en el búfer
            sem_wait(&semaforoBuffer);
            sem_wait(&semaforoProdBuffer); /////////////////////QUITAR PRODBUFFER CON EL DE BUFFER YA VALE

            // Escribir en el búfer
            buffer[itProdBuffer].tipo = c;
            buffer[itProdBuffer].proveedorID = proveedorID;
            sem_post(&hayDato); //////SEMAFORO hayDato


//            printf("%c ", buffer[sharedData->in].tipo); ////

            itProdBuffer = (itProdBuffer + 1) % tamBuffer;

            sem_post(&semaforoProdBuffer);
            sem_post(&semaforoBuffer);


            // Incrementar contador de productos válidos
            productosValidos++;
            // Actualizar registro de productos
            totalProductos.total[c - 'a']++;
            productosLeidos++;

        } else if (c == EOF){ // Si es el final del fichero pone una 'F' para decir que ha acabado.
            sem_wait(&semaforoProdBuffer);
            sem_wait(&semaforoBuffer);

            buffer[itProdBuffer].tipo = 'F';
            buffer[itProdBuffer].proveedorID = proveedorID;
            itProdBuffer = (itProdBuffer + 1) % tamBuffer;

            printf(" _____________FIN de lineaaaa en %d___________ ", itProdBuffer);
            sem_post(&hayDato); //////SEMAFORO hayDato


            sem_post(&semaforoBuffer);
            sem_post(&semaforoProdBuffer);
            bandera = false;

        } else {
            // Procesar productos inválidos
            productosNoValidos++;
            productosLeidos++;

        }
    }
    fclose(file); // Cerrar el archivo
    printf("ACAB PROV:%d ", proveedorID);


    sem_wait(&semaforoFichero);
    // Escribir resultados en el archivo de salida

    sprintf(fichPath, "%s\\%s", path, fichDest);

    outputFile = fopen(fichDest, "a");
    if (outputFile == NULL) {
        fprintf(stderr, "Error al abrir el archivo de salida del proveedor %d.\n", 0);
        free(buffer);
        return;
    }

    fprintf(outputFile, "Proveedor: %d.\n", proveedorID);
    fprintf(outputFile, "   Productos procesados: %d.\n", productosLeidos);
    fprintf(outputFile, "   Productos Inválidos: %d.\n", productosNoValidos);
    fprintf(outputFile, "   Productos Válidos: %d. De los cuales se han insertado:\n", productosValidos);

    for (char tipo = 'a'; tipo <= 'j'; tipo++) {
        fprintf(outputFile, "     %d de tipo \"%c\".\n", totalProductos.total[tipo - 'a'], tipo);
    }
    sem_post(&semaforoFichero);


    // Cerrar el archivo de salida
    fclose(outputFile);
}

void consumidorFunc(const int *arg) {
    int bandera = 0, numProdsConsumidosPorProveedor[nProveedores]['j' - 'a' + 1], numProdsConsumidos = 0, consumidorID = *arg;
    Producto productoConsumido;

    // Incializar numProdsConsumidosPorProveedor[] //No sabes lo que hay en la memoria cuando vas a escribir.
    printf("SEXO|%d|SEXO", consumidorID);
    for (int i = 0; i < nProveedores; i++) {
        for (int j = 0; j < 10; j++) {
            numProdsConsumidosPorProveedor[i][j] = 0;
        }
    }


    // Consumir productos del búfer
    while (bandera != 1) {

        /////////////////////////////
        sem_wait(&hayDato);
        /////////////////////////////


        // Leer del buffer
        sem_wait(&semaforoBuffer);
        //sem_wait(&semaforoConsBuffer);

        productoConsumido = buffer[itConsBuffer];
        itConsBuffer = (itConsBuffer + 1) % tamBuffer;

        //sem_post(&semaforoConsBuffer);

        //printf("|%d|", itConsBuffer);
        //printf("_%c ", productoConsumido.tipo);

        if (productoConsumido.tipo != 'F') {
            numProdsConsumidos++; // Incremento de contador general
            numProdsConsumidosPorProveedor[productoConsumido.proveedorID][productoConsumido.tipo - 'a']++; // Incremento de contador del tipo correspondiente
        }


        //Se da por finalizada la ejecución de un productor

        bandera = (contProdsAcabados == nProveedores) ? 1 : 0;

        //sem_wait(&semaforoConsBuffer);

        if (buffer[itConsBuffer].tipo == 'F') {
            contProdsAcabados++;
        }
        //sem_post(&semaforoConsBuffer);
        sem_post(&semaforoBuffer);

    }

    // Escribe en la lista el producto leido del buffer
    printf(" _A_ ");
    sem_wait(&semaforoLista);
    listaConsumidores = agregarConsumidor(listaConsumidores, numProdsConsumidos, numProdsConsumidosPorProveedor, consumidorID); //hay que pasarle prodConsPorTipo
    sem_post(&semaforoLista);
}


void facturadorFunc(SharedData* sharedData) {
    FILE *outputFile;
    int i = 0;
    printf("PRINT FACTURADOR");
    char *fichPath = calloc(255, sizeof(char));
    sprintf(fichPath, "%s\\%s", path, fichDest);


    sem_wait(&semaforoFichero);
    outputFile = fopen(fichPath, "a");
    sem_wait(&semaforoLista);

    // Agregar a la lista
    while (listaConsumidores != NULL) {
        // Contadores de productos, uno por tipos y uno por consumidores

        fprintf(outputFile, "\nCliente consumidor: %d\n", i);

        fprintf(outputFile, "  Productos consumidos: %d. De los cuales:\n", listaConsumidores->productosConsumidos);
        for (int j = 0; j < ('j' - 'a' + 1); ++j) {
            fprintf(outputFile, "     Producto tipo \"%c\": %d\n", (char) (j + 'a'), listaConsumidores->productosConsumidosPorTipo[j]);
        }
        listaConsumidores = listaConsumidores->siguiente;
        i++;
    }
    sem_post(&semaforoLista);
    sem_post(&semaforoFichero);

    fclose(outputFile);
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


ConsumidorInfo *agregarConsumidor(ConsumidorInfo *nodo, int productosConsumidos, int productosConsumidosPorTipo[nProveedores][10], int ID) {
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