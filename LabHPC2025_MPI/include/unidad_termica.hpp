#ifndef UNIDAD_TERMICA_HPP
#define UNIDAD_TERMICA_HPP

#include "../include/config.hpp"

class UnidadTermica {
public:
    UnidadTermica();

    double costoVariable(double potencia_mw) const;

    double costoFijo() const;

    double costoArranque() const;

    double costoParada() const;
};

#endif
