#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <csignal>

#ifdef __WIN32
    // Me when the windows is sus
#else
    #include <unistd.h>
#endif

const std::string DEBUG_FILE = "bf.debug";
const std::string KEY_CHARACTERS = "><+-.,[]";
const int ARRAY_POSITIONS = 30000;

bool debug = false;

volatile sig_atomic_t keep_running = 1;

void sigint_handler(int signum) {
    keep_running = 0;
    // disable the unused variable warning
    (void)signum;
}

void add_sigint_handler() {
#ifdef __WIN32
    std::signal(SIGINT, sigint_handler);
#else
    struct sigaction action;
    action.sa_handler = sigint_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);
#endif
}
std::string char_to_hexadecimal_str(char c) {
    static const char hexChars[] = "0123456789ABCDEF";
    unsigned char byte = static_cast<unsigned char>(c);
    std::string hexa_str;
    hexa_str += hexChars[byte >> 4];
    hexa_str += hexChars[byte & 0x0F];
    return hexa_str;
}

void write_to_debug(std::vector<char> &array) {

    std::ofstream debug_file(DEBUG_FILE);
    std::ostringstream memory_buffer;

    for (char &c : array) {
        memory_buffer << char_to_hexadecimal_str(c) << " ";
    }

    debug_file << memory_buffer.str();

    debug_file.close();
}

int skip_loop(std::string &brainfuck_code, int starting_pos) {
    std::stack<char> brackets;
    for (size_t i = starting_pos; i < brainfuck_code.size(); i++) {
        if (brainfuck_code[i] == ']') {
            brackets.pop();
            if (brackets.empty()) return i;
        } else if (brainfuck_code[i] == '[') {
            brackets.push('[');
        }
    }
    return starting_pos;
}

int reset_loop(std::string &brainfuck_code, int starting_pos) {
    std::stack<char> brackets;
    for (int i = starting_pos; i >= 0; i--) {
        if (brainfuck_code[i] == '[') {
            brackets.pop();
            if (brackets.empty()) return --i;
        } else if (brainfuck_code[i] == ']') {
            brackets.push(']');
        }
    }
    return starting_pos;
}

int run_brainfuck(
    std::string &brainfuck_code,
    std::vector<char> &array,
    int array_pos
) {

    for (size_t i = 0; i < brainfuck_code.size(); i++) {
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
    std::vector<char> memories(ARRAY_POSITIONS, 0);
    run_brainfuck(brainfuck_code, memories, 0);
    if (debug) write_to_debug(memories);
}

void run_brainfuck_repl() {
    if (debug) std::cout << "please use Ctrl+C to exit.\nEnter help to get help (debug mode only)";
    int array_pos = 0;
    std::vector<char> memories(ARRAY_POSITIONS, 0);
    std::string input;
    while (keep_running) {
        if (debug) std::cout << "\n> " << std::flush;
        if (!std::getline(std::cin, input)) {
            if (!keep_running) {
                break;
            }
            break;
        }
        if (debug) {
            if (input == "clear") {
                std::cout << "\x1b[2J\x1b[3J\x1b[H" << std::flush;
                continue;
            } else if (input == "help") {
                std::cout << "Brainfuck REPL debug mode help options:" << std::endl;
                std::cout << "  help: Print this nice help message" << std::endl;
                std::cout << "  clear: Clear's the terminal output" << std::endl;
                std::cout << "  Ctrl+C: exit the repl" << std::endl;
                std::cout << "Note that these options (with the except of Ctrl+C) only work in the repl when debug mode is on" << std::endl;
                continue;
            }
        }
        array_pos = run_brainfuck(input, memories, array_pos);
        if (debug) {
            write_to_debug(memories);
            std::cout << "\nCursor at: [ " << array_pos << " ]," << " [ Numeric | Hexadecimal ] value at position: [ " << static_cast<int>(memories[array_pos]) << " | " << char_to_hexadecimal_str(memories[array_pos]) << " ]";
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
    add_sigint_handler();



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

    if (cleaned_arguments.empty()) {
        std::cout << "Not enough arguments. Use -h for help" << std::endl;
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
