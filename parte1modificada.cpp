#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <iomanip>
#include <vector>
#include <bitset>
#include <sstream>
#include <cmath>

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

void loadMemoryTrace(const string& memoryFile, vector<MemoryReference>& memoryTrace, int numAddresses)
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

void generatePhysicalMemoryMap(const vector<MemoryReference>& physicalMemoryTrace, unordered_map<string, MemoryMapping>& physicalMemoryMap)
{
    const int pageSize = 4096;
    const int numFrames = 256;
    int nextFrame = 0;

    for (const MemoryReference& reference : physicalMemoryTrace)
    {
        string address = reference.address;
        string page = address.substr(0, address.length() - 3); // Obtener la p�gina (primeros d�gitos de la direcci�n)

        if (physicalMemoryMap.find(page) == physicalMemoryMap.end())
        {
            // La p�gina a�n no ha sido asignada a un marco
            MemoryMapping mapping;
            mapping.page = ""; // No se utiliza en el mapa de memoria f�sica
            mapping.frame = to_string(nextFrame);
            mapping.dirty = (reference.operation == 'W');
            physicalMemoryMap[page] = mapping;

            nextFrame = (nextFrame + 1) % numFrames;
        }
        else
        {
            // La p�gina ya ha sido asignada a un marco
            MemoryMapping& mapping = physicalMemoryMap[page];
            mapping.dirty = mapping.dirty || (reference.operation == 'W');
        }
    }
}
void generateVirtualMemoryMap(const vector<MemoryReference>& virtualMemoryTrace, unordered_map<string, MemoryMapping>& virtualMemoryMap)
{
    const int pageSize = 4096;
    const int numPages = 1048576;
    const int numFrames = 256;
    int nextPage = 0;
    int nextFrame = 0;

    for (const MemoryReference& reference : virtualMemoryTrace)
    {
        string address = reference.address;
        string page = address.substr(0, address.length() - 3); // Obtener la p�gina (primeros d�gitos de la direcci�n)
        int pageNumber = stoi(page);                           // Convertir el n�mero de p�gina a entero

        if (pageNumber < numPages)
        {
            // La p�gina no excede el n�mero m�ximo de p�ginas
            if (nextFrame >= numFrames)
            {
                // No hay m�s marcos de p�gina disponibles
                MemoryMapping mapping;
                mapping.page = page;
                mapping.frame = "Sin asignar";
                mapping.dirty = (reference.operation == 'W');
                virtualMemoryMap[page] = mapping;
            }
            else if (virtualMemoryMap.find(page) == virtualMemoryMap.end())
            {
                // La p�gina a�n no ha sido asignada a un marco
                MemoryMapping mapping;
                mapping.page = page;
                mapping.frame = to_string(nextFrame);
                mapping.dirty = (reference.operation == 'W');
                virtualMemoryMap[page] = mapping;

                nextFrame++;
            }
            else
            {
                // La p�gina ya ha sido asignada a un marco
                MemoryMapping& mapping = virtualMemoryMap[page];
                mapping.dirty = mapping.dirty || (reference.operation == 'W');
            }
        }
        else
        {
            // El n�mero de p�gina excede el n�mero m�ximo de p�ginas
            int adjustedPageNumber = pageNumber % numPages; // Ajustar el n�mero de p�gina dentro del rango permitido
            MemoryMapping mapping;
            mapping.page = to_string(adjustedPageNumber); // Representar el n�mero de p�gina ajustado con d�gitos ar�bigos
            mapping.frame = nextFrame < numFrames ? to_string(nextFrame) : "Sin asignar";
            mapping.dirty = (reference.operation == 'W');
            virtualMemoryMap[page] = mapping;

            nextFrame++;
        }
    }
}

void printVirtualMemoryMap(const unordered_map<string, MemoryMapping>& virtualMemoryMap, const vector<MemoryReference>& virtualMemoryTrace)
{
    cout << setw(15) << left << "Direcci�n" << setw(10) << "Operaci�n" << setw(15) << "P�gina" << setw(10) << "Marco" << setw(10) << "Bit de Modificaci�n" << endl;

    for (const MemoryReference& reference : virtualMemoryTrace)
    {
        const string& address = reference.address;
        const string page = address.substr(0, address.length() - 3); // Obtener la p�gina (primeros d�gitos de la direcci�n)

        const MemoryMapping& mapping = virtualMemoryMap.at(page);
        cout << setw(15) << left << address << setw(10) << reference.operation << setw(15) << mapping.page << setw(10) << mapping.frame << setw(10) << (mapping.dirty ? "\033[1;31m1\033[0m" : "0") << endl;
    }
}

void printPhysicalMemoryMap(const unordered_map<string, MemoryMapping>& physicalMemoryMap, const vector<MemoryReference>& physicalMemoryTrace)
{
    cout << setw(15) << left << "Direcci�n" << setw(10) << "Operaci�n" << setw(10) << "Marco" << setw(10) << "Bit de Modificaci�n" << endl;

    for (size_t i = 0; i < physicalMemoryTrace.size(); i++)
    {
        const MemoryReference& reference = physicalMemoryTrace[i];
        const string& address = reference.address;
        const string page = address.substr(0, address.length() - 3); // Obtener la p�gina (primeros d�gitos de la direcci�n)

        const MemoryMapping& mapping = physicalMemoryMap.at(page);
        cout << setw(15) << left << address << setw(10) << reference.operation << setw(10) << mapping.frame << setw(10) << (mapping.dirty ? "\033[1;31m1\033[0m" : "0") << endl;
    }
}

void saveMemoryMapToFile(const unordered_map<string, MemoryMapping>& memoryMap, const vector<MemoryReference>& memoryTrace, const string& filename)
{
    ofstream file(filename);

    file << setw(15) << left << "Direcci�n" << setw(10) << "Operaci�n" << setw(15) << "P�gina" << setw(10) << "Marco" << setw(10) << "Bit de Modificaci�n" << endl;

    for (const MemoryReference& reference : memoryTrace)
    {
        const string& address = reference.address;
        const string page = address.substr(0, address.length() - 3); // Obtener la p�gina (primeros d�gitos de la direcci�n)

        const MemoryMapping& mapping = memoryMap.at(page);
        file << setw(15) << left << address << setw(10) << reference.operation << setw(15) << mapping.page << setw(10) << mapping.frame << setw(10) << (mapping.dirty ? "1" : "0") << endl;
    }

    file.close();
}

int main(int argc, char* argv[])
{
    string virtualMemoryFile = "bzip.trace";
    string physicalMemoryFile = "gcc.trace";
    int numAddresses;

    cout << "Ingrese el n�mero de direcciones a leer: ";
    cin >> numAddresses;

    vector<MemoryReference> virtualMemoryTrace;
    vector<MemoryReference> physicalMemoryTrace;

    unordered_map<string, MemoryMapping> virtualMemoryMap;
    unordered_map<string, MemoryMapping> physicalMemoryMap;

    loadMemoryTrace(virtualMemoryFile, virtualMemoryTrace, numAddresses);
    loadMemoryTrace(physicalMemoryFile, physicalMemoryTrace, numAddresses);

    generateVirtualMemoryMap(virtualMemoryTrace, virtualMemoryMap);
    generatePhysicalMemoryMap(physicalMemoryTrace, physicalMemoryMap);

    cout << setw(15) << "\n\033[1;33mMapa de memoria\033[0m\n"
        << endl;
    cout << setw(15) << left << "\033[1;32mMapa Virtual\033[0m" << std::endl;
    printVirtualMemoryMap(virtualMemoryMap, virtualMemoryTrace);

    cout << endl;

    /* cout << setw(15) << left << "\033[1;32mMapa Fisico\033[0m" << std::endl;
    printPhysicalMemoryMap(physicalMemoryMap, physicalMemoryTrace);
    cout << endl
         << endl; */

    string virtualMemoryMapFile = "virtual_memory_map.txt";
    string physicalMemoryMapFile = "physical_memory_map.txt";

    saveMemoryMapToFile(virtualMemoryMap, virtualMemoryTrace, virtualMemoryMapFile);
    saveMemoryMapToFile(physicalMemoryMap, physicalMemoryTrace, physicalMemoryMapFile);

    cout << "Los mapas de memoria se han guardado en los archivos: " << virtualMemoryMapFile << " y " << physicalMemoryMapFile << endl;

    return 0;
}
