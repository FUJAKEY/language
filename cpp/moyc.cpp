#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

static std::string escape(const std::string &s) {
    std::string out;
    for(char c: s){
        if(c=='"' || c=='\\') out += '\\';
        out += c;
    }
    return out;
}

int main(int argc, char** argv){
    if(argc<2){
        std::cerr << "Usage: moyc <input.moy> [output.s]" << std::endl;
        return 1;
    }
    std::string inPath = argv[1];
    std::string outPath = argc>=3 ? argv[2] : "a.s";

    std::ifstream in(inPath);
    if(!in){
        std::cerr << "Cannot open " << inPath << std::endl;
        return 1;
    }

    std::vector<std::string> data;
    std::vector<std::string> text;
    std::string line;
    int msgId = 0;
    while(std::getline(in, line)){
        std::string trimmed;
        size_t start = line.find_first_not_of(" \t");
        if(start!=std::string::npos) trimmed = line.substr(start);
        if(trimmed.empty() || trimmed[0]=='#') continue;
        if(trimmed.rfind("print",0)==0){
            size_t q1 = trimmed.find('"');
            size_t q2 = trimmed.rfind('"');
            if(q1==std::string::npos || q2==q1){
                std::cerr << "Invalid print syntax: " << line << std::endl;
                return 1;
            }
            std::string msg = trimmed.substr(q1+1, q2-q1-1);
            std::string label = "msg" + std::to_string(msgId);
            data.push_back(label + ": .ascii \"" + escape(msg) + "\\n\"");
            data.push_back(label+"_len = . - "+label);
            text.push_back("    mov $1, %rax");
            text.push_back("    mov $1, %rdi");
            text.push_back("    lea " + label + "(%rip), %rsi");
            text.push_back("    mov $"+label+"_len, %rdx");
            text.push_back("    syscall");
            msgId++;
        } else if(trimmed.rfind("exit",0)==0){
            std::istringstream iss(trimmed.substr(4));
            long code = 0; iss >> code;
            text.push_back("    mov $60, %rax");
            text.push_back("    mov $" + std::to_string(code) + ", %rdi");
            text.push_back("    syscall");
        } else {
            std::cerr << "Unknown statement: " << line << std::endl;
            return 1;
        }
    }
    // ensure program exits
    if(text.empty() || text.back().find("syscall") == std::string::npos || text.back().find("60") == std::string::npos){
        text.push_back("    mov $60, %rax");
        text.push_back("    xor %rdi, %rdi");
        text.push_back("    syscall");
    }

    std::ofstream out(outPath);
    if(!out){
        std::cerr << "Cannot write to " << outPath << std::endl;
        return 1;
    }
    out << ".section .data\n";
    for(const auto& d : data) out << d << "\n";
    out << "\n.section .text\n.globl _start\n_start:\n";
    for(const auto& t : text) out << t << "\n";
    return 0;
}

