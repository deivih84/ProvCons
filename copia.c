//
// Created by David on 10/25/2023.
//

# include <stdio.h>
# include <string.h>

int main (int argc, char *argv[]) {
    //Vars
    char car;

    FILE *origen, *destino;

    //Errores
    if (argc != 2) {
        fprintf(stderr, "USO: copia archivo_origen archivo_destino");
        return 0;
    } else if (strcmp(argv[0],argv[1]) != 0) {
        fprintf(stderr, "Error: archivo origen y destino no pueden ser el mismo");
        return 0;
    }
    if (((destino = fopen(argv[2], "r")) != NULL)) {
        fclose(destino);
        fprintf(stderr, "Error: archivo destino no debe existir.");
        return 2;
    }
    if (((origen = fopen(argv[1], "r")) == NULL)) {
            fprintf(stderr, "Error: archivo origen no debe existir.");
            return 0;
    }
    //Escritura
    if ((destino = fopen(argv[1], "w")) == NULL){
        fprintf(stderr, "Error: archivo origen no debe existir.");
        return 2;
    }
       //si error cierro origen y return 1 CONTROLAR ERRORRES

    while (feof(origen)) { //Mientras haya linea -> fgets
        car = (char)fgetc(origen);
        fputc(car, destino);
    }
    fclose(origen);
    fclose(destino);

    return 0;
}