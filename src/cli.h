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
    virtual void parse(int &i, int argc, char const *argv[]) = 0;

    virtual ~Argument() = default;
};

class ArgumentsParser {
   public:
    ArgumentsParser(int argc, char const *argv[]);

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

   private:
    void addArgument(std::string argumentName, Argument *argument) {
        arguments_.insert(std::make_pair(argumentName, std::unique_ptr<Argument>(argument)));
    }

    void parse(int argc, char const *argv[]) {
        for (int i = 1; i < argc; i++) {
            const std::string arg = argv[i];
            if (arguments_.count(arg) > 0) {
                arguments_[arg]->parse(i, argc, argv);
            } else {
                std::cout << ("Unrecognized argument: " + arg + " parsing failed.") << std::endl;
                std::terminate();
            }
        }
    }

    std::map<std::string, std::unique_ptr<Argument>> arguments_;
};

// Generic function to parse a standalone value after a dash command.
template <typename T>
void parseValue(int &i, int argc, const char *argv[], T &argumentValue) {
    i++;
    if (i < argc && argv[i][0] != '-') {
        if constexpr (std::is_same_v<T, int>)
            argumentValue = std::stoi(argv[i]);
        else if constexpr (std::is_same_v<T, uint32_t>)
            argumentValue = std::stoul(argv[i]);
        else if constexpr (std::is_same_v<T, float>)
            argumentValue = std::stof(argv[i]);
        else if constexpr (std::is_same_v<T, double>)
            argumentValue = std::stod(argv[i]);
        else if constexpr (std::is_same_v<T, bool>)
            argumentValue = std::string(argv[i]) == "true";
        else
            argumentValue = argv[i];
    }
}

inline void parseDashArguments(int &i, int argc, char const *argv[],
                               std::function<void(std::string, std::string)> func) {
    while (i + 1 < argc && argv[i + 1][0] != '-' && i++) {
        std::string param = argv[i];
        std::size_t pos = param.find('=');
        std::string key = param.substr(0, pos);
        std::string value = param.substr(pos + 1);

        char quote = value[0];

        if (quote == '"' || quote == '\'') {
            while (i + 1 < argc && argv[i + 1][0] != quote) {
                i++;
                value += " " + std::string(argv[i]);
            }
        }

        func(key, value);
    }
}