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
    int **productosConsumidosPorTipo;
    struct nodo *siguiente;
} ConsumidorInfo;

// Variables GLOBALES :)
sem_t semaforoFichero, semContC, semContP, hayEspacio, hayDato, adelanteFacturador, proveedoresAcabados, semContProbsAcabados;
Producto *buffer;
char *path, *fichDest;
int itProdBuffer = 0, itConsBuffer = 0, contProvsAcabados = 0, tamBuffer, nProveedores = 1, nConsumidores;
ConsumidorInfo *nodoPrincipal = NULL, *nodoActual = NULL;

// Declaración de funciones
void* proveedorFunc(void *arg);

void* consumidorFunc(void *arg);

ConsumidorInfo *agregarConsumidor(ConsumidorInfo *nodo, int productosConsumidos, int **productosConsumidosPorTipo, int ID);

void liberarLista(ConsumidorInfo *nodoP);

void* facturadorFunc();

int esTipoValido(char c);

int esCadena(char *string);


int main(int argc, char *argv[]) {
    int idHilosP = 1, idHilosC[MAX_CONSUMIDORES];
    char dirpath[255];
    FILE *file;
    pthread_t *proveedorThread, *consumidorThread, facturadorThread;

    // Verificación de la cantidad de argumentos
    if (argc != 5) {
        fprintf(stderr, "Error: Número incorrecto de argumentos.\n");
        return -1;
    }

    // Verificar si los parámetros pasados son válidos o no. Si no, se pasa -1 para salir en el próximo if
    tamBuffer = (!esCadena(argv[2])) ? atoi(argv[3]) : -1;
    nProveedores = (!esCadena(argv[3])) ? atoi(argv[4]) : -1;
    nConsumidores = (!esCadena(argv[4])) ? atoi(argv[5]) : -1;



    if (tamBuffer <= 0 || tamBuffer > 5000) {
        fprintf(stderr, "Error: T debe ser un entero positivo menor o igual a 5000.\n");
        exit(-1);
    }
    if (nConsumidores <= 0 || nConsumidores > MAX_CONSUMIDORES) {
        fprintf(stderr, "Error: C debe ser un entero positivo menor o igual a %d.\n", MAX_CONSUMIDORES);
        exit(-1);
    }

    proveedorThread = calloc(nProveedores, sizeof(char));
    if (proveedorThread == NULL) {
        fprintf(stderr, "Error al asignar memoria para los hilos proveedores.\n");
        exit(-1);
    }
    consumidorThread = calloc(nConsumidores, sizeof(char));
    if (consumidorThread == NULL) {
        fprintf(stderr, "Error al asignar memoria para los hilos consumidores.\n");
        exit(-1);
    }
    buffer = calloc(tamBuffer, sizeof(Producto));
    if (buffer == NULL) {
        fprintf(stderr, "Error al asignar memoria para el buffer compartido.\n");
        exit(-1);
    }

    // Prueba apertura de TODOS los ficheros para proveedores
    for (int i = 0; i < nProveedores; i++) {
        // Formatear cadena
        sprintf(dirpath, "%s/proveedor%d.dat", argv[1], i);
        file = fopen(dirpath, "r");
        if (file == NULL) {
            fprintf(stderr, "Error al abrir el archivo de entrada del proveedor %d.\n", i);
            exit(-1);
        }
        fclose(file);
    }
  // UTILIZAR SOLO ARGV
    // Verificado correctamente
    path = argv[1];
    fichDest = argv[2];

    // ABRIRLO AQUI SOLO Y NO ABRIRLO EN CADA PTHREAD, CAMBIA PARA SOLO HACERLO EN EL MAIN
    // Creado o limpieza del fichero de salida.
    file = fopen(fichDest, "w");
    if (file == NULL) {
        fprintf(stderr, "Error al abrir el archivo salida.");
        exit(-1);
    }
    fclose(file);

    sem_init(&hayEspacio, 0, tamBuffer);
    sem_init(&semaforoFichero, 0, 1);
    sem_init(&semContP, 0, 1); // Semáforo para el contador de proveedores
    sem_init(&semContC, 0, 1); // Semáforo para el contador de consumidores
    sem_init(&hayDato, 0, 0);
    sem_init(&adelanteFacturador, 0, 0);
    sem_init(&proveedoresAcabados, 0, -nProveedores);
    sem_init(&semContProbsAcabados, 0, 1);


    // Lanzar hilos proveedores
    pthread_create(&proveedorThread, NULL, (void *) proveedorFunc, (void *)argv);
        //       printf("Hilo Proveedor %d lanzado.\n", i);

    // Lanzar hilos consumidores
    for (int j = 0; j < nConsumidores; j++) {
        idHilosC[j] = j;
        pthread_create(&consumidorThread[j], NULL, (void *) consumidorFunc, &idHilosC[j]);
        //printf("Hilo Consumidor %d lanzado.\n", j);
    }

    // Crear hilo del facturadorFunc
    pthread_create(&facturadorThread, NULL, (void *) facturadorFunc, NULL); // No se le pasa nada a facturador

    // Esperar a que los hilos terminen
    pthread_join(*proveedorThread, NULL);
    for (int i = 0; i < nConsumidores; i++) {
        pthread_join(consumidorThread[i], NULL);
    }

    pthread_join(facturadorThread, NULL);

    // Destruir semáforos y liberar memoria
    sem_destroy(&semaforoFichero);
    sem_destroy(&semContP);
    sem_destroy(&semContC);
    sem_destroy(&hayEspacio);
    sem_destroy(&hayDato);
    sem_destroy(&adelanteFacturador);
    sem_destroy(&proveedoresAcabados);
    sem_destroy(&semContProbsAcabados);

    free(buffer);
}

void* proveedorFunc(void *arg) {
    FILE *file, *outputFile;
    int productosLeidos = 0, productosValidos = 0, proveedorID = *((int*) arg), indiceProveedor;
    int *totalProductos;
    char c, fichPath[255];

    totalProductos = calloc(NPRODUCTOS, sizeof(Producto));
    if (totalProductos == NULL) { // No haría falta la mem.Dinámica pues NPRODUCTOS se conoce en compilación
        fprintf(stderr, "Error al asignar memoria para el array totalProductos.\n");
        exit(-1);
    }

    // Todo (Quitar) Inicializar totalProductos
    for (int i = 0; i < NPRODUCTOS; ++i) {
        totalProductos[i] = 0;
    }

    // Formatear cadena
    sprintf(fichPath, "%s/proveedor%d.dat", path, proveedorID);
    // Abrir el archivo de entrada del proveedor
    file = fopen(fichPath, "r");
    if (file == NULL) {
        fprintf(stderr, "Error al abrir el archivo salida.");
        exit(-1);
    }

    // Leer y procesar productos del archivo
    while ((c = fgetc(file)) != EOF) { // Saldrá cuando se acabe el fichero
        if (esTipoValido(c)){

            sem_wait(&hayEspacio);

            // Section crítica del índice de Proveedores
            sem_wait(&semContP);
            indiceProveedor = itProdBuffer;
            itProdBuffer = (itProdBuffer + 1) % tamBuffer;
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

    fclose(file);

    sem_wait(&semContP);
    indiceProveedor = itProdBuffer;
    itProdBuffer = (itProdBuffer + 1) % tamBuffer;
    sem_post(&semContP);

    // FIN DE FICHERO
    buffer[indiceProveedor].tipo = 'F';
    buffer[indiceProveedor].proveedorID = proveedorID;

    sem_post(&hayDato);


    // Escribir resultados en el fichero de salida
    // Formatear cadena
    sprintf(fichPath, "%s/%s", path, fichDest);

    // Sección crítica fichero
    sem_wait(&semaforoFichero);
    outputFile = fopen(fichDest, "a");
    if (outputFile == NULL) {
        fprintf(stderr, "Error al abrir el archivo de salida del proveedor %d.\n", 0);
        exit(-1);
    }

    fprintf(outputFile, "Proveedor: %d.\n", proveedorID);
    fprintf(outputFile, "   Productos procesados: %d.\n", productosLeidos);
    fprintf(outputFile, "   Productos Inválidos: %d.\n", productosLeidos - productosValidos);
    fprintf(outputFile, "   Productos Válidos: %d. De los cuales se han insertado:\n", productosValidos);

    for (char tipo = 'a'; tipo <= 'j'; tipo++) {
        fprintf(outputFile, "     %d de tipo \"%c\".\n", totalProductos[tipo - 'a'], tipo);
    }
    sem_post(&semaforoFichero);
    sem_post(&proveedoresAcabados); ///////////////////////////////////////////////

    // Cerrar archivos de salida y liberar memoria
    free(totalProductos);
    fclose(outputFile);
    pthread_exit(NULL);
}

void *consumidorFunc(void *arg) {
    int bandera = 1;
    int numProdsConsumidos = 0, consumidorID = *((int*) arg);
    Producto producto;


    int** numProdsConsumidosPorProveedor = (int**) calloc(nProveedores, sizeof(int*));
    if (numProdsConsumidosPorProveedor == NULL) {
        fprintf(stderr, "Error al reservar memoria para las filas.\n");
        exit(-1);
    }
    for (int i = 0; i < nProveedores; i++) { // NPRODUCTOS es == a 10
        numProdsConsumidosPorProveedor[i] = (int*) calloc(NPRODUCTOS + 1, sizeof(int));
        if (numProdsConsumidosPorProveedor[i] == NULL) {
            fprintf(stderr, "Error al reservar memoria para las columnas.\n");
            exit(-1);
        }
    }


    // Consumir productos del búfer
    while (bandera) { //contProvsAcabados != nProveedores
        sem_wait(&hayDato);
        printf(" %d ", bandera); // TODO

        // Sección crítica
        sem_wait(&semContC);
        producto = buffer[itConsBuffer];
        if (producto.tipo  == 'F') { // Si es F les digo al resto de consumidores que hay dato y no modifico el contador para que también lean la F
            sem_post(&semContC);
            sem_wait(&hayDato);
            bandera = 0;
        }
        else {
            itConsBuffer = (itConsBuffer + 1) % tamBuffer; // Incrementa contador buffer
            sem_post(&semContC);

            numProdsConsumidos++; // Incremento de contador general
            numProdsConsumidosPorProveedor[producto.proveedorID][producto.tipo - 'a']++; // Incremento de contador del tipo correspondiente
          // SI SE NECESITA UN SEMAFORO PARA EL NUMERO DE ACABADO
            sem_wait(&semContProbsAcabados); //////////////////////////
            contProvsAcabados++;
            sem_post(&semContProbsAcabados);  //////////////////////////
            sem_post(&hayEspacio);
        }
    }

    nodoActual = agregarConsumidor(nodoActual, numProdsConsumidos, numProdsConsumidosPorProveedor, consumidorID); //hay que pasarle prodConsPorTipo

    sem_post(&adelanteFacturador);
    pthread_exit(NULL);
}

void* facturadorFunc() {
    FILE *outputFile = NULL;
    int tipos[10], *proveedores, *consumidores, suma = 0, maximo = 0, cons = 0;
    char *fichPath;

    proveedores = calloc(nProveedores, sizeof(int));
    if (proveedores == NULL) {
        fprintf(stderr, "Error al asignar memoria para el array de proveedores.\n");
        exit(-1);
    }
    consumidores = calloc(nConsumidores, sizeof(int));
    if (consumidores == NULL) {
        fprintf(stderr, "Error al asignar memoria para el array de consumidores.\n");
        exit(-1);
    }
    fichPath = calloc(255, sizeof(char));
    if (fichPath == NULL) {
        fprintf(stderr, "Error al asignar memoria para fichPath.\n");
        exit(-1);
    }

    // Formatear cadena
    sprintf(fichPath, "%s/%s", path, fichDest);

    outputFile = fopen(fichPath, "a");
    if (outputFile == NULL) {
        fprintf(stderr, "Error al abrir el archivo salida.");
        exit(-1);
    }

    for (int k = 0; k < nConsumidores; k++) { // Inicializar array consumidores
        consumidores[k] = 0;
    }


    sem_wait(&proveedoresAcabados);
    // Procesar
    for (int i = 0; i < nConsumidores; i++) {
        sem_wait(&adelanteFacturador);

        for (int k = 0; k < 10; ++k) { // Inicializar array de tipos de producto
            tipos[k] = 0;
        }

        for (int j = 0; j < nProveedores; j++) {
            for (int k = 0; k < 10; k++) {
                proveedores[j] += nodoActual->productosConsumidosPorTipo[j][k];
            }
        }
        for (int j = 0; j < 10; j++) { // Contar cuantos productos se han consumido por tipo entre todos los proveedores
            for (int k = 0; k < nProveedores; k++) {
                tipos[j] += nodoActual->productosConsumidosPorTipo[k][j];
            }
        }
        consumidores[i] = nodoActual->productosConsumidos; // Para sacar el que más ha consumido más tarde

        fprintf(outputFile, "\nCliente consumidor: %d\n", nodoActual->consumidorID);
        fprintf(outputFile, "  Productos consumidos: %d. De los cuales:\n", nodoActual->productosConsumidos);

        for (int tipo = 0; tipo < (NPRODUCTOS); ++tipo) {
            fprintf(outputFile, "     Producto tipo \"%c\": %d\n", (char) (tipo + 'a'), tipos[tipo]);
        }
        nodoActual = nodoActual->siguiente;
    }


    for (int j = 0; j < nProveedores; j++) { // Total de productos
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

    liberarLista(nodoPrincipal);
    free(proveedores);
    free(consumidores);
    free(fichPath);

    fclose(outputFile);
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
    ConsumidorInfo *nuevoConsumidor;
    ConsumidorInfo *aux;
    nuevoConsumidor = (ConsumidorInfo *) calloc(1,sizeof(ConsumidorInfo));
    if (nuevoConsumidor == NULL) {
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