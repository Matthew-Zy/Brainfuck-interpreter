#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <csignal>
#include <atomic>

const std::string DEBUG_FILE = "bf.debug";
const std::string KEY_CHARACTERS = "><+-.,[]";
bool debug = false;

std::atomic<bool> keep_running = true;

void sigint_handler(int signum) {
    keep_running = false; 
    if (debug) std::cout << "Received control + c (" << signum << ")" << std::endl;
}

void write_to_debug(std::vector<char> &array) {
    std::ofstream debug_file(DEBUG_FILE);
    std::ostringstream memory_buffer;

    for (char &c : array) {
        memory_buffer << static_cast<int>(c) << " ";
    }

    debug_file << memory_buffer.str();

    debug_file.close();
}

unsigned int skip_loop(std::string &brainfuck_code, unsigned int starting_pos) {
    std::stack<char> brackets;
    for (unsigned int i = starting_pos; i < brainfuck_code.size(); i++) {
        if (brainfuck_code[i] == ']') {
            brackets.pop();
            if (brackets.empty()) return i;
        } else if (brainfuck_code[i] == '[') {
            brackets.push('[');
        }
    }
    return starting_pos;
}

unsigned int reset_loop(std::string &brainfuck_code, unsigned int starting_pos) {
    std::stack<char> brackets;
    for (unsigned int i = starting_pos; i > 0; i--) {
        if (brainfuck_code[i] == '[') {
            brackets.pop();
            if (brackets.empty()) return --i;
        } else if (brainfuck_code[i] == ']') {
            brackets.push(']');
        }
    }
    return starting_pos;
}

unsigned int run_brainfuck(
    std::string &brainfuck_code, 
    std::vector<char> &array,
    unsigned int array_pos
) {

    for (unsigned int i = 0; i < brainfuck_code.size(); i++) {
        if (!keep_running) {
            break;
        }
        char token = brainfuck_code[i];
        switch (token) {
            case '>':
                array_pos++;
                break; 
            case '<':
                array_pos--;
                break;
            case '+':
                array[array_pos]++;
                break;
            case '-':
                array[array_pos]--;
                break;
            case '.':
                std::cout << array[array_pos];
                break;
            case ',':
                array[array_pos] = []() -> char {
                    std::string input; 
                    std::getline(std::cin, input);
                    return input.empty() ? 0 : input[0];
                }();
                break;
            case '[':
                if (array[array_pos] == 0) {
                    i = skip_loop(brainfuck_code, i);
                }
                break;
            case ']':
                i = reset_loop(brainfuck_code, i);
                break;
            default:
                break;
        }
    }
    return array_pos;
}

std::string remove_comments(std::string &brainfuck_code) {
    auto contains = [](const std::string &str, char lookup) -> bool {
        for (char c : str) {
            if (c == lookup) return true;
        }
        return false;
    };

    std::string cleaned;
    for (char c : brainfuck_code) {
        if (contains(KEY_CHARACTERS, c)) {
            cleaned += c;
        }
    }
    return cleaned;
}

void run_brainfuck(std::string &brainfuck_code) {
    brainfuck_code = remove_comments(brainfuck_code);
    std::vector<char> memories(30000, 0);
    run_brainfuck(brainfuck_code, memories, 0);
    if (debug) write_to_debug(memories);
}

void run_brainfuck_repl() {
    if (debug) std::cout << "please use control + c to exit, enter clear to clear the terminal";
    unsigned int array_pos = 0;
    std::vector<char> memories(30000, 0);
    std::string input;
    while (keep_running) {
        if (debug) std::cout << "\n> " << std::flush;
        std::getline(std::cin, input);
        if (std::cin.eof()) {
            break;
        } else if (input == "clear") {
            std::cout << "\x1b[2J\x1b[3J\x1b[H" << std::flush;
            continue;
        }
        array_pos = run_brainfuck(input, memories, array_pos);
        if (debug) {
            write_to_debug(memories);
            std::cout << "\nCursor at: [" << array_pos << "]" << " Numeric value at position: [" << static_cast<int>(memories[array_pos]) << "]";
        }
        
        input.clear();
    }
    
}

std::string loadFileToString(const std::string& filename) {
    std::ifstream fileStream(filename);
    
    if (!fileStream.is_open()) {
        throw std::runtime_error("Error: The file '" + filename + "' could not be found or opened.");
    }
    
    std::stringstream buffer;
    buffer << fileStream.rdbuf();
    
    return buffer.str();
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, sigint_handler);

    if (argc == 1) {
        std::cout << "Not enough arguments. Use -h for help" << std::endl;
        return 0;
    }


    bool inplace = false;
    bool use_repl = false;
    
    std::vector<std::string> cleaned_arguments;
    for (int i = 1; i<argc; i++) {
        std::string arg = std::string(argv[i]);
        if (arg == "-h") {
            std::cout << "Usage: " << argv[0] << " brainfuck_file brainfuck_file2" << std::endl;
            std::cout << "Options: -i, interpret the rest of the arguments directly as Brainfuck code" << std::endl;
            std::cout << "Option: -repl, use the brainfuck repl" << std::endl;
            std::cout << "Option: -debug, use this in the repl mode to print each cycle to a debug file and other useful information" << std::endl;
            std::cout << "                use this in normal mode to print a dump of the entire array to a file with their numeric value at the end of brainfuck exe" << std::endl;
            return 0;
        } else if (arg == "-i") {
            inplace = true;
        } else if (arg == "-repl") {
            use_repl = true;
        } else if (arg == "-debug") {
            debug = true;
        } else {
            cleaned_arguments.push_back(arg);
        }
    }


    if (use_repl == true) {
        run_brainfuck_repl();
        return 0;
    } 

    std::string brainfuck_code;
    for (std::string &s : cleaned_arguments) {
        if (inplace == true) {
            brainfuck_code += s;
        } else {
            try {
                brainfuck_code += loadFileToString(s);
            } catch (const std::runtime_error& error) {
                std::cerr << "File system error: " << error.what() << "\n";
                return 1;
            }
        }
        
    }


    run_brainfuck(brainfuck_code);
}