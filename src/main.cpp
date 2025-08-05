#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>

#include "../include/calculador_costos_maquina.hpp"

using namespace std;

// Constantes energéticas
const double POT_GAS1 = 171;
const double POT_GAS2 = 171 + 171;
const double POT_VAPOR = 563;
const double CONSUMO_GN_M3PKWH = 0.167 / 0.717;
const double COSTO_GN_USD_M3 = 618;
const double COSTO_AGUA = std::numeric_limits<double>::max();
const double COSTO_ARRANQUE = 4208.0 + (5.110 * COSTO_GN_USD_M3);

vector<double> obtener_demanda()
{
    std::string ruta = "../data/parametros.in";
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

    if (size < 4 || (((size - 1) % 3) != 0))
    {
        if (rank == 0)
            std::cerr << "Se requieren al menos 4 procesos y que sean multiplo de 3 + 1.\n";
        MPI_Finalize();
        return 1;
    }
    std::ofstream archivo_resultado;
    long costo_total_operacion = 0.0;
    std::vector<std::tuple<double, double, double>> *encender = new std::vector<std::tuple<double, double, double>>(demanda.size(), {0, 0, 0});

    if (rank == 0)
    {
        archivo_resultado.open("../resultados/seleccion_maquinas.csv");
        archivo_resultado << "Eolica,Hora,MaquinaSeleccionada(1-Gas1, 2-Gas2, 3-Vapor),Costo,Encendida\n";
    }

    for (int eolica = 0; eolica <= 1464; ++eolica) // Maxima eolica 1464
    {

        int horas_apagada = 0;
        double costo_operacion = 0.0;

        std::vector<RespuestaMaquina> respuestas_local(demanda.size(), {0, 0});
        if (rank != 0)
        {
            double potencia_disponible = 0;

            int s = (size - 1) / 3;
            int grupo = (rank - 1) / s; // 0: gas1, 1: gas2, 2: vapor
            int idx_grupo = (rank - 1) % s;
            int horas_por_proceso = cantidad_horas / s;
            int resto = cantidad_horas % s;

            // Calculá el rango exclusivo para cada proceso dentro del grupo:
            int inicio = idx_grupo * horas_por_proceso + std::min(idx_grupo, resto);
            int fin = inicio + horas_por_proceso + (idx_grupo < resto ? 1 : 0);

            if (grupo == 0)
                potencia_disponible = POT_GAS1;
            else if (grupo == 1)
                potencia_disponible = POT_GAS2;
            else if (grupo == 2)
                potencia_disponible = POT_VAPOR + POT_GAS1 + POT_GAS1;

            for (int h = inicio; h < fin; ++h)
            {

                double demanda_h = demanda[h];

                demanda_h -= eolica;
                if (demanda_h < 0)
                    demanda_h = 0;

                if (demanda_h <= 0.0)
                {
                    horas_apagada++;
                    respuestas_local[h] = {0, 0};
                    continue;
                }
                else
                {
                    respuestas_local[h] = calcular_costo(eolica, demanda_h, h, potencia_disponible, horas_apagada);
                    horas_apagada = 0; // Reseteamos horas apagada
                }
            }
        }
        std::vector<RespuestaMaquina> buffer(size * demanda.size());
        MPI_Gather(respuestas_local.data(), demanda.size() * sizeof(RespuestaMaquina), MPI_BYTE,
                   buffer.data(), demanda.size() * sizeof(RespuestaMaquina), MPI_BYTE,
                   0, MPI_COMM_WORLD);

        if (rank == 0)
        {
            for (int h = 0; h < demanda.size(); ++h)
            {
                double costo_total = 0;
                int mejor_proceso = -1;
                double horas_encendida = 0;
                double menor_costo = std::numeric_limits<double>::max();

                for (int i = 1; i < size; ++i)
                {
                    const auto &resp = buffer[i * cantidad_horas + h];
                    // Obtengo el de menor costo
                    if (resp.costo < menor_costo && resp.encendida > 0)
                    {
                        menor_costo = resp.costo;
                        mejor_proceso = i;
                        horas_encendida = resp.encendida;
                    }
                }

                if (mejor_proceso != -1)
                {
                    costo_total += menor_costo;
                    archivo_resultado << eolica << "," << h << "," << mejor_proceso << "," << menor_costo << "," << horas_encendida << "\n";
                }
                costo_total_operacion += costo_total;
            }
        }
    }
    if (rank == 0)
    {
        archivo_resultado << "Costo total: " << costo_total_operacion << " USD\n";
        archivo_resultado.close();
    }

    MPI_Finalize();
    return 0;
}