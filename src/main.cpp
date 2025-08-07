#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include "../include/DataLoader.hpp"
#include "../include/Constantes.hpp"
#include "../include/calculador_costos_maquina.hpp"

using namespace std;

struct SerieEolicaEvaluada
{
    std::vector<double> wind;
    double costo_estimado;
    std::vector<int> maquinas_por_hora;
};

vector<double> obtener_demanda()
{
    std::string ruta = "../data/demanda.in";
    std::ifstream archivo(ruta);
    std::vector<double> datos;
    std::string linea;
    std::getline(archivo, linea);

    while (std::getline(archivo, linea))
    {

        std::stringstream ss(linea);
        std::string hora, valor_str;
        std::getline(ss, hora, ',');
        std::getline(ss, valor_str, ',');

        double demanda;
        demanda = std::stod(valor_str);
        datos.push_back(demanda);
    }

    return datos;
}

std::vector<double> generar_demanda_aleatoria()
{
    std::vector<double> datos;
    std::srand(static_cast<unsigned int>(std::time(nullptr))); // Semilla aleatoria
    for (int dias = 0; dias < 1; ++dias)
    {
        for (int h = 0; h < 24; ++h)
        {
            double d;
            d = static_cast<double>(std::rand() % 2201); // [0, 2200]
            datos.push_back(d);
        }
    }

    return datos;
}

double distancia(const std::vector<double> &a, const std::vector<double> &b)
{
    double suma = 0.0;
    for (size_t i = 0; i < a.size(); ++i)
        suma += std::abs(a[i] - b[i]);
    return suma;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::vector<double> demanda;
    int cantidad_horas;

    if (rank == 0)
    {
        demanda = obtener_demanda();
        cantidad_horas = demanda.size();
    }

    MPI_Bcast(&cantidad_horas, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank != 0)
    {
        demanda.resize(cantidad_horas);
    }

    MPI_Bcast(demanda.data(), cantidad_horas, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    std::ofstream archivo_resultado;
    long costo_total_operacion = 0.0;
    // std::vector<std::tuple<double, double, double>> *encender = new std::vector<std::tuple<double, double, double>>(demanda.size(), {0, 0, 0});

    if (rank == 0)
    {
        archivo_resultado.open("../resultados/seleccion_maquinas.csv");
        archivo_resultado << "Eolica,Hora,MaquinaSeleccionada(1-Gas1, 2-Gas2, 3-Vapor),Costo\n";
    }

    // Parámetros para anillo
    const int siguiente = (rank + 1) % size;
    const int anterior = (rank - 1 + size) % size;

    // int horas_apagada = 0;
    double costo_operacion = 0.0;

    // Preparar una serie base por proceso (1 valor por hora)
    // std::vector<double> serie_local(cantidad_horas);
    // for (int h = 0; h < cantidad_horas; ++h)
    // {
    //     serie_local[h] = (1464.0 * ((rank + h) % 10)) / 10.0; // combinaciones distintas por rank
    // }

    std::vector<std::vector<double>> wind_data;
    const int num_wind_series = 50;

    if (cantidad_horas <= 0)
    {
        MPI_Finalize();
        return 1;
    }

    if (rank != 0)
    {
        demanda.resize(cantidad_horas);
        wind_data.resize(num_wind_series);
    }

    MPI_Bcast(demanda.data(), cantidad_horas, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Después del MPI_Bcast de cantidad_horas y de la carga en el 0:
    if (rank != 0)
    {
        std::cout << "Cargando " << num_wind_series << " series de viento..." << std::endl;
        wind_data = DataLoader::load_partitioned_wind_series("../data/wind_series", num_wind_series, rank, size);

        // // --- DEBUG: VERIFICAR que cada serie tenga cantidad_horas ---
        // for (int i = 0; i < num_wind_series; ++i)
        // {
        //     if (wind_data[i].size() != cantidad_horas)
        //     {
        //         std::cerr << "Error: Serie " << i << " tiene tamaño " << wind_data[i].size()
        //                   << ", pero se esperan " << cantidad_horas << std::endl;
        //         MPI_Abort(MPI_COMM_WORLD, 1);
        //     }
        // }
        // SOLO los procesos que NO son 0 hacen resize de cada serie
        for (auto &series : wind_data)
            series.resize(cantidad_horas);
    }

    // // Broadcast de los datos de viento:
    // for (int i = 0; i < num_wind_series; ++i)
    //     MPI_Bcast(wind_data[i].data(), cantidad_horas, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    std::vector<SerieEolicaEvaluada> cache_local;
    if (rank != 0)
    {
        for (int serie_id = 0; serie_id < wind_data.size(); ++serie_id)
        {
            const std::vector<double> &wind_serie = wind_data[serie_id];
            std::vector<SeleccionHora> seleccion_horas(cantidad_horas);
            double costo_estimado = evaluar_costo_serie_tres_maquinas(wind_serie, demanda, seleccion_horas);

            std::vector<double> costos_por_hora(cantidad_horas);
            std::vector<int> maquinas_por_hora(cantidad_horas);
            for (int h = 0; h < cantidad_horas; ++h)
            {
                costos_por_hora[h] = seleccion_horas[h].costo;
                maquinas_por_hora[h] = seleccion_horas[h].maquina;
            }

            SerieEolicaEvaluada mi_resultado{wind_serie, costo_estimado, maquinas_por_hora};

            // bool encontrada = false;
            // for (SerieEolicaEvaluada &c : cache_local)
            // {
            //     if (distancia(c.wind, wind_serie) < 1000.0)
            //     {
            //         mi_resultado.costo_estimado = c.costo_estimado;
            //         encontrada = true;
            //         break;
            //     }
            // }

            // // Si no la encontró, la calcula y la agrega al cache
            // if (!encontrada)
            // {
            //     evaluada_local.costo_estimado = evaluar_costo_serie_tres_maquinas(
            //         wind_serie, demand_data, seleccion_horas);
            //     cache_local.push_back(evaluada_local);
            // }

            // // Guardar, enviar, imprimir... usar serie_id
            // std::cout << "Proceso " << rank << " está recorriendo serie: " << serie_id << std::endl;

            // ----- Comunicación en anillo -----
            SerieEolicaEvaluada buffer = mi_resultado;
            for (int paso = 0; paso < size - 1; ++paso)
            {
                // 1. Serializar el tamaño
                int len = buffer.wind.size();
                MPI_Sendrecv_replace(&len, 1, MPI_INT, siguiente, 10, anterior, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                // 2. Serializar el viento
                MPI_Sendrecv_replace(buffer.wind.data(), len, MPI_DOUBLE, siguiente, 11, anterior, 11, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                // 3. Serializar el costo total
                MPI_Sendrecv_replace(&buffer.costo_estimado, 1, MPI_DOUBLE, siguiente, 12, anterior, 12, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                // 4. Serializar las máquinas por hora
                MPI_Sendrecv_replace(buffer.maquinas_por_hora.data(), len, MPI_INT, siguiente, 14, anterior, 14, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                // === Comparar por distancia, guardar mejor ===
                double dist = distancia(mi_resultado.wind, buffer.wind);
                if (dist > 0 && dist < Constantes::UMBRAL_DISTANCIA)
                {
                    if (buffer.costo_estimado < mi_resultado.costo_estimado)
                    {
                        mi_resultado = buffer;
                    }
                }
            }

            // 1. SerieID
            MPI_Send(&serie_id, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            // 2. Viento
            MPI_Send(mi_resultado.wind.data(), cantidad_horas, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);
            // 3. Máquinas por hora
            MPI_Send(mi_resultado.maquinas_por_hora.data(), cantidad_horas, MPI_INT, 0, 3, MPI_COMM_WORLD);
            // 4. Costo total de la serie
            MPI_Send(&mi_resultado.costo_estimado, 1, MPI_DOUBLE, 0, 4, MPI_COMM_WORLD);
        }
    }
    if (rank == 0)
    {
        for (int p = 1; p < size; ++p)
        {
            // double costo;
            // std::vector<double> costos_por_hora(cantidad_horas);
            // std::vector<int> maquinas_por_hora(cantidad_horas);
            // MPI_Recv(&costo, 1, MPI_DOUBLE, p, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // MPI_Recv(costos_por_hora.data(), cantidad_horas, MPI_DOUBLE, p, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // MPI_Recv(maquinas_por_hora.data(), cantidad_horas, MPI_INT, p, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // for (int h = 0; h < cantidad_horas; ++h)
            // {
            //     archivo_resultado << h << "," << costos_por_hora[h] << "," << maquinas_por_hora[h] << "\n";
            // }

            for (int serie_id = p; serie_id < num_wind_series; serie_id += size)
            {
                std::vector<double> wind(cantidad_horas);
                std::vector<double> costos_por_hora(cantidad_horas);
                std::vector<int> maquinas_por_hora(cantidad_horas);
                double costo_total;
                int serie_id_rec;

                // 1. SerieID
                MPI_Recv(&serie_id_rec, 1, MPI_INT, p, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // 2. Viento
                MPI_Recv(wind.data(), cantidad_horas, MPI_DOUBLE, p, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // 3. Máquinas por hora
                MPI_Recv(maquinas_por_hora.data(), cantidad_horas, MPI_INT, p, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // 4. Costo total de la serie
                MPI_Recv(&costo_total, 1, MPI_DOUBLE, p, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                for (int h = 0; h < cantidad_horas; ++h)
                {
                    archivo_resultado << serie_id_rec << "," << h << "," << wind[h] << "," << maquinas_por_hora[h] << "," << costo_total << "\n";
                }
            }
        }
        archivo_resultado.close();
    }

    MPI_Finalize();
    return 0;
}