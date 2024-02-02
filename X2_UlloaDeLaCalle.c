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


typedef struct argsProvFact {
    int proveedorID;
    int tamBuffer;
    int nProveedores;
    int nConsumidores;
    char *pathProv;
    FILE *file; // EL FICHERO ABIERTO.
} ArgsProvFact;
typedef struct argsCons {
    int consumidorID;
    int tamBuffer;
    int nProveedores;
    int nConsumidores;
} ArgsCons;

// Variables GLOBALES :)
sem_t semaforoFichero, semContC, semContP, hayEspacio, hayDato, adelanteFacturador, semProveedorAcabado, semLista;
Producto *buffer;
int itProdBuffer = 0, itConsBuffer = 0, contProvsAcabados = 0;
ConsumidorInfo *nodoPrincipal = NULL, *nodoActual = NULL;

// Declaración de funciones
void* proveedorFunc(void *arg);

void* consumidorFunc(void *argsCont);

ConsumidorInfo *agregarConsumidor(ConsumidorInfo *nodo, int productosConsumidos, int **productosConsumidosPorTipo, int ID);

void liberarLista(ConsumidorInfo *nodoP);

void* facturadorFunc(void *arg);

int esTipoValido(char c);

int esCadena(char *cadena);


int main(int argc, char *argv[]) {
    int nConsumidores, nProveedores, tamBuffer;
    int idHilosP[MAX_PROVEEDORES];
    int idHilosC[MAX_CONSUMIDORES];
    char pathProv[255], pathDest[255];
    FILE *fichProveedor, *fichDestino;
    pthread_t proveedorThread, *consumidorThread, facturadorThread;
    ArgsProvFact *argsProv;
    ArgsCons *argsCons;

    // Verificación de la cantidad de argumentos
    if (argc != 5) {
        fprintf(stderr, "Error: Número incorrecto de argumentos.\n");
        exit(-1);
    }


    // Verificar si los parámetros pasados son válidos o no. Si no, se pasa -1 para salir en el próximo if
    tamBuffer = (!esCadena(argv[3])) ? atoi(argv[3]) : -1;
    nConsumidores = (!esCadena(argv[4])) ? atoi(argv[4]) : -1;


    argsProv->tamBuffer = argsCons->tamBuffer = tamBuffer;
    contProvsAcabados = argsProv->nProveedores = argsCons->nProveedores = nProveedores;
    argsProv->nConsumidores = argsCons->nConsumidores = nConsumidores;



    if (tamBuffer <= 0 || tamBuffer > 5000) {
        fprintf(stderr, "Error: T debe ser un entero positivo menor o igual a 5000.\n");
        exit(-1);
    }
    if (nConsumidores <= 0 || nConsumidores > MAX_CONSUMIDORES) {
        fprintf(stderr, "Error: C debe ser un entero positivo menor o igual a %d.\n", MAX_CONSUMIDORES);
        exit(-1);
    }
    if ((consumidorThread = calloc(nConsumidores, sizeof(char))) == NULL) {
        fprintf(stderr, "Error al asignar memoria para los hilos consumidores.\n");
        exit(-1);
    }
    if ((buffer = calloc(tamBuffer, sizeof(Producto))) == NULL) {
        fprintf(stderr, "Error al asignar memoria para el buffer compartido.\n");
        exit(-1);
    }

    sem_init(&hayEspacio, 0, tamBuffer);
    sem_init(&semaforoFichero, 0, 1);
    sem_init(&semContP, 0, 1); // Semáforo para el contador de proveedores
    sem_init(&semContC, 0, 1); // Semáforo para el contador de consumidores
    sem_init(&hayDato, 0, 0);
    sem_init(&adelanteFacturador, 0, 0);
    sem_init(&semProveedorAcabado, 0, 1);
    sem_init(&semLista, 0, 1);



    // APERTURA DE FICHEROS
    sprintf(argsProv->pathProv, "%s/proveedor%d.dat", argv[1], 0);
    if ((fichProveedor = fopen(argsProv->pathProv, "r")) == NULL) {
        fprintf(stderr, "Error al abrir el archivo de entrada del proveedor %d.\n", 0);
        exit(-1);
    }
    fclose(fichProveedor);

    sprintf(pathDest, "%s/%s", argv[1], argv[2]);
    if ((argsProv->file = fopen(pathDest, "w")) == NULL) {
        fprintf(stderr, "Error al abrir el archivo salida.");
        exit(-1);
    }



    // LANZAMIENTO DE HILOS
    for (int i = 0; i < nProveedores; ++i) {
        argsProv->proveedorID = i;
        pthread_create(&proveedorThread, NULL, (void *) proveedorFunc, (void *)argsProv);
    }
    for (int i = 0; i < nConsumidores; i++) {
        argsCons->consumidorID = i;
        pthread_create(&consumidorThread[i], NULL, (void *) consumidorFunc, (void *)argsCons);
    }
    pthread_create(&facturadorThread, NULL, (void *) facturadorFunc, (void *)argsProv);


    // Esperar a que los hilos terminen
    pthread_join(proveedorThread, NULL); // Proveedor
    for (int i = 0; i < nConsumidores; i++) { // Consumidor
        pthread_join(consumidorThread[i], NULL);
    }
    pthread_join(facturadorThread, NULL); // Facturador


    // Destruir semáforos y liberar memoria
    sem_destroy(&semaforoFichero);
    sem_destroy(&semContP);
    sem_destroy(&semContC);
    sem_destroy(&hayEspacio);
    sem_destroy(&hayDato);
    sem_destroy(&adelanteFacturador);
    sem_destroy(&semProveedorAcabado);
    sem_destroy(&semLista);

    free(buffer);
}

void* proveedorFunc(void *arg) {
    int productosLeidos = 0, productosValidos = 0, proveedorID = 0, indiceProveedor;
    int *totalProductos;
    char c;
    FILE *fichProveedor;
    ArgsProvFact *args = (ArgsProvFact *) arg;

    totalProductos = calloc(NPRODUCTOS, sizeof(Producto));
    if (totalProductos == NULL) { // No haría falta la mem.Dinámica pues NPRODUCTOS se conoce en compilación
        fprintf(stderr, "Error al asignar memoria para el array totalProductos.\n");
        exit(-1);
    }


    // APERTURA FICHERO
    if ((fichProveedor = fopen(args->pathProv, "r")) == NULL) {
        fprintf(stderr, "Error al abrir el archivo de entrada del proveedor %d.\n", 0);
        exit(-1);
    }
    // Leer y procesar productos del archivo

    while ((c = fgetc(fichProveedor)) != EOF) { // Saldrá cuando se acabe el fichero
        if (esTipoValido(c)){

            sem_wait(&hayEspacio);

            // Section crítica del índice de Proveedores
            sem_wait(&semContP);
            indiceProveedor = itProdBuffer;
            itProdBuffer = (itProdBuffer + 1) % args->tamBuffer;
            sem_post(&semContP);


            // Escribir en el búfer
            buffer[indiceProveedor].tipo = c;
            buffer[indiceProveedor].proveedorID = proveedorID;
            sem_post(&hayDato);

            // Incrementar contador de productos válidos
            productosValidos++;
            totalProductos[c - 'a']++;
            productosLeidos++;

        } else { // NO ES TIPO VALIDO
            productosLeidos++;
        }
    }

    sem_wait(&semProveedorAcabado);
    // TODO revisar bn
    buffer[indiceProveedor].tipo = (contProvsAcabados-- == 0) ? 'F' : 0;

    sem_post(&semProveedorAcabado);

    // FIN DE FICHERO



    sem_post(&hayDato);



    // Sección crítica fichero
    sem_wait(&semaforoFichero);



    fprintf(args->file, "Proveedor: %d.\n", proveedorID);
    fprintf(args->file, "   Productos procesados: %d.\n", productosLeidos);
    fprintf(args->file, "   Productos Inválidos: %d.\n", productosLeidos - productosValidos);
    fprintf(args->file, "   Productos Válidos: %d. De los cuales se han insertado:\n", productosValidos);

    for (char tipo = 'a'; tipo <= 'j'; tipo++) {
        fprintf(args->file, "     %d de tipo \"%c\".\n", totalProductos[tipo - 'a'], tipo);
    }
    fclose(args->file);
    sem_post(&semaforoFichero);
    sem_post(&semProveedorAcabado); ///////////////////////////////////////////////

    // Cerrar archivos de salida y liberar memoria
    free(totalProductos);
    pthread_exit(NULL);
}

void *consumidorFunc(void *argsCont) {
    int bandera = 1;
    int numProdsConsumidos = 0, consumidorID = *((int*) argsCont);
    Producto producto;
    ArgsCons *args = (ArgsCons *)argsCont;


    int** numProdsConsumidosPorProveedor = (int**) calloc(args->nProveedores, sizeof(int*));
    if (numProdsConsumidosPorProveedor == NULL) {
        fprintf(stderr, "Error al reservar memoria para las filas.\n");
        exit(-1);
    }
    for (int i = 0; i < args->nProveedores; i++) { // NPRODUCTOS es == a 10
        numProdsConsumidosPorProveedor[i] = (int*) calloc(NPRODUCTOS + 1, sizeof(int));
        if (numProdsConsumidosPorProveedor[i] == NULL) {
            fprintf(stderr, "Error al reservar memoria para las columnas.\n");
            exit(-1);
        }
    }


    // Consumir productos del búfer
    while (bandera) { //contProvsAcabados != nProveedores
        sem_wait(&hayDato);

        // Sección crítica
        sem_wait(&semContC);
        producto = buffer[itConsBuffer];
        if (producto.tipo  == 'F') { // Si es F les digo al resto de consumidores que hay dato y no modifico el contador para que también lean la F
            sem_post(&semContC);
            sem_post(&hayDato);
            bandera = 0;
        }
        else {
            itConsBuffer = (itConsBuffer + 1) % args->tamBuffer; // Incrementa contador buffer
            sem_post(&semContC);

            numProdsConsumidos++; // Incremento de contador general
            numProdsConsumidosPorProveedor[producto.proveedorID][producto.tipo - 'a']++; // Incremento de contador del tipo correspondiente

            sem_post(&hayEspacio);
        }
    }
    sem_wait(&semLista);
    nodoActual = agregarConsumidor(nodoActual, numProdsConsumidos, numProdsConsumidosPorProveedor, consumidorID); //hay que pasarle prodConsPorTipo
    sem_post(&semLista);

    sem_post(&adelanteFacturador);
    pthread_exit(NULL);
}

void* facturadorFunc(void *argsFact) {
    int tipos[10], *proveedores, *consumidores, totalProductos = 0, maximo = 0, IDConsMasGordo = 0;
    ArgsProvFact *args = (ArgsProvFact*) argsFact;


    if ((proveedores = calloc(args->nProveedores, sizeof(int))) == NULL) {
        fprintf(stderr, "Error al asignar memoria para el array de proveedores.\n");
        exit(-1);
    }

    if ((consumidores = calloc(args->nConsumidores, sizeof(int))) == NULL) {
        fprintf(stderr, "Error al asignar memoria para el array de consumidores.\n");
        exit(-1);
    }

    for (int k = 0; k < args->nConsumidores; k++) { // Inicializar array consumidores // TODO quitar, irrelevante
        consumidores[k] = 0;
    }


    sem_wait(&semProveedorAcabado);
    // Procesar
    for (int i = 0; i < args->nConsumidores; i++) {
        sem_wait(&adelanteFacturador);

        for (int k = 0; k < 10; k++) { // Inicializar array de tipos de producto
            tipos[k] = 0;
        }

        for (int j = 0; j < args->nProveedores; j++) {
            proveedores[j] += nodoActual->totalPorProveedor[j]; // TODO reparar el cambio de matriz -> doble array
        }
        for (int j = 0; j < 10; j++) { // Contar cuantos productos se han consumido por tipo entre todos los proveedores
            for (int k = 0; k < args->nProveedores; k++) {
                tipos[j] += nodoActual->productosConsumidosPorTipo[k];
            }
        }
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


    for (int j = 0; j < args->nProveedores; j++) { // Total de productos
        totalProductos += proveedores[j];
    }
    for (int j = 0; j < args->nConsumidores; j++) { // Hallar el consumidor que más a consumido.
        if (maximo <= consumidores[j]) {
            maximo = consumidores[j];
            IDConsMasGordo = j;
        }
    }



    // ESCRITURA CONCLUSION FINAL
    fprintf(args->file, "\nTotal de productos consumidos: %d\n", totalProductos);
    for (int j = 0; j < args->nProveedores; ++j) {
        fprintf(args->file, "     %d del proveedor %d.\n", proveedores[j], j);
    }
    fprintf(args->file, "Cliente consumidor que mas ha consumido: %d\n", IDConsMasGordo);

    //fclose(args->file);
    liberarLista(nodoPrincipal);
    free(proveedores);
    free(consumidores);

    pthread_exit(NULL);
}

// Devuelve True si está entre a y j (incluidas)
int esTipoValido(char c) { return (c >= 'a' && c <= 'j'); }

int esCadena(char *cadena) {
    for (int i = 0; i < strlen(cadena); ++i) {
        if (!isdigit(cadena[i])) {
            return 1; // Devuelve 1 si no es una cadena de dígitos
        }
    }
    return 0; // Devuelve 0 si es una cadena de dígitos
}


ConsumidorInfo *agregarConsumidor(ConsumidorInfo *nodo, int productosConsumidos, int **productosConsumidosPorTipo, int ID) {
    ConsumidorInfo *nuevoConsumidor, *aux;

    if ((nuevoConsumidor = (ConsumidorInfo *) calloc(1,sizeof(ConsumidorInfo))) == NULL) {
        fprintf(stderr, "Error al asignar memoria para el nuevo nodo de la lista enlazada.\n");
        exit(-1);
    }
    nuevoConsumidor->productosConsumidos = productosConsumidos;
    nuevoConsumidor->productosConsumidosPorTipo = productosConsumidosPorTipo;
    nuevoConsumidor->consumidorID = ID;
    nuevoConsumidor->siguiente = NULL;
    if (nodo == NULL) { // Debe de ser el primer nodo
        nodoPrincipal = nodo;
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
void liberarLista(ConsumidorInfo *nodoP) { // Función para liberar la lista enlazada ahora que ya no se necesita
    ConsumidorInfo *actual = nodoP;
    ConsumidorInfo *siguiente;

    while (actual != NULL) {
        siguiente = actual->siguiente;

        // Libera la memoria del nodo
        free(actual);
        actual = siguiente;
    }
}