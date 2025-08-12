#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <numeric>
#include <algorithm>
#include <random>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <iomanip>

#include "../include/config.hpp"
#include "../include/ga.hpp"
#include "../include/solucion.hpp"

DemandaSemanal cargar_demanda(const std::string &ruta_archivo)
{
    DemandaSemanal demanda;
    std::ifstream archivo(ruta_archivo);
    if (!archivo.is_open())
    {
        throw std::runtime_error("Error: No se pudo abrir el archivo de demanda: " + ruta_archivo);
    }
    std::string linea;
    while (std::getline(archivo, linea))
    {
        if (!linea.empty())
        {
            demanda.push_back(std::stod(linea));
        }
    }
    if (demanda.size() != HORAS)
    {
        throw std::runtime_error("Error: El archivo de demanda no contiene " + std::to_string(HORAS) + " horas de datos.");
    }
    return demanda;
}

void generar_escenarios_eolicos(EscenariosEolicos &eolicos)
{
    eolicos.assign(NUM_ESCENARIOS_EOLICOS, std::vector<double>(HORAS));
    std::mt19937 gen(12345);
    std::uniform_real_distribution<> dis_factor(0.2, 0.8);
    std::normal_distribution<> dis_ruido(0.0, 50.0);

    const double CAPACIDAD_EOLICA_INSTALADA = 400.0;
    for (int s = 0; s < NUM_ESCENARIOS_EOLICOS; ++s)
    {
        double factor_base = dis_factor(gen);
        for (int h = 0; h < HORAS; ++h)
        {
            double patron = (sin(h * 2 * M_PI / 24.0) + sin(h * 2 * M_PI / 168.0)) / 2.0;
            patron = (patron + 1.0) / 2.0;
            double gen_eolica = CAPACIDAD_EOLICA_INSTALADA * factor_base * patron + dis_ruido(gen);

            if (gen_eolica < 0.0)
                gen_eolica = 0.0;
            else if (gen_eolica > CAPACIDAD_EOLICA_INSTALADA)
                gen_eolica = CAPACIDAD_EOLICA_INSTALADA;

            eolicos[s][h] = gen_eolica;
        }
    }
}

void escribir_resultados(const Solucion &mejor_solucion, const DemandaSemanal &demanda, const EscenariosEolicos &eolicos, long long duracion_ms)
{
    std::ofstream archivo_res("resultados/mejor_cronograma.csv");
    archivo_res << "Hora,Estado_ON_OFF,Potencia_Despachada_MW,Demanda_Promedio_Neta_MW\n";
    for (int h = 0; h < HORAS; ++h)
    {
        double demanda_neta_promedio = 0;
        for (int s = 0; s < NUM_ESCENARIOS_EOLICOS; ++s)
        {
            demanda_neta_promedio += std::max(0.0, demanda[h] - eolicos[s][h]);
        }
        demanda_neta_promedio /= NUM_ESCENARIOS_EOLICOS;
        archivo_res << h << "," << (mejor_solucion.estado_on_off[h] ? "ON" : "OFF") << ","
                    << mejor_solucion.potencia_despachada[h] << ","
                    << demanda_neta_promedio << "\n";
    }
    archivo_res.close();

    std::ofstream archivo_info("resultados/info_ejecucion.txt");
    archivo_info << "Optimización completada.\n";
    archivo_info << "Mejor costo (fitness): " << std::fixed << std::setprecision(2) << mejor_solucion.fitness << " USD\n";
    archivo_info << "Tiempo total: " << duracion_ms / 1000.0 << " segundos\n";
    archivo_info.close();

    std::cout << "\n======================================================\n";
    std::cout << "Optimización finalizada.\n";
    std::cout << "Mejor costo encontrado: " << std::fixed << std::setprecision(2) << mejor_solucion.fitness << " USD\n";
    std::cout << "Tiempo de ejecución: " << duracion_ms / 1000.0 << " segundos\n";
    std::cout << "Resultados guardados en 'resultados/'.\n";
    std::cout << "======================================================\n";
}

int main()
{
    auto tiempo_inicio = std::chrono::high_resolution_clock::now();

    const double COSTO_OBJETIVO = 175174695812.52; // lo que vos marques
    const double TOLERANCIA = 100000000.0;         // margen de error permitido

    DemandaSemanal demanda = cargar_demanda("data/demanda_semanal.csv");
    EscenariosEolicos eolicos;
    generar_escenarios_eolicos(eolicos);

    AlgoritmoGenetico isla_local(0);

    long generacion_total = 0;
    double mejor_fitness = std::numeric_limits<double>::max();

    using Clock = std::chrono::steady_clock;

    auto t0 = Clock::now();
    auto last_log_time = t0;
    double last_logged_fitness = std::numeric_limits<double>::infinity();
    const double EPS = 1e-6;
    const double OBJ_MEJORA = 0.10; // 10% de mejora deseada
    bool baseline_set = false;
    double baseline_fitness = std::numeric_limits<double>::infinity();

    while (true)
    {
        isla_local.evolucionar_epoca(demanda, eolicos);
        const Solucion &mejor_local = isla_local.obtener_mejor_individuo();
        mejor_fitness = mejor_local.fitness;
        generacion_total += GENERACIONES_POR_EPOCA;
        // Fijar baseline en la primera medición (no toca el formato de impresión)
        if (!baseline_set)
        {
            baseline_fitness = mejor_fitness;
            baseline_set = true;
        }

        // Loguear solo si cambió el mejor fitness
        if (std::abs(mejor_fitness - last_logged_fitness) > EPS)
        {
            auto now = Clock::now();
            double dt_secs = std::chrono::duration<double>(now - last_log_time).count();

            std::cout << "Generación: " << generacion_total
                      << " | Mejor fitness: " << std::fixed << std::setprecision(2) << mejor_fitness
                      << " | Δt desde el anterior: " << std::setprecision(3) << dt_secs << " s"
                      << std::endl;

            last_logged_fitness = mejor_fitness;
            last_log_time = now;
        }

        // Verificar si está dentro del rango deseado
        if (std::abs(mejor_fitness - COSTO_OBJETIVO) <= TOLERANCIA)
        {
            std::cout << "\n*** Objetivo alcanzado: " << mejor_fitness << " ***\n";
            break;
        }

        double mejora_rel = 0.0;
        if (baseline_fitness > 0.0)
        {
            mejora_rel = (baseline_fitness - mejor_fitness) / baseline_fitness;
        }
        if (mejora_rel >= OBJ_MEJORA)
        {
            std::cout << "\n*** Objetivo de mejora alcanzado ("
                      << std::fixed << std::setprecision(2) << (OBJ_MEJORA * 100.0)
                      << "%) ***\n";
            break;
        }
    }

    auto &poblacion = isla_local.obtener_poblacion();
    std::sort(poblacion.begin(), poblacion.end());
    const Solucion &mejor_final = poblacion[0];

    auto tiempo_fin = std::chrono::high_resolution_clock::now();
    auto duracion = std::chrono::duration_cast<std::chrono::milliseconds>(tiempo_fin - tiempo_inicio);

    escribir_resultados(mejor_final, demanda, eolicos, duracion.count());

    return 0;
}