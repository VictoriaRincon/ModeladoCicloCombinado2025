#ifndef CONSTANTES_HPP
#define CONSTANTES_HPP

#include <vector>
#include <limits>

namespace Constantes {
    
    // Parámetros Físicos y de Costos
    const double POT_GAS_1 = 171.0; // MW
    const double POT_GAS_2 = 171.0; // MW
    const double POT_VAPOR = 563.0; // MW
    
    // Potencias
    const double POTENCIA_MINIMA = 14.6; // MW
    const double POTENCIA_SUBIDA = 15; // MW/min según el tipo de arranque
    const double POTENCIA_MAXIMA_CC = (POT_GAS_1 * 2) + 563.0;

    //Constantes de consumo
    const double CONSUMO_GN_M3PKWH = 0.167 / 0.717;
    const double COSTO_GN_USD_M3 = 0.4436;
    const double DENSIDAD_GN = 0.717;                           // kg/m3 (valor aproximado)
    const double CONSUMO_KG_KWH_POT_MINIMA = 226.9 / DENSIDAD_GN; // kg/kWh para POTENCIA_MINIMA

    // Costos de Arranque (USD)
    const double COSTO_ARRANQUE_TG_CALIENTE = 4208.0; // < 2 horas apagada
    const double COSTO_ARRANQUE_TG_TIBIO = 5110.0;  // 2-8 horas apagada
    const double COSTO_ARRANQUE_TG_FRIO = 6500.0;   // > 8 horas apagada
    const double COSTO_ARRANQUE = 4208.0 + (511.0 * COSTO_GN_USD_M3);

    // Costo de combustible (USD por MWh) - Simplificado
    const double COSTO_COMBUSTIBLE_TG = 65.0; // USD/MWh
    const double COSTO_COMBUSTIBLE_CC = 55.0; // USD/MWh (más eficiente)
    
    // Costo de mantener la turbina encendida en su potencia mínima
    const double COSTO_MANTENER_ENCENDIDA_POR_HORA = POTENCIA_MINIMA * COSTO_COMBUSTIBLE_TG * 1.2; // Penalizado

    // Penalización por no cubrir la demanda (costo muy alto)
    const double PENALIZACION = 1.0e9;

    // Horizontes para tipos de arranque
    const int HORAS_ARRANQUE_CALIENTE = 2;
    const int HORAS_ARRANQUE_FRIO = 8;

    // Costo del agua (USD por m3) - Asumido muy alto para desalentar su uso
    const double COSTO_AGUA = std::numeric_limits<double>::max();

    const double UMBRAL_DISTANCIA = 250.0;
}

#endif // CONSTANTS_HPP
