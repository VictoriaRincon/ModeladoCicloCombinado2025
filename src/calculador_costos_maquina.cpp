#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <limits>
#include "../include/calculador_costos_maquina.hpp"

using namespace std;

const double BASE_COSTO_ARRANQUE = 4208.0;
const double CONSUMO_ARRANQUE_M3 = 511.0;
const double DENSIDAD_GN = 0.717;                           // kg/m3 (valor aproximado)
const double CONSUMO_GN_GPKWH = 167;                        // gramos de GN por kWh
const double CONSUMO_GN_KGPKWH = CONSUMO_GN_GPKWH / 1000.0; // g/kWh a kg/kWh
const double CONSUMO_GN_M3PKWH = CONSUMO_GN_KGPKWH / DENSIDAD_GN;
const double COSTO_GN_USD_M3 = 0.4436;
const double COSTO_ARRANQUE = BASE_COSTO_ARRANQUE + (CONSUMO_ARRANQUE_M3 * COSTO_GN_USD_M3);
const double POTENCIA_MAXIMA_GAS = 171;
const double POTENCIA_MAXIMA_MCC = POTENCIA_MAXIMA_GAS + 563.0;
const double POTENCIA_MAXIMA_CC = (POTENCIA_MAXIMA_GAS * 2) + 563.0;
const double POTENCIA_SUBIDA = 15; // MW/min según el tipo de arranque
const double POTENCIA_MINIMA = 14.6; // MW/min que varie por cada maquina
const double CONSUMO_KG_KWH_POT_MINIMA = 226.9 / DENSIDAD_GN; // kg/kWh para POTENCIA_MINIMA
const int PENALIZACION = std::numeric_limits<int>::max();


double calcular_consumo_especifico(double P, double Pn = 171.0) {
    double x = P / Pn;
    double Ce_kJ_kWh = 855017.3351 * pow(x, 6) - 3263831.368 * pow(x, 5) +
                        5072666.693 * pow(x, 4) - 4120723.88 * pow(x, 3) +
                        1862152.656 * pow(x, 2) - 457492.7165 * x + 62334.27254;
    double Ce_g_kWh = (Ce_kJ_kWh / 66928.0) * 1000.0;
    return Ce_g_kWh / 1000.0; // kg/kWh
}

RespuestaMaquina calcular_costo(double esc, double demanda_h, double hora, double pot_disponible, double horas_apagada)
{

  RespuestaMaquina respuesta;
  respuesta.costo = 0;
  respuesta.encendida = 0;

  if(demanda_h > pot_disponible){
    respuesta.encendida = 1;
    respuesta.costo = PENALIZACION;
    return respuesta;
  }

  // Determinar tipo de arranque según horas_apagada
  double tipo_arranque = 1;
  if (horas_apagada != 0 && horas_apagada <= 2)
    tipo_arranque = 0.2;
  else if (horas_apagada > 2)
    tipo_arranque = 0.5;

  respuesta.encendida = 1;
  if (horas_apagada > 0)
  {
    double costo_mantener = CONSUMO_KG_KWH_POT_MINIMA * COSTO_GN_USD_M3 * POTENCIA_MINIMA * horas_apagada;

    if (costo_mantener < COSTO_ARRANQUE)
    {
      std::cout << "Entre h=" << hora << " y h=" << hora - horas_apagada
                << " conviene mantener prendida (costo mantener: "
                << costo_mantener << " < arranque: "
                << COSTO_ARRANQUE << ")\n";
      respuesta.encendida += horas_apagada; // Marca que la máquina estuvo encendida
      respuesta.costo += costo_mantener;
    }
    else
    {
      std::cout << "Entre h=" << hora << " y h=" << hora - horas_apagada
                << " conviene mantener apagada (costo mantener: "
                << costo_mantener << " > arranque: "
                << COSTO_ARRANQUE << ")\n";
      respuesta.costo += (COSTO_ARRANQUE * tipo_arranque);
    }
  }

  if (hora == 0) respuesta.costo += COSTO_ARRANQUE;
  
  // Costo de operación por hora

  double ce_kg_kWh = calcular_consumo_especifico(demanda_h / pot_disponible);
  double consumo_gn_m3 = (demanda_h * ce_kg_kWh * pot_disponible)/DENSIDAD_GN;
  respuesta.costo += consumo_gn_m3 * COSTO_GN_USD_M3;

  // Tiempo en minutos para alcanzar la demanda (si parte de 0)
  double tiempo_subida = demanda_h / POTENCIA_SUBIDA; // en minutos
  int horas_subida = static_cast<int>(ceil(tiempo_subida / 60.0));

  if(respuesta.encendida < horas_subida)
  {
    respuesta.encendida += horas_subida; // Marca que la máquina estuvo encendida
  }

  return respuesta;
}
