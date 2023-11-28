//
// Created by Ivan y David on 11/22/2023.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
//#include <pthread.h>
#include <stdbool.h>

#define MAX_COMMAND_LENGTH 800
#define MAX_ARGUMENTS 5

// Estructura para pasar datos al hilo del proveedor
typedef struct {
    char *ruta;
    int T;
    int P;
    int C;
} ProveedorData;

typedef struct {
    char tipo;
    int proveedorID;
} Producto;

// Estructura para mantener el registro del total de productos de cada tipo
typedef struct {
    int total['j' - 'a' + 1];
} TotalProductos;

void proveedorFunc(void *data);
bool esTipoValido(char c);
bool esCadena(char *string);

char path[MAX_COMMAND_LENGTH];

int main (int argc, char *argv[]) {
    char *path;

    printf("piribi \n");
    printf("piribi \n");

    // Verificación de la cantidad de argumentos
    if (argc != 6) {
        fprintf(stderr, "Error: Número incorrecto de argumentos.\n");
        return -1;
    }

    //Verificación parámetros
    if (esCadena(argv[3]) || atoi(argv[3]) <= 0 || atoi(argv[3]) > 5000){ //Cambiar atoi por strtol?
         fprintf(stderr, "Error: T debe ser un entero positivo menor o igual a 5000.\n");
        return -1;
    }
    if (esCadena(argv[4]) || atoi(argv[4]) <= 0 || atoi(argv[4]) > 7){
        fprintf(stderr, "Error: P debe ser un entero positivo menor o igual a 7.\n");
        return -1;
    }
    if (esCadena(argv[5]) || atoi(argv[5]) <= 0 || atoi(argv[5]) > 1000){
        fprintf(stderr, "Error: C debe ser un entero positivo menor o igual a 1000.\n");
        return -1;
    }
    sprintf(path, "proveedor%d.dat", atoi(argv[2]));
    printf("Waka");

    ProveedorData proveedor_data;
    proveedor_data.ruta = argv[1]; // Ruta de los archivos de entrada
    proveedor_data.T = atoi(argv[3]); // Tamaño del búfer circular
    proveedor_data.P = atoi(argv[4]); // Número total de proveedores
    proveedor_data.C = atoi(argv[5]); // Número total de clientes

    proveedorFunc(&proveedor_data);

    printf("Wak1a");

    return 0;
}

void proveedorFunc(void *data) {
    ProveedorData *proveedor_data = (ProveedorData *)data;
    FILE *file, *outputFile;
    Producto *buffer;
    char outputFilename[MAX_COMMAND_LENGTH], filename[MAX_COMMAND_LENGTH];
    int in = 0, out = 0; // Índice de escritura y lectura en el búfer
    char c;
    int productosLeidos = 0, productosValidos = 0, productosInvalidos = 0;
    TotalProductos totalProductos = {{0}};

    // Abrir el archivo de entrada del proveedor
    sprintf(filename, "%sproveedor%d.dat", proveedor_data->ruta, proveedor_data->P);
    file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, "Error al abrir el archivo de entrada del proveedor %d.\n", proveedor_data->P);
    }

    // Inicializar el búfer circular
    buffer = (Producto *)malloc(proveedor_data->T * sizeof(Producto));

    if (buffer == NULL) {
        fprintf(stderr, "Error al asignar memoria para el búfer del proveedor %d.\n", proveedor_data->P);
        fclose(file); // Cierra el archivo antes de salir
        return;  // Agrega un return para salir de la función en caso de error
    }


    // Leer y procesar productos del archivo
    while ((c = fgetc(file)) != EOF) {
        productosLeidos++;

        if (esTipoValido(c)) {
            // Procesar productos válidos
            Producto nuevoProducto = {c, proveedor_data->P};
            buffer[in] = nuevoProducto;
            in = (in + 1) % proveedor_data->T;
            productosValidos++;
            totalProductos.total[c - 'a']++;
        } else {
            // Procesar productos inválidos
            productosInvalidos++;
        }
    }
    fclose(file); // Cerrar el archivo

    // Escribir resultados en el archivo de salida
    sprintf(outputFilename, "output_proveedor%d.txt", proveedor_data->P);

    outputFile = fopen(outputFilename, "w");
    if (outputFile == NULL) {
        fprintf(stderr, "Error al abrir el archivo de salida del proveedor %d.\n", proveedor_data->P);
        free(buffer);
      return;
    }

    fprintf(outputFile, "Proveedor: %d\n", proveedor_data->P);
    fprintf(outputFile, "Productos Inválidos: %d\n", productosInvalidos);
    fprintf(outputFile, "Productos Válidos: %d. De los cuales se han insertado:\n", productosValidos);

    for (char tipo = 'a'; tipo <= 'j'; tipo++) {
        fprintf(outputFile, "%d de tipo \"%c\".\n", totalProductos.total[tipo - 'a'], tipo);
    }

    // Cerrar el archivo de salida
    fclose(outputFile);

    printf("Waka");

    // Liberar memoria del búfer
    free(buffer);
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