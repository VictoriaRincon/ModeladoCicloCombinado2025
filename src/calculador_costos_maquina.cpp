#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <limits>
#include "../include/Constantes.hpp"
#include "../include/calculador_costos_maquina.hpp"

using namespace std;

double calcular_consumo_especifico(double P, double Pn = 171.0)
{
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

  if (demanda_h > pot_disponible)
  {
    respuesta.encendida = 1;
    respuesta.costo = Constantes::PENALIZACION;
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
    double costo_mantener = Constantes::CONSUMO_KG_KWH_POT_MINIMA * Constantes::COSTO_GN_USD_M3 * Constantes::POTENCIA_MINIMA * horas_apagada;

    if (costo_mantener < Constantes::COSTO_ARRANQUE)
    {
      std::cout << "Entre h=" << hora << " y h=" << hora - horas_apagada
                << " conviene mantener prendida (costo mantener: "
                << costo_mantener << " < arranque: "
                << Constantes::COSTO_ARRANQUE << ")\n";
      respuesta.encendida += horas_apagada; // Marca que la máquina estuvo encendida
      respuesta.costo += costo_mantener;
    }
    else
    {
      std::cout << "Entre h=" << hora << " y h=" << hora - horas_apagada
                << " conviene mantener apagada (costo mantener: "
                << costo_mantener << " > arranque: "
                << Constantes::COSTO_ARRANQUE << ")\n";
      respuesta.costo += (Constantes::COSTO_ARRANQUE * tipo_arranque);
    }
  }

  if (hora == 0)
    respuesta.costo += Constantes::COSTO_ARRANQUE;

  // Costo de operación por hora

  double ce_kg_kWh = calcular_consumo_especifico(demanda_h / pot_disponible);
  double consumo_gn_m3 = (demanda_h * ce_kg_kWh * pot_disponible) / Constantes::DENSIDAD_GN;
  respuesta.costo += consumo_gn_m3 * Constantes::COSTO_GN_USD_M3;

  // Tiempo en minutos para alcanzar la demanda (si parte de 0)
  double tiempo_subida = demanda_h / Constantes::POTENCIA_SUBIDA; // en minutos
  int horas_subida = static_cast<int>(ceil(tiempo_subida / 60.0));

  if (respuesta.encendida < horas_subida)
  {
    respuesta.encendida += horas_subida; // Marca que la máquina estuvo encendida
  }

  return respuesta;
}


double evaluar_costo_serie_tres_maquinas(
    const std::vector<double> &serie_eolica,
    const std::vector<double> &demanda,
    std::vector<SeleccionHora> &seleccion_horas)
{
  // Contadores de horas apagadas por máquina
  int horas_apagada[3] = {0, 0, 0};
  double costo_total = 0.0;

  for (int h = 0; h < demanda.size(); ++h)
  {
    double demanda_h = demanda[h] - serie_eolica[h];
    if (demanda_h < 0)
      demanda_h = 0;

    RespuestaMaquina r[3];
    r[0] = calcular_costo(serie_eolica[h], demanda_h, h, Constantes::POT_GAS_1, horas_apagada[0]);
    r[1] = calcular_costo(serie_eolica[h], demanda_h, h, Constantes::POT_GAS_2, horas_apagada[1]);
    r[2] = calcular_costo(serie_eolica[h], demanda_h, h, Constantes::POT_VAPOR, horas_apagada[2]);

    // Encuentra menor costo
    double menor_costo = std::min({r[0].costo, r[1].costo, r[2].costo});
    // Busca cuántas máquinas lo alcanzan
    std::vector<int> candidatos;
    for (int i = 0; i < 3; ++i)
    {
      if (std::abs(r[i].costo - menor_costo) < 1e-6) // tolerancia por float
        candidatos.push_back(i);
    }

    // Si hay empate, elige la que tiene más horas apagada
    int seleccionada = candidatos[0];
    int max_horas_apagada = horas_apagada[candidatos[0]];
    for (int idx : candidatos)
    {
      if (horas_apagada[idx] > max_horas_apagada)
      {
        seleccionada = idx;
        max_horas_apagada = horas_apagada[idx];
      }
    }

    // Actualiza: solo la seleccionada resetea su contador
    for (int i = 0; i < 3; ++i)
    {
      if (i == seleccionada)
        horas_apagada[i] = 0;
      else
        horas_apagada[i]++;
    }

    // Guarda resultado
    seleccion_horas[h] = {
        seleccionada + 1, // 1=Gas1, 2=Gas2, 3=Vapor
        r[seleccionada].costo,
        (seleccionada == 0) ? Constantes::POT_GAS_1 : (seleccionada == 1) ? Constantes::POT_GAS_2
                                                                          : Constantes::POT_VAPOR,
        serie_eolica[h],
        max_horas_apagada};
    costo_total += r[seleccionada].costo;
  }
  return costo_total;
}