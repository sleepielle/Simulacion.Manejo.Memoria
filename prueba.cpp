#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <iomanip>
#include <vector>

using namespace std;

struct MemoryReference
{
    string address;
    char operation;
};

struct MemoryMapping
{
    string page;
    string frame;
    bool dirty;
};

void loadMemoryTrace(const string &memoryFile, vector<MemoryReference> &memoryTrace, int numAddresses)
{
    ifstream file(memoryFile);
    string line;
    int count = 0;

    while (getline(file, line) && count < numAddresses)
    {
        istringstream iss(line);
        MemoryReference reference;
        if (iss >> reference.address >> reference.operation)
        {
            memoryTrace.push_back(reference);
            count++;
        }
    }
}

void generateMemoryMap(const vector<MemoryReference> &memoryTrace, unordered_map<string, MemoryMapping> &memoryMap)
{
    const int pageSize = 4096;
    const int frameSize = 4096;
    int nextPage = 0;
    int nextFrame = 0;

    for (const MemoryReference &reference : memoryTrace)
    {
        string address = reference.address;
        string page = address.substr(0, address.length() - 3); // Obtener la página (primeros dígitos de la dirección)

        if (memoryMap.find(page) == memoryMap.end())
        {
            // La página aún no ha sido asignada a un marco
            MemoryMapping mapping;
            mapping.page = page;
            mapping.frame = to_string(nextFrame);
            mapping.dirty = (reference.operation == 'W');
            memoryMap[page] = mapping;

            nextFrame++;
        }
        else
        {
            // La página ya ha sido asignada a un marco
            MemoryMapping &mapping = memoryMap[page];
            mapping.dirty = mapping.dirty || (reference.operation == 'W');
        }
    }
}

void printMemoryMap(const unordered_map<string, MemoryMapping> &memoryMap)
{
    cout << setw(15) << left << "Dirección" << setw(10) << "Operación  " << setw(10) << "Página" << setw(10) << "Marco" << setw(10) << "Bit de Modificación" << endl;

    for (const auto &entry : memoryMap)
    {
        const MemoryMapping &mapping = entry.second;
        cout << setw(15) << left << mapping.page + "XXX" << setw(10) << (mapping.dirty ? "W" : "R") << setw(10) << mapping.page << setw(10) << mapping.frame << setw(10) << (mapping.dirty ? "\033[1;31m1\033[0m" : "0") << endl;
    }
}

int main(int argc, char *argv[])
{
    string virtualMemoryFile = "bzip.trace";
    string physicalMemoryFile = "gcc.trace";
    int numAddresses;

    cout << "Ingrese el número de direcciones a leer: ";
    cin >> numAddresses;

    vector<MemoryReference> virtualMemoryTrace;
    vector<MemoryReference> physicalMemoryTrace;

    unordered_map<string, MemoryMapping> virtualMemoryMap;
    unordered_map<string, MemoryMapping> physicalMemoryMap;

    loadMemoryTrace(virtualMemoryFile, virtualMemoryTrace, numAddresses);
    loadMemoryTrace(physicalMemoryFile, physicalMemoryTrace, numAddresses);

    generateMemoryMap(virtualMemoryTrace, virtualMemoryMap);
    generateMemoryMap(physicalMemoryTrace, physicalMemoryMap);

    cout << setw(15) << "\n\033[1;33mMapa de memoria\033[0m\n"
         << endl;
    cout << setw(15) << left << "\033[1;32mMapa Virtual\033[0m" << std::endl;
    printMemoryMap(virtualMemoryMap);

    cout << endl;

    cout << setw(15) << left << "\033[1;32mMapa Fisico\033[0m" << std::endl;
    printMemoryMap(physicalMemoryMap);
    cout << endl
         << endl;

    return 0;
}

/* #include <iostream>
#include <istream>
#include <fstream>
#include <string>

using namespace std;

bool readPhysicalMemory(string);
bool writeVirtualMemory(string);

int main(int argc, char *argv[])
{

    string memoryAddress = argv[1];
    string operation = argv[2];

    string virtualMemoryFile = "bzip.trace";
    string physicalMemoryFile = "gcc.trace";

    readPhysicalMemory(physicalMemoryFile);
}

bool writeVirtualMemory(string memoryFile)
{
    return true;
}

bool readPhysicalMemory(string memoryFile)
{
    std::ifstream file(memoryFile);
    std::string line;

    while (getline(file, line))
    {
        cout << line << endl;
    }
    return true;
} */
