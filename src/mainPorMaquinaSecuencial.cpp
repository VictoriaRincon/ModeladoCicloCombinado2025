#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <ctime>

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

int main()
{
    std::vector<double> demanda = obtener_demanda();

    std::ofstream archivo_resultado;
    long costo_total_operacion = 0.0;
    std::vector<std::tuple<double, double, double>> *encender = new std::vector<std::tuple<double, double, double>>(demanda.size(), {0, 0, 0});

    archivo_resultado.open("../resultados/seleccion_maquinas_secuencial.csv");
    archivo_resultado << "Eolica,Hora,MaquinaSeleccionada(1-Gas1, 2-Gas2, 3-Vapor),Costo,Encendida\n";

    for (int eolica = 0; eolica <= 1464; ++eolica) // Maxima eolica 1464
    {
        std::string tipo_maquina;
        int horas_apagada = 0;
        double costo_operacion = 0.0;
        for (int h = 0; h < demanda.size(); ++h)
        {
            double demanda_h = demanda[h];
            demanda_h -= eolica;
            if (demanda_h < 0)
                demanda_h = 0;

            RespuestaMaquina respuesta;
            respuesta.costo = 0;
            respuesta.encendida = 0;

            std::vector<RespuestaMaquina> respuestas(4);
            if (demanda_h <= 0.0)
            {
                horas_apagada++;
                continue;
            }
            else
            {
                respuestas[0] = calcular_costo(eolica, demanda_h, h, POT_GAS1, horas_apagada);
                respuestas[1] = calcular_costo(eolica, demanda_h, h, POT_GAS2, horas_apagada);
                respuestas[2] = calcular_costo(eolica, demanda_h, h, POT_VAPOR + POT_GAS1 + POT_GAS1, horas_apagada);
                respuestas[3] = calcular_costo(eolica, demanda_h, h, std::numeric_limits<double>::max(), horas_apagada);
                horas_apagada = 0; // Reseteamos horas apagada
            }

            double costo_total = 0;
            int mejor_proceso = -1;
            double horas_encendida = 0;
            double menor_costo = std::numeric_limits<double>::max();

            for (int i = 0; i < 4; ++i)
            {
                // Obtengo el de menor costo
                if (respuestas[i].costo < menor_costo && respuestas[i].encendida > 0)
                {
                    menor_costo = respuestas[i].costo;
                    mejor_proceso = i;
                    horas_encendida = respuestas[i].encendida;
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

    archivo_resultado << "Costo total: " << costo_total_operacion << " USD\n";

    return 0;
}