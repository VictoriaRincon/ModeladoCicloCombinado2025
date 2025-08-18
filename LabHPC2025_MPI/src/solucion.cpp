#include "../include/solucion.hpp"
#include <numeric>
#include <iostream>
#include <cmath>
#include <limits>
#include <algorithm>

Solucion::Solucion() {
    estado_on_off.resize(HORAS, false);
    potencia_despachada.resize(HORAS, 0.0);
    fitness = std::numeric_limits<double>::max();
}

void Solucion::inicializar_aleatoriamente(std::mt19937& gen) {
    std::bernoulli_distribution dist_on_off(0.5); // 50% de probabilidad de estar encendido

    for (int h = 0; h < HORAS; ++h) {
        estado_on_off[h] = dist_on_off(gen);
    }
}

void Solucion::calcular_fitness(const UnidadTermica& unidad, const DemandaSemanal& demanda, const EscenariosEolicos& eolicos) {
    double costo_total_acumulado = 0.0;
    
    std::vector<double> despacho_horario_escenario(HORAS, 0.0);

    for (int s = 0; s < NUM_ESCENARIOS_EOLICOS; ++s) {
        double costo_escenario_actual = 0.0;
        
        for (int h = 0; h < HORAS; ++h) {
            double demanda_neta = std::max(0.0, demanda[h] - eolicos[s][h]);
            bool estado_actual = estado_on_off[h];
            bool estado_anterior = (h > 0) ? estado_on_off[h-1] : false;

            if (estado_actual) {
                double p_min_hora = POTENCIA_MIN_CC;
                double p_max_hora = POTENCIA_MAX_CC;
                
                double p_anterior = (h > 0) ? despacho_horario_escenario[h - 1] : 0.0;

                if (estado_anterior) {
                    p_min_hora = std::max(p_min_hora, p_anterior - RAMPA_BAJADA_HORA);
                    p_max_hora = std::min(p_max_hora, p_anterior + RAMPA_SUBIDA_HORA);
                } else {
                    p_max_hora = std::min(p_max_hora, RAMPA_ARRANQUE_HORA);
                    costo_escenario_actual += unidad.costoArranque();
                }

                if (p_min_hora > p_max_hora) {
                    costo_escenario_actual += PENALIZACION_RESTRICCION;
                    p_min_hora = p_max_hora = POTENCIA_MIN_CC; // Forzar un estado válido para continuar
                }

                double p_despachada = std::max(p_min_hora, std::min(demanda_neta, p_max_hora));
                despacho_horario_escenario[h] = p_despachada;

                costo_escenario_actual += unidad.costoFijo();
                costo_escenario_actual += unidad.costoVariable(p_despachada);

                double demanda_insatisfecha = demanda_neta - p_despachada;
                if (demanda_insatisfecha > 1.0) { // Pequeña tolerancia
                    costo_escenario_actual += demanda_insatisfecha * PENALIZACION_RESTRICCION;
                }
            
            } else {
                despacho_horario_escenario[h] = 0.0;
                if (demanda_neta > 1.0) {
                    costo_escenario_actual += demanda_neta * PENALIZACION_RESTRICCION;
                }
                if (estado_anterior) {
                    costo_escenario_actual += unidad.costoParada();
                }
            }
        }
        costo_total_acumulado += costo_escenario_actual;
    }

    this->fitness = costo_total_acumulado / NUM_ESCENARIOS_EOLICOS;

    DemandaSemanal demanda_neta_promedio(HORAS, 0.0);
    for (int h = 0; h < HORAS; ++h) {
        double eolico_promedio_h = 0;
        for(int s=0; s < NUM_ESCENARIOS_EOLICOS; ++s) {
            eolico_promedio_h += eolicos[s][h];
        }
        eolico_promedio_h /= NUM_ESCENARIOS_EOLICOS;
        demanda_neta_promedio[h] = std::max(0.0, demanda[h] - eolico_promedio_h);
    }
    
    for (int h = 0; h < HORAS; ++h) {
        if (this->estado_on_off[h]) {
            double p_min_hora = POTENCIA_MIN_CC;
            double p_max_hora = POTENCIA_MAX_CC;
            double p_anterior = (h > 0) ? this->potencia_despachada[h-1] : 0.0;
            bool estado_anterior = (h > 0) ? this->estado_on_off[h-1] : false;

            if (estado_anterior) {
                p_min_hora = std::max(p_min_hora, p_anterior - RAMPA_BAJADA_HORA);
                p_max_hora = std::min(p_max_hora, p_anterior + RAMPA_SUBIDA_HORA);
            } else {
                p_max_hora = std::min(p_max_hora, RAMPA_ARRANQUE_HORA);
            }
            if (p_min_hora > p_max_hora) p_min_hora = p_max_hora;
            this->potencia_despachada[h] = std::max(p_min_hora, std::min(demanda_neta_promedio[h], p_max_hora));
        } else {
            this->potencia_despachada[h] = 0.0;
        }
    }
}
