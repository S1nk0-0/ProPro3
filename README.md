# Programación III: Proyecto Final (2026-1)

## Integrantes

* Enzo Matias Calderon
* Ferrel Valentino Infante Garcia
* Sebastian Gonzalo Casas Matos

## Descripción

Plataforma de streaming en consola que permite buscar películas por título, sinopsis, tag (director/género/cast), dar Like, agregarlas a "Ver más tarde" y recibir recomendaciones basadas en los Likes del usuario. Todo el programa está en dos archivos: `Trie.cpp` (estructura de datos) y `main.cpp` (carga, búsqueda, recomendaciones e interfaz).

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

## 2. El árbol: Suffix Trie genérico

`Trie.cpp` implementa el árbol usado para indexar todo el dataset (título, sinopsis, género, director, cast).

```cpp
template <typename T>
struct TrieNode {
    unordered_map<char, TrieNode<T>*> children;
    unordered_set<T> values;
};

template <typename T>
class SuffixTrie {
    void indexWord(const string& word, const T& value);
    unordered_set<T> search(const string& query) const;
    void merge(SuffixTrie<T>& otro);
};
```

### Inserción de una palabra
```
PROCEDIMIENTO indexWord(palabra, valor)
    PARA i DESDE 0 HASTA longitud(palabra)-1 HACER
        sufijo ← substring(palabra, i)
        insertString(sufijo, valor)
```

### Inserción de un sufijo
```
PROCEDIMIENTO insertString(texto, valor)
    nodoActual ← raiz
    PARA cada caracter c EN texto HACER
        SI c no existe en hijos de nodoActual ENTONCES
            crear nuevo nodo
            agregarlo a hijos
        nodoActual ← hijo correspondiente a c
        agregar valor al conjunto values del nodoActual
```

Para la palabra `"barco"` se insertan los sufijos: `barco`, `arco`, `rco`, `co`, `o`.

### Búsqueda
La consulta recorre el árbol carácter por carácter. Si el camino existe, devuelve los valores asociados al último nodo; si no, no hay coincidencias.

### Complejidad
- Inserción de una palabra: `O(n²)`, con `n` = longitud de la palabra
- Búsqueda: `O(m)`, con `m` = longitud del query

### Ventajas / Desventajas
- (+) Búsqueda rápida de substrings y coincidencias parciales.
- (−) Alto consumo de memoria e inserción más costosa que un Trie tradicional.

En `main.cpp` se instancian 5 tries: uno general (`trie`), uno solo de títulos (`titleTrie`, usado también para coincidencias por substring en el título) y tres por tag (`directorTrie`, `castTrie`, `genreTrie`) para la búsqueda por etiqueta.

## 3. Programación Genérica

`SuffixTrie<T>` y `TrieNode<T>` son plantillas: la estructura no conoce el tipo de dato que indexa. En el proyecto se instancian como `SuffixTrie<int>` (el valor guardado es el id de la película), pero la misma clase serviría para indexar cualquier otro identificador sin cambiar una línea del árbol.

## 4. Programación Paralela

La carga del CSV se hace en dos etapas:
1. **Lectura y parseo del CSV** (secuencial, un solo hilo): se lee el archivo línea por línea y se construye el `vector<Movie>`.
2. **Indexación** (paralela): el vector de películas se reparte en `N` bloques, donde `N = std::thread::hardware_concurrency()`. Cada hilo (`indexarRango`) construye sus propios tries **locales** (sin memoria compartida con otros hilos), evitando así condiciones de carrera. Al terminar todos los hilos, se fusionan los tries locales dentro de los tries globales con `SuffixTrie::merge`.

```cpp
vector<thread> hilos;
for (unsigned t = 0; t < nHilos; t++) {
    hilos.emplace_back(indexarRango, desde, hasta,
                        ref(trieParcial[t]), ref(titleParcial[t]), ...);
}
for (auto& h : hilos) h.join();

for (unsigned t = 0; t < nHilos; t++) {
    trie.merge(trieParcial[t]);
    ...
}
```

### Comparación de tiempos (dataset completo: 34,635 películas, Apple M-series, 14 núcleos)

| Modo                          | Tiempo total (carga + indexación) |
|-------------------------------|:----------------------------------:|
| Secuencial (1 hilo)           | 38.4 s                              |
| Paralelo (14 hilos)           | 11.6 s                              |
| **Speedup**                   | **~3.3x**                          |

El speedup no es lineal con el número de núcleos porque la lectura/parseo del CSV (paso 1) sigue siendo secuencial y porque el paso final de `merge` también es secuencial (Amdahl's law); aun así, la etapa de indexación —la más costosa— se paraleliza completamente.

## 5. Patrones de diseño

| Patrón | Dónde | Para qué |
|---|---|---|
| **Factory Method** | `crearPelicula(...)` en `main.cpp` | Encapsula la construcción y validación de un `Movie` a partir de una fila del CSV. |
| **Strategy** | `IEstrategiaRanking` / `RankingPorCobertura` en `main.cpp` | El algoritmo que rankea los resultados de una búsqueda es intercambiable sin tocar el código que arma los conteos de coincidencias. |
| **Singleton** | `Sesion::instancia()` en `main.cpp` | Una única sesión de usuario (likes, ver más tarde) durante la ejecución del programa, con persistencia en disco. |
| **Observer** | `Sesion::suscribirLike(...)` + `recalcularRecomendaciones` en `main.cpp` | Cuando el usuario da Like a una película, `Sesion` notifica a los observadores suscritos para recalcular automáticamente las recomendaciones. |

## 6. Algoritmo de recomendación (propio)

Para "películas similares" se usa **similitud de Jaccard** sobre el conjunto de tokens de género + director + cast:

```
similitud(A, B) = |tokens(A) ∩ tokens(B)| / |tokens(A) ∪ tokens(B)|
```

Las recomendaciones que se muestran al usuario son la unión de las películas similares a cada uno de sus Likes, ponderadas por qué tan arriba salieron en cada comparación.

## 7. Interfaz del programa

Interfaz de consola interactiva.

### Inicio
Al arrancar, se carga el CSV, se indexa en paralelo y se muestran:
- La lista de "Ver más tarde" guardada de sesiones anteriores.
- Recomendaciones basadas en los Likes previos.

```txt
=== Streaming Platform ===
34635 peliculas cargadas.

-- Ver mas tarde --
  * The Matrix

-- Porque te gusto algo similar --
  * The Matrix Revolutions
  * The Matrix Reloaded
  ...
```

### Menú principal
```txt
=== Menu ===
  [1] Buscar peliculas
  [2] Buscar por tag (director/genero/cast)
  [3] Ver mi lista (Ver mas tarde)
  [4] Ver recomendaciones
  [0] Salir
```

### Búsqueda por texto libre
Encuentra coincidencias por palabra, frase o sub-palabra en título/sinopsis/género/director/cast, y muestra las 5 más relevantes con opción de ver 5 más.

```txt
Buscar (titulo, sinopsis, frase o sub-palabra): matrix
[1] The Matrix (1999)
[2] The Matrix Revolutions (2003)
[3] The Matrix Reloaded (2003)
[4] Kireedam (2007)
[5] Spaceballs (1987)

  Numero = ver detalle | [m] ver 5 mas | [0] volver
```

### Búsqueda por tag
```txt
Buscar por: [1] Director  [2] Genero  [3] Cast
> 1
Valor a buscar: Wachowski
```

### Detalle de una película
Al elegir un número se muestra la sinopsis completa y las opciones:
```txt
  [1] Like
  [2] Ver mas tarde
  [0] Volver
```

### Flujo general
```txt
CSV → Parseo secuencial → Indexacion paralela (Suffix Trie) → Busqueda/Ranking → Resultados
                                                             ↘ Like → Observer → Recomendaciones (Jaccard)
```
