//
// Created by forex on 03/12/2023.
//
#import <stdio.h>
#import <stdlib.h>

typedef struct nodo {
    char tipo;
    int proveedorID;
    struct nodo *siguiente;
} Producto;


Producto *initListaProducto(Producto *lista){
    lista = NULL;
    return lista;
}

Producto *agregarProducto(Producto *producto, char tipo, int proveedorID) {
    Producto *nuevoProducto;
    Producto *aux;
    nuevoProducto = (Producto*) malloc(sizeof(Producto));
    nuevoProducto->tipo = tipo;
    nuevoProducto->proveedorID = proveedorID;
    nuevoProducto->siguiente = NULL;
    if (producto == NULL){
        producto = nuevoProducto;
    } else {
        aux = producto;
        while (aux->siguiente != NULL) {
            aux = aux->siguiente;
        }
        aux->siguiente = nuevoProducto;
    }
    return producto;
}

int main(){
    Producto *listaProductos = initListaProducto(listaProductos);
    listaProductos = agregarProducto(listaProductos, 'a', 1);
    listaProductos = agregarProducto(listaProductos, 'b', 1);
    listaProductos = agregarProducto(listaProductos, 'b', 2);
    while (listaProductos != NULL) {
        printf("%c%d\n", listaProductos->tipo, listaProductos->proveedorID);
        listaProductos = listaProductos->siguiente;
    }
}