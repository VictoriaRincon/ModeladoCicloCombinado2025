#include <mpi.h>
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

void generar_escenarios_eolicos(EscenariosEolicos &eolicos, int rank)
{
    eolicos.assign(NUM_ESCENARIOS_EOLICOS, std::vector<double>(HORAS));
    std::mt19937 gen(rank * 5678 + time(0));
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

void empaquetar_solucion(const Solucion &sol, std::vector<double> &buffer)
{
    buffer.clear();
    buffer.reserve(1 + HORAS * 2);
    buffer.push_back(sol.fitness);
    for (bool estado : sol.estado_on_off)
    {
        buffer.push_back(static_cast<double>(estado));
    }
    for (double pot : sol.potencia_despachada)
    {
        buffer.push_back(pot);
    }
}

Solucion desempaquetar_solucion(const std::vector<double> &buffer)
{
    Solucion sol;
    sol.fitness = buffer[0];
    for (int i = 0; i < HORAS; ++i)
    {
        sol.estado_on_off[i] = static_cast<bool>(buffer[1 + i]);
    }
    for (int i = 0; i < HORAS; ++i)
    {
        sol.potencia_despachada[i] = buffer[1 + HORAS + i];
    }
    return sol;
}

void escribir_resultados(const Solucion &mejor_solucion_global, const DemandaSemanal &demanda, const EscenariosEolicos &eolicos, long long duracion_ms)
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

        archivo_res << h << "," << (mejor_solucion_global.estado_on_off[h] ? "ON" : "OFF") << ","
                    << mejor_solucion_global.potencia_despachada[h] << ","
                    << demanda_neta_promedio << "\n";
    }
    archivo_res << "Mejor costo encontrado: " << std::fixed << std::setprecision(2) << mejor_solucion_global.fitness << " USD (promedio por escenario)\n";
    archivo_res.close();

    std::cout << "\n======================================================\n";
    std::cout << "Optimización finalizada.\n";
    std::cout << "Mejor costo encontrado: " << std::fixed << std::setprecision(2) << mejor_solucion_global.fitness << " USD\n";
    std::cout << "Resultados guardados en la carpeta 'resultados'.\n";
    std::cout << "======================================================\n";
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    auto tiempo_inicio = std::chrono::high_resolution_clock::now();

    if (size < 2)
    {
        if (rank == 0)
        {
            std::cerr << "Error: El modelo de islas requiere al menos 2 procesos MPI (islas)." << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    DemandaSemanal demanda(HORAS);
    EscenariosEolicos eolicos(NUM_ESCENARIOS_EOLICOS, std::vector<double>(HORAS));

    if (rank == 0)
    {
        std::cout << "Proceso 0 (maestro) cargando datos y generando escenarios eólicos..." << std::endl;
        try
        {
            demanda = cargar_demanda("../data/demanda_semanal.csv");
            generar_escenarios_eolicos(eolicos, rank);
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << e.what() << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Bcast(demanda.data(), HORAS, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    for (int s = 0; s < NUM_ESCENARIOS_EOLICOS; ++s)
    {
        MPI_Bcast(eolicos[s].data(), HORAS, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }

    if (rank == 0)
        std::cout << "Inicializando " << size << " islas... Cada una con su propia población aleatoria.\n"
                  << std::endl;
    AlgoritmoGenetico isla_local(rank);

    Solucion mejor_solucion_global;

    using Clock = std::chrono::steady_clock;
    auto last_log_time = Clock::now();
    double last_logged_fitness = std::numeric_limits<double>::infinity();
    const double EPS = 1e-6; // tolerancia para comparar doubles

    int num_epocas = NUM_GENERACIONES_TOTAL / GENERACIONES_POR_EPOCA;
    for (int epoca = 0; epoca < num_epocas; ++epoca)
    {
        isla_local.evolucionar_epoca(demanda, eolicos);

        const Solucion &mejor_local = isla_local.obtener_mejor_individuo();

        if (rank == 0)
        {
            if (std::abs(mejor_local.fitness - last_logged_fitness) > EPS)
            {
                auto now = Clock::now();
                double dt_secs = std::chrono::duration<double>(now - last_log_time).count();

                std::cout << "Época " << epoca + 1 << "/" << num_epocas
                          << " | Mejor fitness en isla 0: " << std::fixed << std::setprecision(2) << mejor_local.fitness
                          << " | Δt desde anterior: " << std::setprecision(3) << dt_secs << " s"
                          << std::endl;

                last_logged_fitness = mejor_local.fitness;
                last_log_time = now;
            }
        }
        const int buffer_size = 1 + HORAS * 2;
        std::vector<double> buffer_envio(buffer_size * NUM_ELITES_MIGRACION);
        std::vector<double> buffer_recepcion(buffer_size * NUM_ELITES_MIGRACION);

        auto &poblacion_local_ref = isla_local.obtener_poblacion();
        for (int i = 0; i < NUM_ELITES_MIGRACION; ++i)
        {
            std::vector<double> temp_buffer;
            empaquetar_solucion(poblacion_local_ref[i], temp_buffer);
            std::copy(temp_buffer.begin(), temp_buffer.end(), buffer_envio.begin() + i * buffer_size);
        }

        int dest = (rank + 1) % size;
        int source = (rank - 1 + size) % size;

        MPI_Sendrecv(buffer_envio.data(), buffer_envio.size(), MPI_DOUBLE, dest, 0,
                     buffer_recepcion.data(), buffer_recepcion.size(), MPI_DOUBLE, source, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        auto &poblacion_local = isla_local.obtener_poblacion();
        for (int i = 0; i < NUM_ELITES_MIGRACION; ++i)
        {
            std::vector<double> temp_buffer(buffer_recepcion.begin() + i * buffer_size, buffer_recepcion.begin() + (i + 1) * buffer_size);
            poblacion_local[TAMANO_POBLACION - 1 - i] = desempaquetar_solucion(temp_buffer);
        }
    }

    for (auto &sol : isla_local.obtener_poblacion())
    {
        if (sol.fitness == std::numeric_limits<double>::max())
        {
            sol.calcular_fitness(UnidadTermica(), demanda, eolicos);
        }
    }
    std::sort(isla_local.obtener_poblacion().begin(), isla_local.obtener_poblacion().end());
    const Solucion &mejor_final_local = isla_local.obtener_mejor_individuo();

    const int buffer_size = 1 + HORAS * 2;
    std::vector<double> buffer_envio_final;
    empaquetar_solucion(mejor_final_local, buffer_envio_final);

    std::vector<double> buffer_gather(size * buffer_size);
    MPI_Gather(buffer_envio_final.data(), buffer_size, MPI_DOUBLE,
               buffer_gather.data(), buffer_size, MPI_DOUBLE,
               0, MPI_COMM_WORLD);

    if (rank == 0)
    {
        for (int i = 0; i < size; ++i)
        {
            std::vector<double> temp_buffer(buffer_gather.begin() + i * buffer_size, buffer_gather.begin() + (i + 1) * buffer_size);
            Solucion sol_recibida = desempaquetar_solucion(temp_buffer);
            if (sol_recibida.fitness < mejor_solucion_global.fitness)
            {
                mejor_solucion_global = sol_recibida;
            }
        }

        auto tiempo_fin = std::chrono::high_resolution_clock::now();
        auto duracion = std::chrono::duration_cast<std::chrono::milliseconds>(tiempo_fin - tiempo_inicio);

        escribir_resultados(mejor_solucion_global, demanda, eolicos, duracion.count());
    }

    MPI_Finalize();
    return 0;
}
