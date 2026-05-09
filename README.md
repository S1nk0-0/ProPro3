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
        
Ejemplo

Para la palabra:

"barco"

Se insertan los siguientes sufijos:

barco
arco
rco
co
o

## 3. Estructura de datos: Suffix Trie
El proyecto utiliza un Suffix Trie para permitir búsquedas rápidas de substrings dentro de títulos, géneros, directores, reparto y sinopsis.

### Nodo del Trie
struct TrieNode {
    unordered_map<char, TrieNode*> children;
    unordered_set<int> movieIds;
};

### Componentes
children

Mapa que almacena las conexiones hacia los nodos hijos.

movieIds

Conjunto de IDs de películas asociadas al substring representado por el nodo.

### Funcionamiento

#### Inserción

Cada palabra genera todos sus sufijos y estos se almacenan en el Trie.

#### Búsqueda

La consulta recorre el árbol carácter por carácter:

Si el camino existe → devuelve películas relacionadas.
Si no existe → no hay coincidencias.
Complejidad
Inserción
O(n
2
)

donde n es la longitud de la palabra.

Búsqueda
O(m)

donde m es la longitud del query.

Ventajas
Búsqueda rápida de substrings.
Coincidencias parciales eficientes.
Mejora la experiencia de búsqueda.
Desventajas
Alto consumo de memoria.
Mayor costo de inserción comparado con un Trie normal.


## 4. Interfaz del programa

