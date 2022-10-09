#include <iostream>
#include <fstream>
#include <string>
#include <string.h>


// function declarations
void isInstruction(std::string line);
void parseLabels(std::string line);
void assemble(std::string line);
int typeR(int op, int rs, int rt, int rd, int shamt, int func);
int typeI(int op, int rs, int rt, int addr);
int regToInt(std::string regi);
int immToInt(std::string num);

// global variables
std::string instruction[100];
int instruction_count = 0;
int instr_addr = 0;
bool boolInstruction[100]{false};
int line_count = 0;

// opcode and label structures
struct instructions
{
    std::string name;
    int opcode;
    char type;
};

instructions list[29]{ {"add", 0b100000, 'R'}, {"addi", 0b001000, 'I'}, {"addiu", 0b001001, 'I'}, {"addu", 0b100001, 'R'},
    {"and", 0b100100, 'R'}, {"andi", 0b001100, 'I'}, {"beq", 0b000100, 'I'}, {"bne", 0b000101 , 'I'}, {"jr", 0b001000, 'R'},
    {"lbu", 0b100100, 'I'}, {"lhu", 0b100101, 'I'}, {"ll", 0b110000, 'I'}, {"lui", 0b001111, 'I'}, {"lw", 0b100011, 'I'},
    {"nor", 0b100111, 'R'}, {"or", 0b100101, 'R'}, {"ori", 0b001101, 'I'}, {"slt", 0b101010, 'R'}, {"slti", 0b001010, 'I'},
    {"sltiu", 0b001011, 'I'}, {"sltu", 0b101011, 'R'}, {"sll", 0b000000, 'R'}, {"srl", 0b000010, 'R'}, {"sb", 0b101000, 'I'},
    {"sc", 0b111000, 'I'}, {"sh", 0b101001, 'I'}, {"sw", 0b101011, 'I'}, {"sub", 0b100010, 'R'}, {"subu", 0b100011, 'R'} };

struct registers
{
    std::string reg;
    int code;
};

registers reg[32]{{"$zero", 0b00000}, {"at", 0b00001}, {"$v0", 0b00010}, {"$v1", 0b00011}, {"$a0", 0b00100},
    {"$a1", 0b00101}, {"$a2", 0b00110}, {"$a3", 0b00111}, {"$t0", 0b01000}, {"$t1", 0b01001}, {"$t2", 0b01010}
, {"$t3", 0b01011} , {"$t4", 0b01100} , {"$t5", 0b01101} , {"$t6", 0b01110} , {"$t7", 0b01111}
, {"$s0", 0b10000} , {"$s1", 0b10001} , {"$s2", 0b10010} , {"$s3", 0b10011} , {"$s4", 0b10100} , {"$s5", 0b10101}
, {"$s6", 0b10110} , {"$s7", 0b10111} , {"$t8", 0b11000} , {"$t9", 0b11001} , {"$k0", 0b11010} , {"$k1", 0b11011}
, {"$gp", 0b11100} , {"$sp", 0b11101} , {"$fp", 0b11110} , {"$ra", 0b11111} };

struct labels
{
    std::string label;
    int address;
};

labels lab[100];
int label_count = 0;

int main(int argc, char** argv)
{
    std::ifstream file(argv[1]); // reads user input into file

    if (file.is_open()) { 
        std::string line;

        while (std::getline(file, line)) { // reads each line to check if it is an instruction or not 
            isInstruction(line);
            line_count++;
        }

        file.close();
    }

    line_count = 0;

    file.open(argv[1], std::ios::in);
    if (file.is_open()) {
        std::string line;

        while (std::getline(file, line)) {
            parseLabels(line);              // parses each line to search and take note of labels and their addresses 
            line_count++;
        }

        file.close();
    }

    line_count = 0;

    file.open(argv[1], std::ios::in);
    if (file.is_open()) {
        std::string line;

        while (std::getline(file, line)) {
            assemble(line);                 // reads line by line for MIPS instructions and translates them into 32-bit machine code

            if (boolInstruction[line_count]) {
                instr_addr++;
            }

            line_count++;
        }

        file.close();
    }

    // Creates output file and writes each machine code instruction line by line into said file
    std::string in = argv[1];
    in.replace(in.size() - 2, 2, ".obj");
    std::ofstream outfile;
    outfile.open(in);
    for (int i = 0; i < instruction_count; i++) {
        outfile << instruction[i] << std::endl;
    }

    outfile.close();

    return 0;
}

// Given a string input, checks if the first portion of the string is a valid MIPS instruction
// Fills out an array populated with bools. Adds true if the line is an instruction, false if not
void isInstruction(std::string line)
{
    int i = 0;
    std::string str;

    if (line == "") {
        return;
    }

    while (!isalpha(line[i]) && line[i] != '\0')
        i++;

    while (line[i] != '\t' && line[i] != '\0' && line[i] != ' ') {
        if (isalpha(line[i])) {
            str += line[i];
        }
        i++;
    }

    for (int k = 0; k < 29; k++) {
        if (str == list[k].name) {
            boolInstruction[line_count] = true;
        }
    }
}

// Checks each line for labels and stores the label name and address in an array
void parseLabels(std::string line)
{
    if (strstr(line.c_str(), ":")) {
        line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return std::isblank(c); }), line.end());
        line.replace(line.length() - 1, 1, "");
        lab[label_count].label = line;

        int i = line_count;

        while (boolInstruction[i] == false) {
            i++;
        }

        for (int j = i; j >= 0; j--) {
            if (boolInstruction[j] == false) {
                i--;
            }
        }

        lab[label_count].address = i;
        label_count++;
    }
}

// Parses each line for opcodes and arguments, and then translates valid instructions into machine code
void assemble(std::string line)
{
    // Initializing variables and strings to break apart each line into
    int code = 0;
    std::string str = "";
    std::string one = "";
    std::string two = "";
    std::string three = "";
    int three1 = -1;
    int i = 0;

    // If line is blank, return
    if (line == three) {
        return;
    }

    // Skips over whitespace
    while (!isalpha(line[i]) && line[i] != '\0') {
        if (line[i] == '.' || line[i] == '#') {
            return;
        }
        i++;
    }



    // Reads instruction into string
    while (line[i] != '\t' && line[i] != '\0' && line[i] != ' ') {
        if (isalpha(line[i]) || line[i] == ':') {
            str += line[i];
       }
        i++;
    }

    if (str == "" || line[i - 1] == ':' || str == "syscall" || str == "li") {
        return;
    }

    int j = 1;

    // Reads first register into string
    while (line[i] != '\0' && j != 0) {
        if (line[i] == '$') {
            j = i;
            while (line[j] != ',') {
                one += line[j];
                j++;
                i++;
            }

            j = 0;
        }

        i++;
    }

    j = 1;

    // Reads second register into string
    while (line[i] != '\0' && j != 0) {
        if (line[i] == '$') {
            j = i;
            while (line[j] != ',' && line[j] != ')' && line[i] != '\0') {
                two += line[j];
                j++;
                i++;
            }

            j = 0;
        }

        if (i < line.length())
            i++;
    }

    j = 1;

    // Reads third register, immediate, or label into string
    while (line[i] != '\0' && j != 0) {
        if (line[i] == '$' || isdigit(line[i]) ||isalpha(line[i])) {
            j = i;
            while (line[j] != '\0') {
                three += line[j];
                j++;
            }

            j = 0;
        }

        i++;
    }

    // If any field is blank, instruction is invalid
    if (one == "" || two == "") {
        std::cout << "Cannot assemble " << line << " at line " << line_count << std::endl;
        return;
    }

    // Loops through dictionary of instructions to find the correct opcode
    for (int k = 0; k < 29; k++) {
        if (str == list[k].name) {
            if (list[k].type == 'R') {
                if (regToInt(one) != -1 && regToInt(two) != -1) {                                                 // If registers are valid, continue to writing the machine code
                    if (str == "sll" || str == "srl") {
                        code = typeR(0, 0, regToInt(two), regToInt(one), immToInt(three), list[k].opcode);
                    }

                    else if (regToInt(three) != 0) {                                                             // If registers are valid, continue to writing the machine code
                        code = typeR(0, regToInt(two), regToInt(three), regToInt(one), 0, list[k].opcode);
                    }

                    else {
                        std::cout << "Cannot assemble " << line << " at line " << line_count << std::endl;
                        return;
                    }
                }

                else {
                    std::cout << "Cannot assemble " << line << " at line " << line_count << std::endl;
                    return;
                }
            }

            else if (list[k].type == 'I') {
                if (str == "sw" || str == "lw") {
                    while (line[i] != ' ') {
                        i--;
                    }

                    i++;

                    while (line[i] != '(') {
                        three += line[i];
                        i++;
                    }


                    if (regToInt(one) != -1 && regToInt(two) != -1) {                      // If registers are valid, continue to writing the machine code
                        code = typeI(list[k].opcode, regToInt(two), regToInt(one), immToInt(three));
                        break;
                    }
                }

                else if (str == "bne" || str == "beq") {
                    for (int l = 0; l < 100; l++) {
                        if (three == lab[l].label) {
                                three1 = lab[l].address - (instr_addr + 1);
                            break;
                        }
                    }
                     
                    if (three1 != -1)
                        code = typeI(list[k].opcode, regToInt(one), regToInt(two), three1);

                    else {
                        std::cout << "Cannot assemble " << line << " at line " << line_count << std::endl;
                        return;
                    }

                }

                else {
                    if (regToInt(one) != -1 && regToInt(two) != -1 && immToInt(three) != -1) {
                        code = typeI(list[k].opcode, regToInt(two), regToInt(one), immToInt(three));
                    }

                    else {
                        std::cout << "Cannot assemble " << line << " at line " << line_count << std::endl;
                        return;
                    }
                }

                break;
            }

            else {
                std::cout << "Cannot assemble " << line << " at line " << line_count << std::endl;
                return;
            }
        }
    }
    
    // If registers are valid, continue to writing the machine code
    if (code != 0) {    
        // Translates the machine code instruction to a hex string
        char hex[9]{0};
        _itoa_s(code, hex, 16);

        std::string s(hex);

        // Zero fills each instruction to 8-digit hex
        while (s.length() != 8) {
            s.insert(0, "0");
        }

        instruction[instruction_count] = s;
        instruction_count++;
    }

    else {
        std::cout << "Cannot assemble " << line << " at line " << line_count << std::endl;
    }
}

// Assembles R type instructions, given valid registers and function code
int typeR(int op, int rs, int rt, int rd, int shamt, int func)
{
    int32_t ans = 0;
    ans = (op << 26);
    ans |= (rs << 21);
    ans |= (rt << 16);
    ans |= (rd << 11);
    ans |= (shamt << 6);
    ans |= func;

    return ans;
}

// Assembles I type instructions given valid immediate and registers 
int typeI(int op, int rs, int rt, int addr) {
    int32_t ans = 0;
    ans = (op << 26);
    ans |= (rs << 21);
    ans |= (rt << 16);
    ans |= (addr & 65535);

    return ans;
}

// Converts register string into integer register codes by looping through an array of structs 
// which holds register names and codes
int regToInt(std::string regi) {
    
    int num = -1;

    for (int i = 0; i < 32; i++) {
        if (regi == reg[i].reg) {
            num = reg[i].code;
            break;
        }
    }
        
    return num;
}

// Converts immediate strings into an integer
int immToInt(std::string num) {
    if (!num.empty() && std::find_if(num.begin(),
        num.end(), [](unsigned char c) { return !std::isdigit(c); }) == num.end()) {
        return std::stoi(num);
    }

    else {
        return -1;
    }
}