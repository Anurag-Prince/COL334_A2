#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <chrono>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

// Simple JSON parser for config (avoiding external dependency)
struct Config {
    std::string server_ip;
    int server_port;
    int k;
    int p;
    std::string filename;
    int num_repetitions;
};

Config parse_config(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + filename);
    }
    
    Config config;
    std::string line;
    while (std::getline(file, line)) {
        // Simple parsing - it remove quotes and extract values
        if (line.find("\"server_ip\"") != std::string::npos) {
            size_t start = line.find("\"", line.find(":")) + 1;
            size_t end = line.find("\"", start);
            config.server_ip = line.substr(start, end - start);
        } else if (line.find("\"server_port\"") != std::string::npos) {
            size_t start = line.find(":") + 1;
            config.server_port = std::stoi(line.substr(start));
        } else if (line.find("\"k\"") != std::string::npos) {
            size_t start = line.find(":") + 1;
            config.k = std::stoi(line.substr(start));
        } else if (line.find("\"p\"") != std::string::npos) {
            size_t start = line.find(":") + 1;
            config.p = std::stoi(line.substr(start));
        }
    }
    return config;
}

//  build a Helper to split string by delimiter
void split(const std::string& s, char delimiter, std::vector<std::string>& tokens) {
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
}

int main(int argc, char const *argv[]) {
    // 1. Load configuration
    Config config;
    try {
        config = parse_config("config.json");
    } catch (const std::exception& e) {
        std::cerr << "Error reading config file: " << e.what() << std::endl;
        return 1;
    }

    const std::string SERVER_IP = config.server_ip;
    const int PORT = config.server_port;
    
    // Allow overriding k and p from command line for analysis
    int k = (argc > 1) ? std::stoi(argv[1]) : config.k;
    int p_initial = (argc > 2) ? std::stoi(argv[2]) : config.p;

    // 2. Setup TCP socket
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "\n Socket creation error \n";
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "\nInvalid address/ Address not supported \n";
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "\nConnection Failed \n";
        return -1;
    }
    
    // 3. Download file and measure time
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::string> all_words;
    int current_p = p_initial;
    bool eof_received = false;

    while (!eof_received) {
        std::string request = std::to_string(current_p) + "," + std::to_string(k) + "\n";
        send(sock, request.c_str(), request.length(), 0);

        char buffer[2048] = {0};
        int valread = read(sock, buffer, 2048);
        
        if (valread <= 0) {
            break; // Server closed connection or error when it doesnot 
        }

        std::string response(buffer, valread);
        
        // Remove trailing newline for processing
        if (!response.empty() && response.back() == '\n') {
            response.pop_back();
        }

        // Check for EOF when too much k
        if (response == "EOF") {
            eof_received = true;
            break;
        } else if (response.find(",EOF") != std::string::npos) {
            eof_received = true;
            // Extract words before EOF
            size_t eof_pos = response.find(",EOF");
            response = response.substr(0, eof_pos);
        }
        
        if (!response.empty()) {
            split(response, ',', all_words);
        }

        current_p += k;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> completion_time = end - start;
    
    close(sock);

    // 4. Count word frequencies
    std::map<std::string, int> freq_map;
    for (const auto& word : all_words) {
        if (!word.empty()) {
            freq_map[word]++;
        }
    }

    // 5. Print results
    std::cout << "--- Word Frequencies ---" << std::endl;
    for (const auto& pair : freq_map) {
        std::cout << pair.first << ", " << pair.second << std::endl;
    }
    std::cout << "------------------------" << std::endl;
    std::cout << "Completion Time: " << completion_time.count() << " ms" << std::endl;

    return 0;
}