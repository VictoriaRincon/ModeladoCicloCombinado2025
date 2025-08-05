#ifndef CALCULADOR_COSTOS_MAQUINA_HPP
#define CALCULADOR_COSTOS_MAQUINA_HPP

struct RespuestaMaquina
{
    double costo;
    double encendida; //Marca cuantas horas de encendido necesita la máquina
};

RespuestaMaquina calcular_costo(double esc, double demanda_h, double hora, double pot_disponible, double);

#endif