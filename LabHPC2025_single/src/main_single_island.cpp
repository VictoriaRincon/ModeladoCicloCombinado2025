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
#include <limits>

#include "../include/config.hpp"
#include "../include/ga.hpp"
#include "../include/solucion.hpp"

// Carga los datos de demanda semanal desde un archivo CSV.
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

// Genera escenarios de producción eólica sintéticos.
void generar_escenarios_eolicos(EscenariosEolicos &eolicos, int seed)
{
    eolicos.assign(NUM_ESCENARIOS_EOLICOS, std::vector<double>(HORAS));
    std::mt19937 gen(seed); // Usa una semilla fija para reproducibilidad
    std::uniform_real_distribution<> dis_factor(0.2, 0.8);
    std::normal_distribution<> dis_ruido(0.0, 50.0);

    const double CAPACIDAD_EOLICA_INSTALADA = 1400.0; // MW

    for (int s = 0; s < NUM_ESCENARIOS_EOLICOS; ++s)
    {
        double factor_base = dis_factor(gen);
        for (int h = 0; h < HORAS; ++h)
        {
            double patron = (sin(h * 2 * M_PI / 24.0) + sin(h * 2 * M_PI / HORAS)) / 2.0; // de -1 a 1
            patron = (patron + 1.0) / 2.0;                                                // de 0 a 1

            double gen_eolica = CAPACIDAD_EOLICA_INSTALADA * factor_base * patron + dis_ruido(gen);

            if (gen_eolica < 0)
                gen_eolica = 0;
            if (gen_eolica > CAPACIDAD_EOLICA_INSTALADA)
                gen_eolica = CAPACIDAD_EOLICA_INSTALADA;
            eolicos[s][h] = gen_eolica;
        }
    }
}

// Escribe la mejor solución encontrada a un archivo de resultados.
void escribir_resultados(const Solucion &mejor_solucion_global, const DemandaSemanal &demanda, const EscenariosEolicos &eolicos, long long duracion_ms)
{
    std::ofstream archivo_res("resultados/mejor_cronograma.csv");
    if (!archivo_res.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo de resultados." << std::endl;
        return;
    }
    archivo_res << "Hora,Estado_ON_OFF,Potencia_Despachada_MW,Demanda_Promedio_Neta_MW\n";

    for (int h = 0; h < HORAS; ++h)
    {
        double demanda_neta_promedio = 0;
        for (int s = 0; s < NUM_ESCENARIOS_EOLICOS; ++s)
        {
            demanda_neta_promedio += std::max(0.0, demanda[h] - eolicos[s][h]);
        }
        demanda_neta_promedio /= NUM_ESCENARIOS_EOLICOS;

        archivo_res << h << "," << (mejor_solucion_global.estado_on_off[h] ? "ON" : "OFF") << ","
                    << mejor_solucion_global.potencia_despachada[h] << ","
                    << demanda_neta_promedio << "\n";
    }
    archivo_res << "Mejor costo encontrado: " << std::fixed << std::setprecision(2) << mejor_solucion_global.fitness << " USD (promedio por escenario)\n";
    archivo_res.close();

    std::cout << "\n======================================================\n";
    std::cout << "Optimización finalizada.\n";
    std::cout << "Mejor costo encontrado: " << std::fixed << std::setprecision(2) << mejor_solucion_global.fitness << " USD\n";
    std::cout << "Tiempo de ejecución: " << duracion_ms / 1000.0 << " segundos\n";
    std::cout << "Resultados guardados en la carpeta 'resultados'.\n";
    std::cout << "======================================================\n";
}

int main(int argc, char **argv)
{
    auto tiempo_inicio = std::chrono::high_resolution_clock::now();

    // 1. Cargar datos y generar escenarios
    DemandaSemanal demanda;
    EscenariosEolicos eolicos;
    try
    {
        demanda = cargar_demanda("data/demanda_semanal.csv");
        generar_escenarios_eolicos(eolicos, 12345); // Semilla fija para consistencia
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    std::cout << "Modelo de Islas (Single-Threaded)" << std::endl;
    std::cout << "Inicializando " << NUM_ISLAS << " islas... Cada una con su propia población aleatoria.\n"
              << std::endl;

    // 2. Inicializar todas las islas en un vector
    std::vector<AlgoritmoGenetico> islas;
    for (int i = 0; i < NUM_ISLAS; ++i)
    {
        islas.emplace_back(i);
    }

    Solucion mejor_solucion_global;
    double mejor_fitness_reportado = std::numeric_limits<double>::max();
    
    // 3. Bucle principal de evolución por épocas
    int num_epocas = NUM_GENERACIONES_TOTAL / GENERACIONES_POR_EPOCA;
    for (int epoca = 0; epoca < num_epocas; ++epoca)
    {
        for (int i = 0; i < NUM_ISLAS; ++i)
        {
            islas[i].evolucionar_epoca(demanda, eolicos);
        }

        // Fase de Migración
        std::vector<std::vector<Solucion>> elites_para_migrar(NUM_ISLAS);
        for (int i = 0; i < NUM_ISLAS; ++i)
        {
            auto &poblacion_origen = islas[i].obtener_poblacion();
            for (int j = 0; j < NUM_ELITES_MIGRACION; ++j)
            {
                elites_para_migrar[i].push_back(poblacion_origen[j]);
            }
        }

        for (int i = 0; i < NUM_ISLAS; ++i)
        {
            int isla_origen_idx = (i - 1 + NUM_ISLAS) % NUM_ISLAS;
            auto &poblacion_destino = islas[i].obtener_poblacion();
            const auto &elites_recibidos = elites_para_migrar[isla_origen_idx];
            for (int j = 0; j < NUM_ELITES_MIGRACION; ++j)
            {
                poblacion_destino[TAMANO_POBLACION - 1 - j] = elites_recibidos[j];
            }
        }
        
        // Imprimir progreso solo si se encuentra un nuevo mínimo en la isla 0
        const Solucion& mejor_actual_isla0 = islas[0].obtener_mejor_individuo();
        if (mejor_actual_isla0.fitness < mejor_fitness_reportado) {
            mejor_fitness_reportado = mejor_actual_isla0.fitness;
            
            auto tiempo_actual = std::chrono::high_resolution_clock::now();
            double elapsed_secs = std::chrono::duration<double>(tiempo_actual - tiempo_inicio).count();

            std::cout << "Época " << std::setw(4) << epoca + 1 << "/" << num_epocas
                      << " | Nuevo mejor fitness (isla 0): " << std::fixed << std::setprecision(2)
                      << mejor_fitness_reportado
                      << " | Tiempo: " << std::fixed << std::setprecision(2) << elapsed_secs << "s"
                      << std::endl;
        }
    }

    // 4. Recolectar y encontrar la mejor solución global final
    UnidadTermica unidad_final;
    for (int i = 0; i < NUM_ISLAS; ++i)
    {
        for (auto &sol : islas[i].obtener_poblacion())
        {
            if (sol.fitness == std::numeric_limits<double>::max())
            {
                sol.calcular_fitness(unidad_final, demanda, eolicos);
            }
        }
        std::sort(islas[i].obtener_poblacion().begin(), islas[i].obtener_poblacion().end());
        
        const Solucion &mejor_de_isla = islas[i].obtener_mejor_individuo();
        if (mejor_de_isla.fitness < mejor_solucion_global.fitness)
        {
            mejor_solucion_global = mejor_de_isla;
        }
    }

    // 5. Escribir resultados finales
    auto tiempo_fin = std::chrono::high_resolution_clock::now();
    auto duracion = std::chrono::duration_cast<std::chrono::milliseconds>(tiempo_fin - tiempo_inicio);
    escribir_resultados(mejor_solucion_global, demanda, eolicos, duracion.count());

    return 0;
}
