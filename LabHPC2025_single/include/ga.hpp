#ifndef GA_HPP
#define GA_HPP

#include <vector>
#include <random>
#include "../include/solucion.hpp"
#include "../include/config.hpp"
#include "../include/unidad_termica.hpp"

class AlgoritmoGenetico {
private:
    std::vector<Solucion> poblacion;
    UnidadTermica unidad;
    std::mt19937 generador_aleatorio; // Motor de números aleatorios

    Solucion seleccionar_padre();

    Solucion cruzar(const Solucion& padre1, const Solucion& padre2);

    void mutar(Solucion& individuo);

public:
    AlgoritmoGenetico(int rank);

    void evolucionar_epoca(const DemandaSemanal& demanda, const EscenariosEolicos& eolicos);

    std::vector<Solucion>& obtener_poblacion();

    const Solucion& obtener_mejor_individuo() const;
};

#endif
