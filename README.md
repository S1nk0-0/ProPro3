# Programación III: Proyecto Final (2026-1)

## Integrantes (3 ó 4)

* Enzo Matias Calderon 
* Ferrel Valentino Infante Garcia
* Sebastian Gonzalo Casas Matos

## Descripción
Plataforma de búsqueda y visualización de películas implementada en C++.
Permite buscar películas por título, sinopsis, género, director o actor
usando un Suffix Trie como estructura de datos principal.

## Estructura de datos: Suffix Trie

Se eligió un **Suffix Trie** porque el enunciado requiere búsqueda de sub-palabras.
Con un Trie normal, buscar `"bar"` solo encontraría palabras que *empiezan* con `"bar"`.
Al insertar todos los sufijos de cada palabra, cualquier substring se vuelve buscable.

**Ejemplo:**
La palabra `"barco"` se indexa como: `"barco"`, `"arco"`, `"rco"`, `"co"`, `"o"`.
Así, buscar `"arco"` funciona aunque no sea el inicio de la palabra.

**Complejidad de búsqueda:** O(m) donde m = longitud del query.

## Pre-procesamiento de datos

Antes de insertar al Trie, cada campo de texto pasa por `preprocesar()`:

1. Convertir a minúsculas
2. Separar por caracteres no alfanuméricos (puntuación, espacios, guiones)
3. Descartar tokens de 1 carácter

**Ejemplo:**
"Dream-sharing, AI!" → ["dream", "sharing", "ai"]

## Base de datos

Dataset: [link] (https://drive.google.com/file/d/1UJkRuCF8UD92W_DT7S8dXCYzaR_9wqB_/view?usp=sharing). 

Columnas usadas: `Release Year`, `Title`, `Origin/Ethnicity`, `Director`, `Cast`, `Genre`, `Plot`

## Compilar y ejecutar

Abrir el proyecto en Clion y darle click a run en main.cpp

## Avance actual

- [x] Lectura y pre-procesamiento del CSV
- [x] Inserción en Suffix Trie
- [x] Búsqueda por texto (palabra, frase, sub-palabra)
- [ ] Ranking de resultados
- [ ] Like y Ver más tarde
- [ ] Recomendaciones
