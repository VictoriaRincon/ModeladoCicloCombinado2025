#ifndef DATA_LOADER_HPP
#define DATA_LOADER_HPP

#include <string>
#include <vector>

class DataLoader {
public:
    static std::vector<double> load_series(const std::string& path);
    static std::vector<std::vector<double>> load_partitioned_wind_series(
        const std::string& base_path, int num_series, int rank, int size);
    static std::vector<std::vector<double>> load_all_wind_series(
        const std::string& base_path, int num_series);
};

#endif // DATA_LOADER_HPP
    