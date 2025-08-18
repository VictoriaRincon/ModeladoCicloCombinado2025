#include "../include/unidad_termica.hpp"
#include <stdexcept>
#include <algorithm>

UnidadTermica::UnidadTermica() {}

// C(P) = a*P^2 + b*P
double UnidadTermica::costoVariable(double potencia_mw) const {
    if (potencia_mw > 0 && potencia_mw < POTENCIA_MIN_CC) {
        // Penalize for operating below minimum, but calculate cost at minimum stable point
        return (COEF_A_COSTO_VAR * POTENCIA_MIN_CC * POTENCIA_MIN_CC + COEF_B_COSTO_VAR * POTENCIA_MIN_CC) + PENALIZACION_RESTRICCION;
    }
    if (potencia_mw > POTENCIA_MAX_CC) {
         // Penalize for operating above maximum
        return (COEF_A_COSTO_VAR * potencia_mw * potencia_mw + COEF_B_COSTO_VAR * potencia_mw) + PENALIZACION_RESTRICCION * (potencia_mw - POTENCIA_MAX_CC);
    }
    return COEF_A_COSTO_VAR * potencia_mw * potencia_mw + COEF_B_COSTO_VAR * potencia_mw;
}

double UnidadTermica::costoFijo() const {
    return COSTO_FIJO_POR_hora;
}

double UnidadTermica::costoArranque() const {
    return COSTO_ARRANQUE;
}

double UnidadTermica::costoParada() const {
    return COSTO_PARADA;
}
