#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <sstream>
#include <unordered_map>

void parse_dump(const std::string& input_filename) {
    const std::string output_filename = "symbols_from_dump.txt";
    std::ifstream infile(input_filename);
    std::ofstream outfile(output_filename, std::ios::out | std::ios::trunc);

    if (!infile) {
        std::cerr << "Error opening input file: " << input_filename << "\n";
        return;
    }
    if (!outfile) {
        std::cerr << "Error opening output file: " << output_filename << "\n";
        return;
    }

    std::stringstream buffer;
    buffer << infile.rdbuf();
    std::string content = buffer.str();

    const std::regex pattern(R"(^(.{17})(.*?)(\s*(= <no type information>))?$)");
    std::smatch match;
    std::string result;

    auto search_start = content.cbegin();
    while (std::regex_search(search_start, content.cend(), match, pattern)) {
        result += "dd" + match[2].str() + " l1\n";
        search_start = match.suffix().first;
    }

    outfile << result;
    std::cout << "Commands have been written to " << output_filename << "\n";
}


void parse_symbols(const std::string& input_filename) {
    const std::string output_filename = "parsed_symbols_with_values.txt";
    std::ifstream infile(input_filename);
    std::ofstream outfile(output_filename, std::ios::out | std::ios::trunc);

    if (!infile) {
        std::cerr << "Error opening input file: " << input_filename << "\n";
        return;
    }
    if (!outfile) {
        std::cerr << "Error opening output file: " << output_filename << "\n";
        return;
    }

    std::string line, next_line;

    while (std::getline(infile, line)) {
        line.erase(0, line.find_first_not_of(" \t"));

        if (line.rfind("Matched:", 0) == 0) {
            continue;
        }


        if (line.rfind("lkd> dd ", 0) == 0 && line.find('!') != std::string::npos && std::getline(infile, next_line)) {
            next_line.erase(0, next_line.find_first_not_of(" \t"));

            if (next_line.find("Couldn't resolve error") != std::string::npos) {
                continue;
            }

            size_t exclamation_pos = line.find('!');
            if (exclamation_pos != std::string::npos) {
                std::string symbol_name = line.substr(exclamation_pos + 1);
                symbol_name.erase(0, symbol_name.find_first_not_of(" \t"));


                if (symbol_name.size() >= 2 && symbol_name.compare(symbol_name.size() - 2, 2, "l1") == 0) {
                    symbol_name.resize(symbol_name.size() - 2);
                }

                symbol_name = std::regex_replace(symbol_name, std::regex(R"(\s*\(.*\))"), "");

                symbol_name.erase(symbol_name.find_last_not_of(" \t") + 1);
                std::string hex_value = next_line.substr(next_line.find_last_of(" ") + 1);

                outfile << symbol_name << " " << hex_value << "\n";
            }
        }
    }

    std::cout << "Symbols have been written to " << output_filename << "\n";
}


void compareFiles(const std::string& file1, const std::string& file2) {
    std::ifstream inputFile1(file1);
    std::ifstream inputFile2(file2);
    std::ofstream outputFile("comparison_result.txt");

    if (!inputFile1.is_open() || !inputFile2.is_open() || !outputFile.is_open()) {
        std::cerr << "Error opening one or more files!" << std::endl;
        return;
    }

    std::unordered_map<std::string, std::string> file1Data;
    std::unordered_map<std::string, std::string> file2Data;

    std::string line1, line2;

    while (std::getline(inputFile1, line1)) {
        std::stringstream ss1(line1);
        std::string symbol1, value1;

        if (!(ss1 >> symbol1 >> value1)) {
            continue;
        }

        file1Data[symbol1] = value1;
    }

    while (std::getline(inputFile2, line2)) {
        std::stringstream ss2(line2);
        std::string symbol2, value2;

        if (!(ss2 >> symbol2 >> value2)) {
            continue;
        }

        file2Data[symbol2] = value2;
    }

    for (const auto& entry : file1Data) {
        const std::string& symbol1 = entry.first;
        const std::string& value1 = entry.second;

        auto it = file2Data.find(symbol1);
        if (it != file2Data.end()) {
            const std::string& value2 = it->second;
            if (value1 != value2) {
                outputFile << "Symbol: " << symbol1
                    << " | " << file1 << " Value: " << value1
                    << " | " << file2 << " Value: " << value2 << std::endl;
            }
        }
        else {
            outputFile << "Symbol: " << symbol1
                << " is missing in " << file2 << std::endl;
        }
    }

    for (const auto& entry : file2Data) {
        const std::string& symbol2 = entry.first;
        if (file1Data.find(symbol2) == file1Data.end()) {
            outputFile << "Symbol: " << symbol2
                << " is missing in " << file1 << std::endl;
        }
    }
    std::cout << "Symbols have been written to comparison_result.txt " << "\n";
    inputFile1.close();
    inputFile2.close();
    outputFile.close();
}



int main(int argc, char* argv[]) {
    if (argc < 3 || (argc < 4 && std::string(argv[1]) == "--compare-symbols")) {
        std::cerr << "Usage: WinDbgParser.exe --parse-dump <input_filename>\n";
        std::cerr << "       WinDbgParser.exe --parse-symbols <input_filename>\n";
        std::cerr << "       WinDbgParser.exe --compare-symbols <input_filename_1> <input_filename_2>\n";
        return 1;
    }

    std::string option = argv[1];

    if (option == "--parse-dump") {
        std::string input_filename = argv[2];
        parse_dump(input_filename);
    }
    else if (option == "--parse-symbols") {
        std::string input_filename = argv[2];
        parse_symbols(input_filename);
    }
    else if (option == "--compare-symbols") {
        if (argc != 4) {
            std::cerr << "Usage: WinDbgParser.exe --compare-symbols <input_filename_1> <input_filename_2>\n";
            return 1;
        }
        std::string input_filename1 = argv[2];
        std::string input_filename2 = argv[3];
        compareFiles(input_filename1, input_filename2);
    }
    else {
        std::cerr << "Unknown option: " << option << "\n";
        return 1;
    }

    return 0;
}

