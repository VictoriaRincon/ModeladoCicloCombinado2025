#include "../include/unidad_termica.hpp"
#include <stdexcept>

UnidadTermica::UnidadTermica() {}

// C(P) = a*P^2 + b*P
double UnidadTermica::costoVariable(double potencia_mw) const {
    if (potencia_mw < POTENCIA_MIN_CC && potencia_mw > 0) {
        return COEF_A_COSTO_VAR * POTENCIA_MIN_CC * POTENCIA_MIN_CC + COEF_B_COSTO_VAR * POTENCIA_MIN_CC;
    }
    if (potencia_mw > POTENCIA_MAX_CC) {
        throw std::runtime_error("Potencia solicitada excede la máxima.");
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
