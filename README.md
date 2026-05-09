# Programación III: Proyecto Final (2026-1)

## Integrantes

* Enzo Matias Calderon
* Ferrel Valentino Infante Garcia
* Sebastian Gonzalo Casas Matos

## Descripción

Plataforma de búsqueda y visualización de películas implementada en C++.
Permite buscar películas por título, sinopsis, género, director o actor
usando un Suffix Trie como estructura de datos principal.

---

## 1. Pre-procesamiento de datos

Antes de insertar cualquier texto al Suffix Trie, cada campo pasa por `preprocesar()`:

| Paso | Operación | Ejemplo |
|------|-----------|---------|
| 1 | Recorrer carácter a carácter | `"Dream-sharing, AI!"` |
| 2 | Si es alfanumérico → acumular en minúscula | `d`, `r`, `e`, `a`, `m` → `"dream"` |
| 3 | Si no es alfanumérico y hay acumulado → guardar token (solo si longitud > 1) | `"-"` → guarda `"dream"`, descarta `"a"` |
| 4 | Repetir hasta fin del texto | resultado: `["dream", "sharing"]` |

**Campos indexados por película:** título, sinopsis, género, director, cast.

```
preprocesar("Dream-sharing, AI!")  →  ["dream", "sharing"]
preprocesar("Sci-Fi, Action")      →  ["sci", "fi", "action"]
preprocesar("A Beautiful Mind")    →  ["beautiful", "mind"]
```

> Los tokens de 1 carácter ("a", "I") se descartan para reducir el tamaño del índice.

---

## 2. Pseudocódigo de inserción al Suffix Trie

```
FUNCIÓN cargarCSV(archivo):
    leer encabezados → detectar índice de cada columna

    PARA CADA línea en el CSV:
        campos ← parsearCSV(línea)          // maneja comas dentro de comillas
        película ← {id, título, año, género, director, cast, sinopsis}

        PARA CADA campo_de_texto en {título, sinopsis, género, director, cast}:
            tokens ← preprocesar(campo_de_texto)

            PARA CADA token en tokens:
                trie.indexWord(token, película.id)

        agregar película al vector global

FUNCIÓN indexWord(palabra, id):
    PARA i DESDE 0 HASTA longitud(palabra) - 1:
        sufijo ← palabra[i .. fin]          // "barco" → "barco","arco","rco","co","o"
        insertarCadena(sufijo, id)

FUNCIÓN insertarCadena(cadena, id):
    nodo ← raíz
    PARA CADA carácter c en cadena:
        SI c no está en nodo.hijos:
            nodo.hijos[c] ← nuevo TrieNode
        nodo ← nodo.hijos[c]
        nodo.movieIds.insertar(id)          // id queda registrado en CADA nodo del camino
```

---

## 3. Estructura de datos: Suffix Trie

### ¿Por qué Suffix Trie y no Trie simple?

Con un Trie normal, buscar `"bar"` solo encuentra palabras que **empiezan** con `"bar"`.
Al insertar todos los sufijos de cada palabra, **cualquier subcadena se vuelve buscable**.

```
"barco"  →  sufijos insertados: "barco", "arco", "rco", "co", "o"

Búsqueda "arco" → encontrado ✓   (aunque no es el inicio de la palabra)
Búsqueda "bar"  → encontrado ✓
Búsqueda "rco"  → encontrado ✓
```

### Estructura de un nodo

```
TrieNode {
    hijos:    unordered_map<char, TrieNode*>   // un puntero por carácter posible
    movieIds: unordered_set<int>               // IDs de películas que pasan por aquí
}
```

> Los IDs se guardan en **cada nodo del camino**, no solo en el nodo hoja.
> Esto permite que al terminar de recorrer el query, el nodo ya tenga los IDs correctos.

### Algoritmo de inserción

```
insertarCadena("arco", id=42):

raíz
 └─ 'a'  → movieIds: {42}
     └─ 'r'  → movieIds: {42}
         └─ 'c'  → movieIds: {42}
             └─ 'o'  → movieIds: {42}
```

Si otra película (id=7) también contiene "arco":

```
raíz
 └─ 'a'  → movieIds: {42, 7}
     └─ 'r'  → movieIds: {42, 7}
         ...
```

### Algoritmo de búsqueda

```
FUNCIÓN search(query):
    nodo ← raíz
    PARA CADA carácter c en query:
        SI c no está en nodo.hijos:
            RETORNAR {}          // subcadena no existe en ninguna película
        nodo ← nodo.hijos[c]
    RETORNAR nodo.movieIds       // IDs de todas las películas que contienen el query
```

**Complejidad:**
| Operación | Complejidad |
|-----------|-------------|
| Inserción de una palabra de longitud L | O(L²) — inserta L sufijos de longitud promedio L/2 |
| Búsqueda de un query de longitud m | O(m) — recorre m nodos |

---

## 4. Interfaz del programa

```
=== Streaming Platform ===
34886 peliculas cargadas.

[1] Buscar  [0] Salir
> 1
Buscar: inception dream

  Resultados 1-10 de 23:
  --------------------------------------------------
  [1] Inception (2010)  |  Sci-Fi, Action
  [2] Dreamscape (1984)  |  Sci-Fi
  [3] The Cell (2000)  |  Thriller
  ...
  --------------------------------------------------
  [#] Ver detalle   [n] Siguiente   [0] Volver
  > 1

  Titulo:   Inception
  Genero:   Sci-Fi, Action
  Director: Christopher Nolan
  Sinopsis: A thief who steals corporate secrets through dream-sharing...
```

---

## Base de datos

Dataset: [link](https://drive.google.com/file/d/1UJkRuCF8UD92W_DT7S8dXCYzaR_9wqB_/view?usp=sharing)

Columnas usadas: `Release Year`, `Title`, `Origin/Ethnicity`, `Director`, `Cast`, `Genre`, `Plot`

## Compilar y ejecutar

Abrir el proyecto en CLion y ejecutar `main.cpp`.

## Avance actual

- [x] Lectura y pre-procesamiento del CSV
- [x] Inserción en Suffix Trie
- [x] Búsqueda por texto (palabra, sub-palabra, frase)
- [x] Interfaz con paginación y detalle de película
- [ ] Ranking de resultados
- [ ] Like y Ver más tarde
- [ ] Recomendaciones
