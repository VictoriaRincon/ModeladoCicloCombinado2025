#include "../include/DataLoader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

std::vector<double> DataLoader::load_series(const std::string &path)
{
    std::vector<double> values;
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Error: No se pudo abrir el archivo " << path << std::endl;
        return values;
    }

    std::string line;
    // Omitir la cabecera
    std::getline(file, line);

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string part;
        // Omitir la primera columna (hora/índice)
        std::getline(ss, part, ',');
        // Leer el valor
        std::getline(ss, part, ',');
        try
        {
            values.push_back(std::stod(part));
        }
        catch (const std::invalid_argument &e)
        {
            std::cerr << "Error de formato en archivo " << path << " en línea: " << line << std::endl;
        }
    }
    return values;
}

std::vector<std::vector<double>> DataLoader::load_partitioned_wind_series(
    const std::string &base_path, int num_series, int rank, int size)
{
    std::vector<std::vector<double>> partitioned_series;
    for (int i = 1; i <= num_series; ++i)
    {
                std::cout << "Valor serie " << ((i - 1) % (size)) << std::endl;

        if (((i - 1) % (size)) != rank)
            continue; // Solo las series de este proceso
        std::string path = base_path + "/viento_serie_" + std::to_string(i) + ".csv";
        std::cout << "Cargando " << path << std::endl;
        partitioned_series.push_back(load_series(path));
    }
    return partitioned_series;
}

std::vector<std::vector<double>> DataLoader::load_all_wind_series(
    const std::string &base_path, int num_series)
{
    std::vector<std::vector<double>> all_series;
    for (int i = 1; i <= num_series; ++i)
    {
        std::string path = base_path + "/viento_serie_" + std::to_string(i) + ".csv";
        std::cerr << "Cargando " << path << std::endl;
        all_series.push_back(load_series(path));
    }
    return all_series;
}