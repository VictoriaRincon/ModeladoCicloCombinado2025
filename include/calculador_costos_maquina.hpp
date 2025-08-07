#ifndef CALCULADOR_COSTOS_MAQUINA_HPP
#define CALCULADOR_COSTOS_MAQUINA_HPP

struct RespuestaMaquina
{
    double costo;
    double encendida; //Marca cuantas horas de encendido necesita la máquina
};
struct SeleccionHora
{
  int maquina; // 1=Gas1, 2=Gas2, 3=Vapor
  double costo;
  double potencia_maquina;
  double potencia_eolica;
  int horas_apagada; // guardamos cuántas llevaba apagada la máquina seleccionada
};

RespuestaMaquina calcular_costo(double esc, double demanda_h, double hora, double pot_disponible, double);
double evaluar_costo_serie_tres_maquinas(const std::vector<double>& serie_eolica, const std::vector<double>& demanda, std::vector<SeleccionHora>& seleccion_horas);

#endif