#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
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
    config.server_port = 8080; // Default
    config.filename = "words.txt"; // Default
    
    std::string line;
    while (std::getline(file, line)) {
        // Simple parsing - remove quotes and extract values
        if (line.find("\"server_ip\"") != std::string::npos) {
            size_t start = line.find("\"", line.find(":")) + 1;
            size_t end = line.find("\"", start);
            config.server_ip = line.substr(start, end - start);
        } else if (line.find("\"server_port\"") != std::string::npos) {
            size_t start = line.find(":") + 1;
            std::string port_str = line.substr(start);
            // Remove whitespace and comma
            port_str.erase(0, port_str.find_first_not_of(" \t"));
            if (port_str.back() == ',') port_str.pop_back();
            config.server_port = std::stoi(port_str);
        } else if (line.find("\"filename\"") != std::string::npos) {
            size_t start = line.find("\"", line.find(":")) + 1;
            size_t end = line.find("\"", start);
            config.filename = line.substr(start, end - start);
        }
    }
    return config;
}

// Function to read words from the file into a vector
std::vector<std::string> read_words_from_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    
    std::vector<std::string> words;
    std::string content;
    std::getline(file, content); // Read entire file content
    
    std::stringstream ss(content);
    std::string word;
    while (std::getline(ss, word, ',')) {
        if (!word.empty()) {
            words.push_back(word);
        }
    }
    file.close();
    return words;
}

int main() {
    // 1. Load configuration
    Config config;
    try {
        config = parse_config("config.json");
    } catch (const std::exception& e) {
        std::cerr << "Error reading config file: " << e.what() << std::endl;
        return 1;
    }

    const int PORT = config.server_port;
    const std::string FILENAME = config.filename;

    std::cout << "Server: Using port " << PORT << std::endl;
    std::cout << "Server: Using file " << FILENAME << std::endl;

    // 2. Load words from file
    std::vector<std::string> words;
    try {
        words = read_words_from_file(FILENAME);
        std::cout << "Server: Loaded " << words.size() << " words from " << FILENAME << std::endl;
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    // 3. Setup TCP socket
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Use only SO_REUSEADDR (SO_REUSEPORT not available on all systems)
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    std::cout << "Server: Attempting to bind to port " << PORT << std::endl;
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server: Successfully listening on port " << PORT << std::endl;

    // 4. Accept connections and handle requests
    while (true) {
        std::cout << "Server: Waiting for connection..." << std::endl;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }
        std::cout << "Server: New connection accepted." << std::endl;

        char buffer[1024] = {0};
        while (true) {
            int valread = read(new_socket, buffer, 1024);
            if (valread <= 0) {
                std::cout << "Server: Client disconnected or read error." << std::endl;
                break; // Client disconnected or error
            }

            std::string req(buffer, valread);
            // Remove trailing newline
            if (!req.empty() && req.back() == '\n') {
                req.pop_back();
            }
            
            std::cout << "Server: Received request: " << req << std::endl;
            
            size_t comma_pos = req.find(',');
            if (comma_pos == std::string::npos) {
                std::cerr << "Server: Invalid request format: " << req << std::endl;
                break;
            }

            int p = std::stoi(req.substr(0, comma_pos));
            int k = std::stoi(req.substr(comma_pos + 1));

            std::cout << "Server: Request p=" << p << ", k=" << k << std::endl;

            std::string response;

            if (p >= (int)words.size()) {
                response = "EOF\n";
            } else {
                std::stringstream response_ss;
                int words_sent = 0;
                
                for (int i = 0; i < k && (p + i) < (int)words.size(); ++i) {
                    if (i > 0) response_ss << ",";
                    response_ss << words[p + i];
                    words_sent++;
                }
                
                // If we sent fewer words than requested, add EOF
                if (words_sent < k) {
                    if (words_sent > 0) response_ss << ",";
                    response_ss << "EOF";
                }
                
                response_ss << "\n";
                response = response_ss.str();
            }

            send(new_socket, response.c_str(), response.length(), 0);
            std::cout << "Server: Sent response: " << response.substr(0, 100) << (response.length() > 100 ? "..." : "") << std::endl;
            
            memset(buffer, 0, sizeof(buffer));
        }

        close(new_socket);
    }

    close(server_fd);
    return 0;
}