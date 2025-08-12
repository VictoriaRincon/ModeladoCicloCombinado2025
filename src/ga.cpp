#include "../include/ga.hpp"
#include <algorithm>
#include <iostream>
#include <ctime>

AlgoritmoGenetico::AlgoritmoGenetico(int rank) {
    generador_aleatorio.seed(rank * 1234 + time(0));
    poblacion.resize(TAMANO_POBLACION);
    for (auto& individuo : poblacion) {
        individuo.inicializar_aleatoriamente(generador_aleatorio);
    }
}

Solucion AlgoritmoGenetico::seleccionar_padre() {
    std::uniform_int_distribution<> dist_poblacion(0, TAMANO_POBLACION - 1);
    
    Solucion mejor_del_torneo;

    for (int i = 0; i < TAMANO_TORNEO; ++i) {
        int idx = dist_poblacion(generador_aleatorio);
        if (poblacion[idx].fitness < mejor_del_torneo.fitness) {
            mejor_del_torneo = poblacion[idx];
        }
    }
    return mejor_del_torneo;
}

Solucion AlgoritmoGenetico::cruzar(const Solucion& padre1, const Solucion& padre2) {
    Solucion hijo;
    std::uniform_int_distribution<> dist_puntos(0, HORAS - 1);
    
    int punto1 = dist_puntos(generador_aleatorio);
    int punto2 = dist_puntos(generador_aleatorio);
    if (punto1 > punto2) std::swap(punto1, punto2);

    for (int i = 0; i < HORAS; ++i) {
        if (i >= punto1 && i < punto2) {
            hijo.estado_on_off[i] = padre2.estado_on_off[i];
        } else {
            hijo.estado_on_off[i] = padre1.estado_on_off[i];
        }
    }
    return hijo;
}

void AlgoritmoGenetico::mutar(Solucion& individuo) {
    std::uniform_real_distribution<> dist_prob(0.0, 1.0);

    for (int i = 0; i < HORAS; ++i) {
        if (dist_prob(generador_aleatorio) < PROBABILIDAD_MUTACION) {
            individuo.estado_on_off[i] = !individuo.estado_on_off[i];
        }
    }
}

void AlgoritmoGenetico::evolucionar_epoca(const DemandaSemanal& demanda, const EscenariosEolicos& eolicos) {
    // 1. Calcular fitness de toda la población (solo si no ha sido evaluada)
    for (auto& individuo : poblacion) {
        if (individuo.fitness == std::numeric_limits<double>::max()) {
            individuo.calcular_fitness(unidad, demanda, eolicos);
        }
    }
    
    // 2. Ordenar la población para encontrar y preservar al mejor (elitismo)
    std::sort(poblacion.begin(), poblacion.end());

    // 3. Crear nueva generación
    std::vector<Solucion> nueva_poblacion;
    nueva_poblacion.reserve(TAMANO_POBLACION);

    // Elitismo: el mejor individuo pasa directamente a la siguiente generación
    nueva_poblacion.push_back(poblacion[0]);

    // 4. Llenar el resto de la nueva población con hijos de la generación anterior
    std::uniform_real_distribution<> dist_prob(0.0, 1.0);
    while (nueva_poblacion.size() < TAMANO_POBLACION) {
        Solucion padre1 = seleccionar_padre();
        Solucion padre2 = seleccionar_padre();
        
        Solucion hijo;
        if (dist_prob(generador_aleatorio) < PROBABILIDAD_CRUCE) {
            hijo = cruzar(padre1, padre2);
        } else {
            hijo = padre1; // Clonación
        }
        
        mutar(hijo);
        
        nueva_poblacion.push_back(hijo);
    }

    poblacion = nueva_poblacion;
}


std::vector<Solucion>& AlgoritmoGenetico::obtener_poblacion() {
    return poblacion;
}

const Solucion& AlgoritmoGenetico::obtener_mejor_individuo() const {
    return poblacion[0];
}
