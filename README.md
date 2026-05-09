# Programación III: Proyecto Final (2026-1)

## Integrantes

* Enzo Matias Calderon
* Ferrel Valentino Infante Garcia
* Sebastian Gonzalo Casas Matos

## Descripción

## 1. Pre-procesamiento de datos
El sistema realiza un proceso de limpieza y normalización de los datos antes de indexarlos en el Suffix Trie. Esto permite mejorar la precisión y velocidad de las búsquedas.
### Proceso Realizado:
1. Conversión a minúsculas
2. Eliminación de caracteres especiales
3. Tokenización
4. Eliminación de palabras irrelevantes
5. Validación de títulos
6. Indexación

Ejemplo
Entrada original:
"Spider-Man: Homecoming!"
Después del pre-procesamiento:
["spiderman", "homecoming"]

## 2. Pseudocódigo de inserción al Suffix Trie
### Inserción de una palabra:
PROCEDIMIENTO insertarPalabra(palabra, id)

    PARA i DESDE 0 HASTA longitud(palabra)-1 HACER
        sufijo ← substring(palabra, i)
        insertarString(sufijo, id)
    FIN PARA

### Inserción de un sufijo:
PROCEDIMIENTO insertarString(texto, id)

    nodoActual ← raiz

    PARA cada caracter c EN texto HACER

        SI c no existe en hijos de nodoActual ENTONCES
            crear nuevo nodo
            agregarlo a hijos
        FIN SI

        nodoActual ← hijo correspondiente a c

        agregar id al conjunto movieIds del nodoActual

    FIN PARA


## 3. Estructura de datos: Suffix Trie


## 4. Interfaz del programa

