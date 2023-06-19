#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <iomanip>
#include <vector>
#include <deque>
#include <algorithm>
#include <iterator>

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

int simulatePageReplacement(const vector<MemoryReference> &memoryTrace, const unordered_map<string, MemoryMapping> &memoryMap, int numFrames, const string &replacementAlgorithm)
{
    int pageFaults = 0;
    deque<string> frameQueue;

    // Tabla de marcos
    vector<vector<string>> frameTable;

    for (const MemoryReference &reference : memoryTrace)
    {
        string address = reference.address;
        // Obtener la página (primeros dígitos de la dirección)
        string page = address.substr(0, address.length() - 3); 

        if (memoryMap.find(page) == memoryMap.end())
        {
            // La página no está en memoria
            pageFaults++;

            if (frameQueue.size() == numFrames)
            {
                // La memoria está llena, se debe reemplazar una página
                string pageToReplace;

                if (replacementAlgorithm == "FIFO")
                {
                    // FIFO: Se reemplaza la página más antigua en la cola
                    pageToReplace = frameQueue.front();
                    frameQueue.pop_front();
                }
                else if (replacementAlgorithm == "LRU")
                {
                    // LRU: Se reemplaza la página que no ha sido utilizada recientemente
                    pageToReplace = frameQueue.back();
                    frameQueue.pop_back();
                }
                else if (replacementAlgorithm == "OPT")
                {
                    // OPT: Se reemplaza la página que se utilizará más adelante en el futuro
                    unordered_map<string, int> pageNextUse;

                    // Calcular el próximo uso de cada página en el futuro
                    for (int i = 0; i < memoryTrace.size(); i++)
                    {
                        const MemoryReference &nextReference = memoryTrace[i];
                        string nextAddress = nextReference.address;
                        string nextPage = nextAddress.substr(0, nextAddress.length() - 3);

                        if (memoryMap.find(nextPage) != memoryMap.end())
                        {
                            // La página está en memoria
                            pageNextUse[nextPage] = i;
                        }
                    }

                    // Buscar la página que se utilizará más adelante
                    int maxNextUse = -1;
                    for (const string &frame : frameQueue)
                    {
                        int nextUse = pageNextUse[frame];
                        if (nextUse > maxNextUse)
                        {
                            maxNextUse = nextUse;
                            pageToReplace = frame;
                        }
                    }
                    frameQueue.erase(remove(frameQueue.begin(), frameQueue.end(), pageToReplace), frameQueue.end());
                }

                // Reemplazar la página
                frameQueue.push_back(page);

                // Actualizar la tabla de marcos
                vector<string> frameRow;
                for (const string &frame : frameQueue)
                {
                    frameRow.push_back(frame);
                }
                frameTable.push_back(frameRow);
            }
            else
            {
                // Aún hay espacio en la memoria, simplemente agregar la página a un marco
                frameQueue.push_back(page);

                // Actualizar la tabla de marcos
                vector<string> frameRow;
                for (const string &frame : frameQueue)
                {
                    frameRow.push_back(frame);
                }
                frameTable.push_back(frameRow);
            }
        }
    }
    return 0;
}

    //


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

    cout << setw(15) << "\n\033[1;33mMapa de memoria\033[0m\n" << endl;
    cout << setw(15) << left << "\033[1;32mMapa Virtual\033[0m" << std::endl;
    printMemoryMap(virtualMemoryMap);

    cout << endl;

    cout << setw(15) << left << "\033[1;32mMapa Fisico\033[0m" << std::endl;
    printMemoryMap(physicalMemoryMap);
    cout << endl << endl;

    // Simular el reemplazo de páginas con diferentes algoritmos
    vector<int> numFramesList = {10, 50, 100};
    vector<string> replacementAlgorithms = {"FIFO", "LRU", "OPT"};

    cout << setw(15) << left << "Algoritmo" << setw(10) << "Frames" << setw(15) << "Page Faults" << endl;

    for (const string &algorithm : replacementAlgorithms)
    {
        for (int numFrames : numFramesList)
        {
            int pageFaults = simulatePageReplacement(virtualMemoryTrace, virtualMemoryMap, numFrames, algorithm);
            cout << setw(15) << left << algorithm << setw(10) << numFrames << setw(15) << pageFaults << endl;
        }
    }

    return 0;
}