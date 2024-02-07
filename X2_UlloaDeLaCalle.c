///
// Autores Ivan Ulloa Gómez 12343449Q y David de la Calle Azahares 71231179H del grupo de prácticas L2
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_COMMAND_LENGTH 800
#define MAX_ARGUMENTS 4
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
    int *productosConsumidosPorTipo;
    int *totalPorProveedor;
    struct nodo *siguiente;
} ConsumidorInfo;


typedef struct {
    int proveedorID;
    int tamBuffer;
    int nProveedores;
    int nConsumidores;
    char pathProv[255];
    FILE *file; // EL FICHERO ABIERTO.
} ArgsProvFact;
typedef struct {
    int consumidorID;
    int tamBuffer;
    int nProveedores;
    int nConsumidores;
} ArgsCons;

// Variables GLOBALES :)
sem_t semaforoFichero, semContC, semContP, hayEspacio, hayDato, adelanteFacturador, semContProveedorAcabado, semLista;
Producto *buffer;
int itProdBuffer = 0, itConsBuffer = 0, contProvsAcabados = 0;
ConsumidorInfo *nodoInicial = NULL, *nodoActual = NULL;

// Declaración de funciones
void* proveedorFunc(void *arg);

void* consumidorFunc(void *argsCont);

void* facturadorFunc(void *arg);

int esCadena(char *cadena);


int main(int argc, char *argv[]) {
    int nConsumidores, nProveedores, tamBuffer;
    char pathDest[255];
    FILE *fichProveedor, *ficheroSalida;
    pthread_t *proveedorThread, *consumidorThread, facturadorThread;
    ArgsProvFact *argsProv;
    ArgsCons *argsCons;

    // Verificación de la cantidad de argumentos
    if (argc != 6) {
        fprintf(stderr, "Error: Número incorrecto de argumentos.\n");
        exit(-1);}


    // Verificar si los parámetros pasados son válidos o no. Si no, se pasa -1 para salir en el próximo if
    tamBuffer = (!esCadena(argv[3])) ? atoi(argv[3]) : -1;
    nProveedores = (!esCadena(argv[4])) ? atoi(argv[4]) : -1;
    nConsumidores = (!esCadena(argv[5])) ? atoi(argv[5]) : -1;


    // COMPROBACIONES
    if (tamBuffer <= 0 || tamBuffer > 5000) {
        fprintf(stderr, "Error: T debe ser un entero positivo menor o igual a 5000.\n");
        exit(-1);}
    if (nProveedores <= 0 || nProveedores > MAX_PROVEEDORES) {
        fprintf(stderr, "Error: P debe ser un entero positivo menor o igual a %d.\n", MAX_PROVEEDORES);
        exit(-1);}
    if (nConsumidores <= 0 || nConsumidores > MAX_CONSUMIDORES) {
        fprintf(stderr, "Error: C debe ser un entero positivo menor o igual a %d.\n", MAX_CONSUMIDORES);
        exit(-1);}


    // RESERVA DINÁMICA
    if ((argsProv = calloc(nProveedores, sizeof(ArgsProvFact))) == NULL) {
        fprintf(stderr, "Error al asignar memoria para los hilos proveedores.\n");
        exit(-1);}
    if ((argsCons = calloc(nConsumidores, sizeof(ArgsCons))) == NULL) {
        fprintf(stderr, "Error al asignar memoria para los hilos proveedores.\n");
        exit(-1);}
    if ((proveedorThread = calloc(nProveedores, sizeof(pthread_t))) == NULL) {
        fprintf(stderr, "Error al asignar memoria para los hilos proveedores.\n");
        exit(-1);}
    if ((consumidorThread = calloc(nConsumidores, sizeof(pthread_t))) == NULL) {
        fprintf(stderr, "Error al asignar memoria para los hilos consumidores.\n");
        exit(-1);}
    if ((buffer = calloc(tamBuffer, sizeof(Producto))) == NULL) {
        fprintf(stderr, "Error al asignar memoria para el buffer compartido.\n");
        exit(-1);}


    sem_init(&hayDato, 0, 0);
    sem_init(&hayEspacio, 0, tamBuffer);
    sem_init(&semContP, 0, 1); // Semáforo para el contador de proveedores
    sem_init(&semContC, 0, 1); // Semáforo para el contador de consumidores
    sem_init(&semContProveedorAcabado, 0, 1);
    sem_init(&semaforoFichero, 0, 1);
    sem_init(&adelanteFacturador, 0, 0);
    sem_init(&semLista, 0, 1);



    // APERTURA DE FICHERO DESTINO
    sprintf(pathDest, "%s/%s", argv[1], argv[2]);
    if ((ficheroSalida = fopen(pathDest, "w")) == NULL) {
        fprintf(stderr, "Error al abrir el archivo salida.");
        exit(-1);}



    // PREPARACIÓN DE ARGUMENTOS
    for (int i = 0; i < nProveedores; i++) { // Argumentos Proveedores
        argsProv[i].proveedorID = i;
        argsProv[i].file = ficheroSalida;
        argsProv[i].tamBuffer = tamBuffer;
        argsProv[i].nProveedores = contProvsAcabados = nProveedores;
        argsProv[i].nConsumidores = nConsumidores;

        sprintf(argsProv[i].pathProv, "%s/proveedor%d.dat", argv[1], i);
        if ((fichProveedor = fopen(argsProv[i].pathProv, "r")) == NULL) { // Prueba de apertura de los proveedor.dat
            fprintf(stderr, "Error al abrir el archivo de entrada del proveedor %d.\n", 0);
            exit(-1);}
        fclose(fichProveedor);
    }
    for (int i = 0; i < nConsumidores; i++) { // Argumentos Consumidores
        argsCons[i].consumidorID = i;
        argsCons[i].nProveedores = nProveedores;
        argsCons[i].tamBuffer = tamBuffer;
        argsCons[i].nConsumidores = nConsumidores;
    }



    // LANZAMIENTO DE HILOS
    for (int i = 0; i < nProveedores; i++) {
        pthread_create(&proveedorThread[i], NULL, (void *) proveedorFunc, (void *)&argsProv[i]);
    }
    for (int i = 0; i < nConsumidores; i++) {
        pthread_create(&consumidorThread[i], NULL, (void *) consumidorFunc, (void *)&argsCons[i]);
    }
    pthread_create(&facturadorThread, NULL, (void *) facturadorFunc, (void *)&argsProv[0]);


    // Esperar a que los hilos terminen
    for (int i = 0; i < nProveedores; i++) {
        pthread_join(proveedorThread[i], NULL); // Proveedor
    }
    for (int i = 0; i < nConsumidores; i++) { // Consumidor
        pthread_join(consumidorThread[i], NULL);
    }
    pthread_join(facturadorThread, NULL); // Facturador



    fclose(ficheroSalida);

    // Destruir semáforos y liberar memoria
    sem_destroy(&semaforoFichero);
    sem_destroy(&semContP);
    sem_destroy(&semContC);
    sem_destroy(&hayEspacio);
    sem_destroy(&hayDato);
    sem_destroy(&adelanteFacturador);
    sem_destroy(&semContProveedorAcabado);
    sem_destroy(&semLista);

    free(buffer);
    free(proveedorThread);
    free(consumidorThread);

    printf("\033[1;32m\nEjecutado con éxito para %d proveedores y %d consumidores.\033[0m\n", nProveedores, nConsumidores);
    printf("Se ha creado un fichero de nombre \033[1;36m%s\033[0m con los resultados en %s\n\n", argv[2], argv[1]);
}

void* proveedorFunc(void *arg) {
    int productosLeidos = 0, productosValidos = 0, indiceProveedor, *totalProductos;
    char c;
    FILE *fichProveedor;
    ArgsProvFact *args = (ArgsProvFact *) arg;

    totalProductos = calloc(NPRODUCTOS, sizeof(Producto));
    if (totalProductos == NULL) { // No haría falta la mem.Dinámica pues NPRODUCTOS se conoce en compilación
        fprintf(stderr, "Error al asignar memoria para el array totalProductos.\n");
        exit(-1);}


    // APERTURA FICHERO
    if ((fichProveedor = fopen(args->pathProv, "r")) == NULL) {
        fprintf(stderr, "Error al abrir el archivo de entrada del proveedor %d.\n", 0);
        exit(-1);}


    // BUCLE PRINCIPAL
    while ((c = fgetc(fichProveedor)) != EOF) { // Saldrá cuando se acabe el fichero
        if ((c >= 'a' && c <= 'j')) {
            sem_wait(&hayEspacio);

            // SECCIÓN CRÍTICA del índice de Proveedores
            sem_wait(&semContP);
            indiceProveedor = itProdBuffer;
            itProdBuffer = (itProdBuffer + 1) % args->tamBuffer;
            sem_post(&semContP);

            // Escritura en el búfer
            buffer[indiceProveedor].tipo = c;
            buffer[indiceProveedor].proveedorID = args->proveedorID;
            sem_post(&hayDato);

            // Operaciones para más tarde
            productosValidos++;
            totalProductos[c - 'a']++;
        }
        productosLeidos++;
    }
    fclose(fichProveedor);


    // SECCIÓN CRÍTICA del contador de proveedores acabados
    sem_wait(&semContProveedorAcabado);
    if (--contProvsAcabados == 0) { // Si entras aquí eres el último proveedor con vida
        sem_wait(&hayEspacio);

        sem_wait(&semContP); // Actualizar el contador para poner una F
        indiceProveedor = itProdBuffer;
        itProdBuffer = (itProdBuffer + 1) % args->tamBuffer;
        sem_post(&semContP);

        buffer[indiceProveedor].tipo = 'F';
        sem_post(&hayDato);
    }
    sem_post(&semContProveedorAcabado);


    // SECCIÓN CRÍTICA del fichero
    sem_wait(&semaforoFichero);
    fprintf(args->file, "Proveedor: %d.\n", args->proveedorID);
    fprintf(args->file, "   Productos procesados: %d.\n", productosLeidos);
    fprintf(args->file, "   Productos Inválidos: %d.\n", productosLeidos - productosValidos);
    fprintf(args->file, "   Productos Válidos: %d. De los cuales se han insertado:\n", productosValidos);
    for (char tipo = 'a'; tipo <= 'j'; tipo++) {
        fprintf(args->file, "     %d de tipo \"%c\".\n", totalProductos[tipo - 'a'], tipo);
    }
    sem_post(&semaforoFichero);


    free(totalProductos);
    pthread_exit(NULL);
}

void *consumidorFunc(void *argsCont) {
    int bandera = 1, numProdsConsumidos = 0, *numProdsConsumidosPorTipo, *numProdsConsumidosPorProveedor;
    Producto producto;
    ConsumidorInfo *nuevoConsumidor, *aux;
    ArgsCons *args = (ArgsCons *)argsCont;


    if ((numProdsConsumidosPorProveedor = calloc(args->nProveedores, sizeof(int))) == NULL) {
        fprintf(stderr, "Error al reservar memoria para las filas.\n");
        exit(-1);}
    if ((numProdsConsumidosPorTipo = calloc(NPRODUCTOS, sizeof(int))) == NULL) {
        fprintf(stderr, "Error al reservar memoria para las filas.\n");
        exit(-1);}
    if ((nuevoConsumidor = calloc(1,sizeof(ConsumidorInfo))) == NULL) {
        fprintf(stderr, "Error al asignar memoria para el nuevo nodo de la lista enlazada.\n");
        exit(-1);}


    // Consumir productos del búfer
    while (bandera) { //contProvsAcabados != nProveedores
        sem_wait(&hayDato);

        // Sección crítica
        sem_wait(&semContC);
        producto = buffer[itConsBuffer];
        if (producto.tipo  == 'F') { // Si es F les digo al resto de consumidores que hay dato y no modifico el contador para que también lean la F
            sem_post(&semContC); //Fin sección crítica
            sem_post(&hayDato);
            bandera = 0;
        }
        else {
            itConsBuffer = (itConsBuffer + 1) % args->tamBuffer; // Incrementa contador buffer
            sem_post(&semContC);

            numProdsConsumidos++; // Incremento de contador general
            numProdsConsumidosPorProveedor[producto.proveedorID]++; // Incremento de contador del tipo correspondiente
            numProdsConsumidosPorTipo[producto.tipo - 'a']++;

            sem_post(&hayEspacio);
        }
    }



    // SECCIÓN CRÍTICA DE LA LISTA
    sem_wait(&semLista);
    nuevoConsumidor->productosConsumidos = numProdsConsumidos;
    nuevoConsumidor->productosConsumidosPorTipo = numProdsConsumidosPorTipo;
    nuevoConsumidor->totalPorProveedor = numProdsConsumidosPorProveedor;
    nuevoConsumidor->consumidorID = args->consumidorID;
    nuevoConsumidor->siguiente = NULL;
    if (nodoActual == NULL) { // Debe de ser el primer nodo
        nodoInicial = nodoActual;
        nodoActual = nuevoConsumidor;
    } else {
        aux = nodoActual;
        while (aux->siguiente != NULL) {
            aux = aux->siguiente;
        }
        aux->siguiente = nuevoConsumidor;
    }
    sem_post(&semLista);


    sem_post(&adelanteFacturador);
    pthread_exit(NULL);
}

void* facturadorFunc(void *argsFact) {
    int tipos[10], *proveedores, *consumidores, totalProductos = 0, maximo = 0, IDConsMasGordo = 0;
    ArgsProvFact *args = (ArgsProvFact*) argsFact;


    if ((proveedores = calloc(args->nProveedores, sizeof(int))) == NULL) {
        fprintf(stderr, "Error al asignar memoria para el array de proveedores.\n");
        exit(-1);}
    if ((consumidores = calloc(args->nConsumidores, sizeof(int))) == NULL) {
        fprintf(stderr, "Error al asignar memoria para el array de consumidores.\n");
        exit(-1);}


    // Procesar
    for (int i = 0; i < args->nConsumidores; i++) {
        sem_wait(&adelanteFacturador);


        // OPERACIONES
        for (int j = 0; j <args->nProveedores; j++) proveedores[j] += nodoActual->totalPorProveedor[j];
        for (int j = 0; j < 10; j++) tipos[j] += nodoActual->productosConsumidosPorTipo[j];
        consumidores[i] = nodoActual->productosConsumidos; // Para sacar el que más ha consumido más tarde


        // ESCRITURA CONSUMIDORES
        fprintf(args->file, "\nCliente consumidor: %d\n", nodoActual->consumidorID);
        fprintf(args->file, "  Productos consumidos: %d. De los cuales:\n", nodoActual->productosConsumidos);
        for (int tipo = 0; tipo < NPRODUCTOS; tipo++) {
            fprintf(args->file, "     Producto tipo \"%c\": %d\n", (char) (tipo + 'a'), nodoActual->productosConsumidosPorTipo[tipo]);
        }


        // NUEVO NODO
        nodoActual = nodoActual->siguiente;
    }

    for (int j = 0; j < args->nProveedores; j++) totalProductos += proveedores[j];
    for (int j = 0; j < args->nConsumidores; j++) { // Hallar el consumidor que más a consumido.
        if (maximo <= consumidores[j]) {
            maximo = consumidores[j];
            IDConsMasGordo = j;
        }
    }



    // ESCRITURA CONCLUSION FINAL
    fprintf(args->file, "\nTotal de productos consumidos: %d\n", totalProductos);
    for (int j = 0; j < args->nProveedores; j++) {
        fprintf(args->file, "     %d del proveedor %d.\n", proveedores[j], j);
    }
    fprintf(args->file, "Cliente consumidor que mas ha consumido: %d\n", IDConsMasGordo);



    // LIBERAR LA LISTA
    while (nodoInicial != NULL) {
        nodoActual = nodoInicial->siguiente;
        free(nodoInicial);
        nodoInicial = nodoActual;
    }

    free(proveedores);
    free(consumidores);

    pthread_exit(NULL);
}

int esCadena(char *cadena) {
    for (int i = 0; i < strlen(cadena); i++) {
        if (!isdigit(cadena[i])) {
            return 1; // Devuelve 1 si no es una cadena de dígitos
        }
    }
    return 0; // Devuelve 0 si es una cadena de dígitos
}