#pragma once

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

class Argument {
public:
    virtual int parse(int &i, int argc, char const *argv[]) = 0;

    virtual ~Argument() = default;
};

class ArgumentsParser {
public:
    ArgumentsParser();

    static void throwMissing(std::string_view name, std::string_view key, std::string_view value) {
        std::cout << ("Unrecognized " + std::string(name) + " argument: " + std::string(key) +
                      " with value " + std::string(value) + " parsing failed.")
                  << std::endl;
        std::terminate();
    }

    [[nodiscard]] static std::string getVersion() {
        std::unordered_map<std::string, std::string> months({{"Jan", "01"},
                                                             {"Feb", "02"},
                                                             {"Mar", "03"},
                                                             {"Apr", "04"},
                                                             {"May", "05"},
                                                             {"Jun", "06"},
                                                             {"Jul", "07"},
                                                             {"Aug", "08"},
                                                             {"Sep", "09"},
                                                             {"Oct", "10"},
                                                             {"Nov", "11"},
                                                             {"Dec", "12"}});

        std::string month, day, year;
        std::stringstream ss, date(__DATE__);  // {month} {date} {year}

        const std::string version = "dev";

        ss << "Smallbrain " << version;
        ss << "-";
#ifdef GIT_DATE
        ss << GIT_DATE;
#else

        date >> month >> day >> year;
        if (day.length() == 1) day = "0" + day;
        ss << year.substr(2) << months[month] << day;
#endif

#ifdef GIT_SHA
        ss << "-" << GIT_SHA;
#endif
        return ss.str();
    }

    [[nodiscard]] int parse(int argc, char const *argv[]) {
        int status = 0;
        for (int i = 1; i < argc; i++) {
            const std::string arg = argv[i];
            if (arguments_.count(arg) > 0) {
                status += arguments_[arg]->parse(i, argc, argv);
            } else {
                std::cout << ("Unrecognized argument: " + arg + " parsing failed.") << std::endl;
                std::terminate();
            }
        }
        return status;
    }

private:
    void addArgument(const std::string &argument_name, Argument *argument) {
        arguments_.insert(std::make_pair(argument_name, std::unique_ptr<Argument>(argument)));
    }

    std::map<std::string, std::unique_ptr<Argument>> arguments_;
};

// Generic function to parse a standalone value after a dash command.
template<typename T>
void parseValue(int &i, int argc, const char *argv[], T &argument_value) {
    i++;
    if (i < argc && argv[i][0] != '-') {
        if constexpr (std::is_same_v<T, int>)
            argument_value = std::stoi(argv[i]);
        else if constexpr (std::is_same_v<T, uint32_t>)
            argument_value = std::stoul(argv[i]);
        else if constexpr (std::is_same_v<T, float>)
            argument_value = std::stof(argv[i]);
        else if constexpr (std::is_same_v<T, double>)
            argument_value = std::stod(argv[i]);
        else if constexpr (std::is_same_v<T, bool>)
            argument_value = std::string(argv[i]) == "true";
        else
            argument_value = argv[i];
    }
}
