#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>

const int HORAS = 168;
const int NUM_ESCENARIOS_EOLICOS = 50;

const double POTENCIA_MIN_CC = 150.0;
const double POTENCIA_MAX_CC = 450.0;

const double RAMPA_SUBIDA_HORA = 120.0;
const double RAMPA_BAJADA_HORA = 120.0;
const double RAMPA_ARRANQUE_HORA = 150.0;

const double COSTO_ARRANQUE = 50000.0;
const double COSTO_PARADA = 10000.0;
const double COSTO_FIJO_POR_hora = 1500.0;

// Costo(P) = A*P^2 + B*P
const double COEF_A_COSTO_VAR = 0.02;
const double COEF_B_COSTO_VAR = 25.0;

const double PENALIZACION_RESTRICCION = 1e6;


// ================== PARÁMETROS DEL ALGORITMO GENÉTICO (MODELO DE ISLAS) ==================
const int NUM_ISLAS = 32;                   // Número de poblaciones (islas) a simular
const int TAMANO_POBLACION = 1000;         // Individuos por isla
const int NUM_GENERACIONES_TOTAL = 10000000;   // Criterio de parada principal
const int GENERACIONES_POR_EPOCA = 50;    // Generaciones entre migraciones
const int NUM_ELITES_MIGRACION = 20;       // Cuántos individuos migran
const double PROBABILIDAD_CRUCE = 0.85;
const double PROBABILIDAD_MUTACION = 0.05; // Prob. de mutar el estado ON/OFF de una hora
const int TAMANO_TORNEO = 3;              // Para la selección por torneo


using DemandaSemanal = std::vector<double>;

using EscenariosEolicos = std::vector<std::vector<double>>;

#endif // CONFIG_HPP
