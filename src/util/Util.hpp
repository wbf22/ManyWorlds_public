
#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <type_traits>
#include <iomanip>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <cstdlib>

#if defined(__GLIBC__)
    #include <malloc.h>
#endif


using namespace std;

struct Util {

    // 9,223,372,036,854,775,808
    // 8,301,034,833,169,298,226
    inline static int64_t MAX_INT64_T = std::numeric_limits<int64_t>().max();
    inline static int64_t MIN_INT64_T = std::numeric_limits<int64_t>().min();
    // 1.7976931348623157 × 10^308
    inline static double MAX_DOUBLE = std::numeric_limits<double>().max();


    // Reset / attributes
    inline static const string RESET     = "\x1b[0m";
    inline static const string BOLD      = "\x1b[1m";
    inline static const string UNDERLINE = "\x1b[4m";
    inline static const string REVERSED  = "\x1b[7m";

    // Foreground (24-bit / truecolor)
    inline static const string RED       = "\x1b[38;2;255;0;0m";
    inline static const string GREEN     = "\x1b[38;2;0;255;0m";
    inline static const string BLUE      = "\x1b[38;2;0;0;255m";
    inline static const string YELLOW    = "\x1b[38;2;255;255;0m";
    inline static const string MAGENTA   = "\x1b[38;2;255;0;255m";
    inline static const string CYAN      = "\x1b[38;2;0;255;255m";
    inline static const string ORANGE    = "\x1b[38;2;255;165;0m";
    inline static const string PURPLE    = "\x1b[38;2;128;0;128m";
    inline static const string PINK      = "\x1b[38;2;255;192;203m";
    inline static const string LIGHT_GRAY= "\x1b[38;2;192;192;192m";
    inline static const string DARK_GRAY = "\x1b[38;2;64;64;64m";

    // Backgrounds (24-bit / truecolor)
    inline static const string RED_BG       = "\x1b[48;2;255;0;0m";
    inline static const string GREEN_BG     = "\x1b[48;2;0;255;0m";
    inline static const string BLUE_BG      = "\x1b[48;2;0;0;255m";
    inline static const string YELLOW_BG    = "\x1b[48;2;255;255;0m";
    inline static const string MAGENTA_BG   = "\x1b[48;2;255;0;255m";
    inline static const string CYAN_BG      = "\x1b[48;2;0;255;255m";
    inline static const string ORANGE_BG    = "\x1b[48;2;255;165;0m";
    inline static const string PURPLE_BG    = "\x1b[48;2;128;0;128m";
    inline static const string PINK_BG      = "\x1b[48;2;255;192;203m";
    inline static const string LIGHT_GRAY_BG= "\x1b[48;2;192;192;192m";
    inline static const string DARK_GRAY_BG = "\x1b[48;2;64;64;64m";


    /**
     * Returns seconds since the epoch
     */
	static double time() {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        return microseconds / 1e6;  // convert microseconds to seconds
    }


    static void writeToFile(string filePath, string contents) {
        std::ofstream outputFile(filePath);
    
        // Check if the file is successfully opened
        if (outputFile.is_open()) {
            // Write data to the file
            outputFile << contents;
    
            outputFile.close();
        }
        else {
            std::cerr << "Error opening the file." << std::endl;
        }
    
    }

    static string read_file(string filePath) {
        ifstream file(filePath);
        ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
	
    template<typename... Ts>
    static void print(const Ts&... args) {
        ((cout << args << " "), ...);
        cout << endl;
    }


    template<typename T>
    static void print_vector_copiable(vector<T>& vec) {

        cout << "vector<T> test = {" << endl;
        for (int i = 0; i < vec.size(); i++) {
            cout << "\t" << vec[i] << "," << endl;
        }
        cout << "};" << endl << endl;
    }



    template<typename T>
    static void print_vector_pairs_copiable(vector<pair<T, T>>& vec) {

        cout << "vector<pair<T, T>> test = {" << endl;
        for (int i = 0; i < vec.size(); i++) {
            cout << "\t{" << vec[i].first << ", " << vec[i].second << "}," << endl;
        }
        cout << "};" << endl << endl;
    }



    template<typename T>
    static void vector_to_csv(vector<vector<T>>& vec) {

        for (int i = 0; i < vec.size(); i++) {
            vector<T> inner_vec = vec[i];
            for (int j = 0; j < inner_vec.size() - 1; ++j) {
                cout << inner_vec[j] << ", ";
            }
            cout << inner_vec[inner_vec.size() - 1] << endl;
        }
        cout << endl << endl;
    }


    template<typename T>
    static string print_map_in_columns(const vector<pair<string, T>>& map) {

        stringstream ss;
        int max_key_length = 0;
        for (const pair<string, T>& entry : map) {
            if (entry.first.length() > max_key_length) {
                max_key_length = entry.first.length();
            }
        }
        max_key_length += 6;

        for (const pair<string, T>& entry : map) {
            ss << entry.first;
            int key_length = entry.first.length();
            for (int i = 0; i < max_key_length - key_length; ++i) {
                ss << "_";
            }
            ss << entry.second << endl;
        }
        ss << endl;

        return ss.str();
    }

    template<typename T>
    static string to_string(T& value) {
        if (std::is_same<T, bool>::value) {
            if (value) {
                return "true";
            }
            else {
                return "false";
            }
        } else {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << value;
            return oss.str();
        }
    }

    
    template<typename T>
    static void vector_pairs_to_csv(vector<pair<T, T>>& vec) {

        for (int i = 0; i < vec.size(); i++) {
            cout << vec[i].first << ", " << vec[i].second << endl;
        }
        cout << endl << endl;
    }
    

	/**
	 * Gets a random seed using the current time
	 */
	static int64_t seed() {
		return static_cast<int64_t>(Util::time());
	}



	static double ceil_to_precision(double value, double precision) {
		if (precision == 0) precision = 1;
		
		return std::ceil(value / precision) * precision;
	}

	static double floor_to_precision(double value, double precision) {
		if (precision == 0) precision = 1;
		
		return std::floor(value / precision) * precision;
	}


	static double round_to_precision(double value, double precision) {
		if (precision == 0) precision = 1;
		
		return std::round(value / precision) * precision;
	}

	static double truncate_to_precision(double value, double precision) {
		if (precision == 0) precision = 1;

        if (value > 0) {
		    return std::floor(value / precision) * precision;
        }
        else {
		    return std::ceil(value / precision) * precision;
        }
	}

    static double round_if_almost(double value, double precision) {
        double rounded = Util::round_to_precision(value, precision);
        if (abs(rounded - value) <= 0.001) {
            return rounded;
        }
        else {
            return value;
        }
    }

    // Recursive case: compare first with max of the rest
    template<typename... Args>
    static double max(double a, Args... args) {
        return std::max({a, static_cast<double>(args)...});
    }

    
    template<typename... Args>
    static double min(double first, Args... args) {
        return std::min({first, static_cast<double>(args)...});
    }

    template<typename T>
    static void swap_pop_remove(vector<T> &vec, T value) {
        auto it = std::find(vec.begin(), vec.end(), value);
        if (it != vec.end()) {
            int index = std::distance(vec.begin(), it);
            vec[index] = vec.back();
            vec.pop_back();
        }
    }

    template<typename T>
    static void swap_pop_remove_at(vector<T> &vec, int index) {
        vec[index] = vec.back();
        vec.pop_back();
    }

    template<typename T>
    static bool contains(vector<T> &vec, T value) {
        return std::find(vec.begin(), vec.end(), value) != vec.end();
    }

    static double round_to_n(double value, int n) {
        double factor = std::pow(10.0, n);
        return std::round(value * factor) / factor;
    }


    /**
     * Returns euclidean distance
     */
    static int64_t dist(int64_t x, int64_t y, int64_t z, int64_t x2, int64_t y2, int64_t z2) {
        return std::sqrt(std::pow(x - x2, 2) + std::pow(y - y2, 2) + std::pow(z - z2, 2));
    }

    static int64_t manhatten_dist(int64_t x, int64_t y, int64_t z, int64_t x2, int64_t y2, int64_t z2) {
        return Util::max(abs(x-x2), abs(y-y2), abs(z-z2));
    }

    static double manhatten_dist_d(double x, double y, double z, double x2, double y2, double z2) {
        return Util::max(abs(x-x2), abs(y-y2), abs(z-z2));
    }

    static double no_zero(double value) {
        if (value == 0) return 0.0000001;

        return value;
    }


    static double piecewise(
        double x,
        vector<pair<pair<double, double>, function<double(double)>>> pieces
    )
    {
        for (auto& [range, func] : pieces) {
            auto& [low, high] = range;
            if (x >= low && x < high) {
                return func(x);
            }
        }
        cout << "Value out of range for piecewise function. Using first function as a default" << endl;
        return pieces.begin()->second(x);
    }

    static double clamp(double min, double max, double value) {
        if (value < min) {
            value = min;
        }

        if (value > max) {
            value = max;
        }

        return value;
    }

    static double clamp(double max, double value) {
        
        if (value > max) {
            value = max;
        }

        return value;
    }

    static double weighted_average(vector<double> values, vector<double> weights) {
        double sum = 0;
        double sum_weights = 0;
        for (int i = 0; i < values.size(); ++i) {
            sum += values[i] * weights[i];
            sum_weights += weights[i];
        }
        return sum / sum_weights;
    }
    
    static double interpolate(vector<double> values, vector<double> weights, double total_weight) {
        double sum = 0;
        for (int i = 0; i < values.size(); ++i) {
            sum += values[i] * weights[i];
        }
        return sum / total_weight;
    }


    // for converting strings to c strings in 'format()' below
    template<typename T>
    static decltype(auto) convert_arg(T&& value) {
        using Decayed = std::decay_t<T>;

        if constexpr (std::is_same_v<Decayed, std::string>) {
            return value.c_str();
        } else {
            return std::forward<T>(value);
        }
    }

    /**
     * Formats a string c style
     */
    template <typename... Args>
    static string format(string fmt, Args&&... args) {

        // Figure out the required buffer size
        int size = std::snprintf(
            nullptr,
            0,
            fmt.c_str(),
            convert_arg(std::forward<Args>(args))...
        ) + 1;

        if (size <= 0) {
            throw std::runtime_error("Formatting error");
        }

        std::string buf(size, '\0');

        std::snprintf(
            buf.data(),
            size,
            fmt.c_str(),
            convert_arg(std::forward<Args>(args))...
        );

        buf.pop_back(); // remove null terminator

        return buf;
    }

    template<typename T, typename... Vecs>
    static vector<T> combine_vectors(const vector<T>& first, const Vecs&... rest) {
        vector<T> result;
        
        // Reserve space upfront for efficiency
        size_t total = first.size() + (rest.size() + ...);
        result.reserve(total);
        
        // Insert all vectors
        result.insert(result.end(), first.begin(), first.end());
        (result.insert(result.end(), rest.begin(), rest.end()), ...);
        
        return result;
    }

    template<typename T, typename... Vecs>
    static void extend(vector<T>& vec, Vecs&... rest) {
        (vec.insert(vec.end(), rest.begin(), rest.end()), ...);
    }

    template<typename T>
    static vector<T> copy(const vector<T>& vec) {
        vector<T> result;
        
        // Reserve space upfront for efficiency
        result.reserve(vec.size());
        
        // Insert all elements
        result.insert(result.end(), vec.begin(), vec.end());
        
        return result;
    }


    static bool equals(double a, double b, double eps = 1e-12) {
        return std::fabs(a - b) <= eps;
    }

    
};

struct Timer {
    inline static double start_time;

    static void start() {
        Timer::start_time = Util::time();
    }

    static void stop(string label) {
        cout << label << ": " << Util::time() - Timer::start_time << "s" << endl;
    }
};


/**
 * General Buget guidlines:
 * 
 * + low (8gb) - 4gb 
 * + mainstream (16gb) - 8gb
 * + high (32gb) - 16gb
 * 
 * You can push like to 60% of available ram 
 */
struct MemInfo {

    #if defined(__GLIBC__)
        inline static struct mallinfo2 game_start_mem = mallinfo2();
        inline static struct mallinfo2 start_mem = {};
    #endif

    static void start() {
        #if defined(__GLIBC__)
            MemInfo::start_mem = mallinfo2();
        #endif
    }

    static void stop(string label) {
        #if defined(__GLIBC__)
            auto after = mallinfo2();
            int64_t delta = (after.uordblks - start_mem.uordblks) +
                        (after.hblkhd   - start_mem.hblkhd);
            double mb = double(delta) / (1024.0 * 1024.0);
            double gb = double(delta) / (1024.0 * 1024.0 * 1024.0);
            cout << label << ": " << mb << " MB (" << gb << " GB)" << endl;
        #else
            cout << label << ": Memory info not available on this platform." << endl;
        #endif
    }

    static int64_t mb_allocated_since_start() {
        #if defined(__GLIBC__)
            auto current = mallinfo2();
            return (current.uordblks - game_start_mem.uordblks) + (current.hblkhd - game_start_mem.hblkhd);
        #else
            return 0;
        #endif
    }

    static double system_ram_percent_used() {
        ifstream meminfo("/proc/meminfo");
        string line;
        double total = 0, available = 0;
        while (getline(meminfo, line)) {
            size_t colon = line.find(':');
            if (colon == string::npos) continue;
            string val_str = line.substr(colon + 1);
            double val = stod(val_str);
            if (line.rfind("MemTotal:", 0) == 0)
                total = val;
            else if (line.rfind("MemAvailable:", 0) == 0)
                available = val;
        }
        if (total == 0) return -1;
        return 100.0 * (1.0 - available / total);
    }

};

