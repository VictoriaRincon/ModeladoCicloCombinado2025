#ifndef SOLUCION_HPP
#define SOLUCION_HPP

#include <vector>
#include <random>
#include "../include/config.hpp"
#include "../include/unidad_termica.hpp"

class Solucion {
public:
    std::vector<bool> estado_on_off;

    std::vector<double> potencia_despachada;

    double fitness;

    Solucion();

    void inicializar_aleatoriamente(std::mt19937& gen);

    void calcular_fitness(const UnidadTermica& unidad, const DemandaSemanal& demanda, const EscenariosEolicos& eolicos);

    bool operator<(const Solucion& otra) const {
        return this->fitness < otra.fitness;
    }
};

#endif
