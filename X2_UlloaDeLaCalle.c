//
// Autores Ivan Ulloa Gómez 12343449Q y David de la Calle Azahares 71231179H del grupo de prácticas L2
//
// Hemos estado haciéndolo en Clion y Replit y en ninguno de los dos nos ha dado problema, si embargo al probar en jair
// nos da una violación de segmento, suponemos que por usar strdup :(
// Aparte de eso funciona bien para varios proveedores y consumidores
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
#define NPRODUCTOS ('j' - 'a' + 1)

typedef struct {
    char tipo;
    int proveedorID;
} Producto;

// Estructura para almacenar la información del consumidor
typedef struct nodo {
    int consumidorID;
    int productosConsumidos;
    int productosConsumidosPorTipo[7]['j' - 'a' + 1];
    struct nodo *siguiente;
} ConsumidorInfo;

// Variables GLOBALES :)
sem_t semaforoFichero, semaforoBuffer, semaforoLista, hayEspacio, hayDato;
Producto *buffer;
char *path, *fichDest;
int itProdBuffer = 0, itConsBuffer = 0, contProvsAcabados = 0, tamBuffer, nProveedores, nConsumidores;
ConsumidorInfo *listaConsumidores = NULL;

// Declaración de funciones
void* proveedorFunc(void *arg);

void* consumidorFunc(void *arg);

ConsumidorInfo *agregarConsumidor(ConsumidorInfo *nodo, int productosConsumidos, int productosConsumidosPorTipo[nProveedores][10], int ID);

void facturadorFunc();

bool esTipoValido(char c);

void copiarValor(int *destino, int origen);

bool esCadena(char *string);


int main(int argc, char *argv[]) {
    char *dirpath = strdup(argv[1]); /////?
    char *fileName = strdup(argv[2]); /////?
    int argHilosP[MAX_PROVEEDORES], argHilosC[MAX_PROVEEDORES];
    int k;
    FILE *file;

    // Verificación de la cantidad de argumentos
    if (argc != 6) {
        fprintf(stderr, "Error: Número incorrecto de argumentos.\n");
        return -1;
    }

    // Verificación parámetros

    // Verificar si los parámetros pasados son válidos o no. Si no, se pasa -1 para salir en el próximo if
    tamBuffer = (!esCadena(argv[3])) ? atoi(argv[3]) : -1;
    nProveedores = (!esCadena(argv[4])) ? atoi(argv[4]) : -1;
    nConsumidores = (!esCadena(argv[5])) ? atoi(argv[5]) : -1;

    pthread_t proveedorThread[nProveedores], consumidorThread[nConsumidores], facturadorThread;

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

    buffer = calloc(tamBuffer, sizeof(Producto));
    if (buffer == NULL) {
        free(buffer);
        fprintf(stderr, "Error al asignar memoria para el búfer compartido.\n");
        return -1;
    }

    // Prueba apertura de TODOS los ficheros para proveedores
    for (int i = 0; i < nProveedores; i++) {
        // Formatear cadena
        sprintf(dirpath, "%s\\proveedor%d.dat", argv[1], i);
        file = fopen(dirpath, "r");
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

    sem_init(&semaforoFichero, 0, 1);
    sem_init(&semaforoBuffer, 0, 1);
    sem_init(&semaforoLista, 0, 1);
    sem_init(&hayEspacio, 0, tamBuffer);
    sem_init(&hayDato, 0, 0);


    // Crear hilos proveedor
    for (int i = 0; i < nProveedores; i++) {
        // Configurar los argumentos para el hilo actual
        argHilosP[i] = i;
        pthread_create(&proveedorThread[i], NULL, (void *) proveedorFunc, &argHilosP[i]);
        printf("Hilo Proveedor %d lanzado.\n", i);
    }

    // Crear hilos consumidor
    for (int j = 0; j < nConsumidores; j++) {
        argHilosC[j] = j;
        pthread_create(&consumidorThread[j], NULL, (void *) consumidorFunc, &argHilosC[j]);
        printf("Hilo Consumidor %d lanzado.\n", j);
    }


    // Esperar a que los hilos terminen
    for (int i = 0; i < nProveedores; i++) {
        pthread_join(proveedorThread[i], NULL);
    }
    for (int i = 0; i < nConsumidores; i++) {
        pthread_join(consumidorThread[i], NULL);
    }

    // Crear hilo del facturadorFunc
    pthread_create(&facturadorThread, NULL, (void *) facturadorFunc, NULL);
    pthread_join(facturadorThread, NULL);

    // Destruir semáforos y liberar memoria
    sem_destroy(&semaforoFichero);
    sem_destroy(&semaforoBuffer);
    sem_destroy(&semaforoLista);
    sem_destroy(&hayEspacio);
    sem_destroy(&hayDato);

    free(buffer);
    return 0;
}

// TODO pasar los argumentos como void (una lista de elementos para que no de core)
void* proveedorFunc(void *arg) {
    FILE *file, *outputFile;
    bool bandera = true;
    int productosLeidos = 0, productosValidos = 0, productosNoValidos = 0, proveedorID = *((int*) arg); ////////?
    int totalProductos[NPRODUCTOS];
    char c, *fichPath = calloc(255, sizeof(char));
    if (buffer == NULL) {
        fprintf(stderr, "Error al asignar memoria para el búfer compartido.\n");
        free(buffer);
        exit(-1);
    }

    // Inicializar totalProductos (Por si acaso)
    for (int i = 0; i < NPRODUCTOS; ++i) {
        totalProductos[i] = 0;
    }

    // Formatear cadena
    sprintf(fichPath, "%s\\proveedor%d.dat", path, proveedorID);
    // Abrir el archivo de entrada del proveedor
    file = fopen(fichPath, "r");
    if (file == NULL) {
        fprintf(stderr, "Error al abrir el archivo salida.");
        fclose(file);
        exit(-1);
    }

    // Leer y procesar productos del archivo
    while (bandera) { //////////break?
        sem_wait(&hayEspacio);
        c = (char) fgetc(file);

        if (esTipoValido(c)) {
            // Semáforos para escritura en el búfer
            sem_wait(&semaforoBuffer);

            // Escribir en el búfer
            buffer[itProdBuffer].tipo = c;
            buffer[itProdBuffer].proveedorID = proveedorID;
            sem_post(&hayDato);

            itProdBuffer = (itProdBuffer + 1) % tamBuffer;

            sem_post(&semaforoBuffer);

            // Incrementar contador de productos válidos
            productosValidos++;
            totalProductos[c - 'a']++;
            productosLeidos++;

        } else if (c == EOF) { // Si es el final del fichero pone una 'F' para decir que ha acabado.
            sem_wait(&semaforoBuffer);

            buffer[itProdBuffer].tipo = 'F';
            buffer[itProdBuffer].proveedorID = proveedorID;
            itProdBuffer = (itProdBuffer + 1) % tamBuffer;

            sem_post(&hayDato);
            sem_post(&semaforoBuffer);
            bandera = false;

        } else {
            // Procesar productos no válidos
            productosNoValidos++;
            productosLeidos++;

        }
    }
    fclose(file); // Cerrar el archivo

    // Escribir resultados en el fichero de salida
    sem_wait(&semaforoFichero);

    // Formatear cadena
    sprintf(fichPath, "%s\\%s", path, fichDest);

    outputFile = fopen(fichDest, "a");
    if (outputFile == NULL) {
        fprintf(stderr, "Error al abrir el archivo de salida del proveedor %d.\n", 0);
        fclose(outputFile);
        exit(-1);
    }

    fprintf(outputFile, "Proveedor: %d.\n", proveedorID);
    fprintf(outputFile, "   Productos procesados: %d.\n", productosLeidos);
    fprintf(outputFile, "   Productos Inválidos: %d.\n", productosNoValidos);
    fprintf(outputFile, "   Productos Válidos: %d. De los cuales se han insertado:\n", productosValidos);

    for (char tipo = 'a'; tipo <= 'j'; tipo++) {
        fprintf(outputFile, "     %d de tipo \"%c\".\n", totalProductos[tipo - 'a'], tipo);
    }
    sem_post(&semaforoFichero);

    // Cerrar el archivo de salida
    free(fichPath);
    fclose(outputFile);
    return NULL; ///////////hace falta el return?
}

void *consumidorFunc(void *arg) {
    bool bandera = true;
    int numProdsConsumidosPorProveedor[nProveedores]['j' - 'a' + 1], numProdsConsumidos = 0, consumidorID = *((int*) arg);
    Producto productoConsumido;

    // Incializar numProdsConsumidosPorProveedor[][] No sabes lo que hay en la memoria cuando vas a escribir.
    for (int i = 0; i < nProveedores; i++) {
        for (int j = 0; j < 10; j++) {
            numProdsConsumidosPorProveedor[i][j] = 0;
        }
    }

    // Consumir productos del búfer
    while (bandera) {
        sem_wait(&hayDato);

        // Leer del buffer
        sem_wait(&semaforoBuffer);
        productoConsumido = buffer[itConsBuffer];

        if ('a' <= productoConsumido.tipo && productoConsumido.tipo < ('a' + NPRODUCTOS)) { //Está entre 'a' y 'j'
            numProdsConsumidos++; // Incremento de contador general
            numProdsConsumidosPorProveedor[productoConsumido.proveedorID][productoConsumido.tipo - 'a']++; // Incremento de contador del tipo correspondiente
        } else if (productoConsumido.tipo == ' ') { ///////////?
            // Para los productos que ya han sido consumidos
        } else if (productoConsumido.tipo == 'F'){
            contProvsAcabados += 1;
        }

        // Significa que el producto ha sido consumido //////////?
        buffer[itConsBuffer].tipo = ' ';
        buffer[itConsBuffer].proveedorID = -1;


        itConsBuffer = (itConsBuffer + 1) % tamBuffer;

        sem_post(&semaforoBuffer);
        sem_post(&hayEspacio);

        printf("  Fin:%d,ID:%d  ", contProvsAcabados == nProveedores, consumidorID);
        bandera = (contProvsAcabados == nProveedores) ? false : true;
    }

    // Escribe en la lista el producto leido del buffer
    sem_wait(&semaforoLista);
    listaConsumidores = agregarConsumidor(listaConsumidores, numProdsConsumidos, numProdsConsumidosPorProveedor,consumidorID); //hay que pasarle prodConsPorTipo
    sem_post(&semaforoLista);
}


void facturadorFunc() {
    FILE *outputFile;
    int i = 0, proveedores[nProveedores], tipos[10], consumidores[nConsumidores], suma = 0, maximo = 0, cons = 0;
    char *fichPath = calloc(255, sizeof(char));
    if (buffer == NULL) {
        fprintf(stderr, "Error al asignar memoria para el búfer compartido.\n");
        free(buffer);
        exit(-1);
    }
    // Formatear cadena
    sprintf(fichPath, "%s\\%s", path, fichDest);

    outputFile = fopen(fichPath, "a");
    if (outputFile == NULL) {
        fprintf(stderr, "Error al abrir el archivo salida.");
        fclose(outputFile);
        exit(-1);
    }

    sem_wait(&semaforoLista);

    for (int k = 0; k < nConsumidores; ++k) { // Inicializar array consumidores
        consumidores[k] = 0;
    }
    // Agregar a la lista
    while (listaConsumidores != NULL) {
        for (int k = 0; k < nProveedores; ++k) { // Inicializar array proveedores
            proveedores[k] = 0;
        }
        for (int k = 0; k < 10; ++k) { // Inicializar array tipos
            tipos[k] = 0;
        }
        for (int j = 0; j < nProveedores; ++j) {
            for (int k = 0; k < 10; ++k) {
                proveedores[j] += listaConsumidores->productosConsumidosPorTipo[j][k];
            }
        }
        for (int j = 0; j < 10; ++j) { // Contar cuantos productos se han consumido por tipo entre todos los proveedores
            for (int k = 0; k < nProveedores; ++k) {
                tipos[j] += listaConsumidores->productosConsumidosPorTipo[k][j];
            }
        }
        consumidores[i] = listaConsumidores->productosConsumidos; // Para sacar el que más ha consumido más tarde

        fprintf(outputFile, "\nCliente consumidor: %d\n", i);
        fprintf(outputFile, "  Productos consumidos: %d. De los cuales:\n", listaConsumidores->productosConsumidos);

        for (int tipo = 0; tipo < ('j' - 'a' + 1); ++tipo) {
            fprintf(outputFile, "     Producto tipo \"%c\": %d\n", (char) (tipo + 'a'), tipos[tipo]);
        }
        listaConsumidores = listaConsumidores->siguiente;
        i++;
    }
    for (int j = 0; j < nProveedores; ++j) { // Total de productos
        suma += proveedores[j];
    }
    for (int j = 0; j < nConsumidores; j++) { // Hallar el consumidor que más a consumido.
        if (maximo <= consumidores[j]) {
            maximo = consumidores[j];
            cons = j;
        }
    }
    fprintf(outputFile, "\nTotal de productos consumidos: %d\n", suma);
    for (int j = 0; j < nProveedores; ++j) {
        fprintf(outputFile, "     %d del proveedor %d.\n", proveedores[j], j);
    }
    fprintf(outputFile, "Cliente consumidor que mas ha consumido: %d\n", cons);

    sem_post(&semaforoLista);
    free(fichPath);
    fclose(outputFile);
}

// Devuelve True si está entre a y j (incluidas)
bool esTipoValido(char c) { return (c >= 'a' && c <= 'j'); }

void copiarValor(int *destino, int origen) {
    *destino = origen;  // Aquí se copia el valor al que apunta el puntero destino
}


bool esCadena(char *cadena) {
    for (int i = 0; i < strlen(cadena); ++i) {
        if (!isdigit(cadena[i])) {
            return true; // Devuelve 1 si no es una cadena de dígitos
        }
    }
    return false; // Devuelve 0 si es una cadena de dígitos
}


ConsumidorInfo *agregarConsumidor(ConsumidorInfo *nodo, int productosConsumidos, int productosConsumidosPorTipo[nProveedores][10], int ID) {
    ConsumidorInfo *nuevoConsumidor;
    ConsumidorInfo *aux;
    nuevoConsumidor = (ConsumidorInfo *) calloc(1,sizeof(ConsumidorInfo));
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