
#include <list>
#include <queue>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <iomanip>
#include <vector>
#include <bitset>
#include <cmath>
#include <algorithm> 
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

void generatePhysicalMemoryMap(const vector<MemoryReference>& physicalMemoryTrace, unordered_map<string, MemoryMapping>& physicalMemoryMap, int numFrames)
{
    int nextFrame = 0;

    for (const MemoryReference& reference : physicalMemoryTrace)
    {
        string address = reference.address;
        string page = address.substr(0, address.length() - 3); // Obtener la página (primeros dígitos de la dirección)

        if (physicalMemoryMap.find(page) == physicalMemoryMap.end())
        {
            // La página aún no ha sido asignada a un marco
            MemoryMapping mapping;
            mapping.page = ""; // No se utiliza en el mapa de memoria física
            mapping.frame = to_string(nextFrame);
            mapping.dirty = (reference.operation == 'W');
            physicalMemoryMap[page] = mapping;

            nextFrame = (nextFrame + 1) % numFrames;
        }
        else
        {
            // La página ya ha sido asignada a un marco
            MemoryMapping& mapping = physicalMemoryMap[page];
            mapping.dirty = mapping.dirty || (reference.operation == 'W');
        }
    }
}

int simulatePageFaultsFIFO(const vector<MemoryReference>& memoryTrace, int numFrames)
{
    unordered_map<string, MemoryMapping> pageTable;
    vector<string> frameQueue;
    int pageFaults = 0;

    for (const MemoryReference& reference : memoryTrace)
    {
        string address = reference.address;
        string page = address.substr(0, address.length() - 3); // Obtener la página (primeros dígitos de la dirección)

        if (pageTable.find(page) == pageTable.end())
        {
            // Page fault: la página no está en memoria
            pageFaults++;

            if (frameQueue.size() >= numFrames)
            {
                // Se debe reemplazar una página usando FIFO
                string pageToRemove = frameQueue.front();
                frameQueue.erase(frameQueue.begin());
                pageTable.erase(pageToRemove);
            }

            // Agregar la nueva página a la tabla y a la cola de marcos
            MemoryMapping mapping;
            mapping.page = page;
            mapping.frame = to_string(frameQueue.size());
            mapping.dirty = (reference.operation == 'W');
            pageTable[page] = mapping;
            frameQueue.push_back(page);
        }
    }

    return pageFaults;
}

int simulatePageFaultsLRU(const vector<MemoryReference>& memoryTrace, int numFrames)
{
    unordered_map<string, MemoryMapping> pageTable;
    vector<string> frameStack;
    int pageFaults = 0;

    for (const MemoryReference& reference : memoryTrace)
    {
        string address = reference.address;
        string page = address.substr(0, address.length() - 3); // Obtener la página (primeros dígitos de la dirección)

        if (pageTable.find(page) == pageTable.end())
        {
            // Page fault: la página no está en memoria
            pageFaults++;

            if (frameStack.size() >= numFrames)
            {
                // Se debe reemplazar una página usando LRU
                string pageToRemove = frameStack.front();
                frameStack.erase(frameStack.begin());
                pageTable.erase(pageToRemove);
            }

            // Agregar la nueva página a la tabla y al stack de marcos
            MemoryMapping mapping;
            mapping.page = page;
            mapping.frame = to_string(frameStack.size());
            mapping.dirty = (reference.operation == 'W');
            pageTable[page] = mapping;
            frameStack.push_back(page);
        }
        else
        {
            // La página ya está en memoria, actualizar su posición en el stack
            auto it = find(frameStack.begin(), frameStack.end(), page);
            frameStack.erase(it);
            frameStack.push_back(page);
        }
    }

    return pageFaults;
}

int simulatePageFaultsOPT(const vector<MemoryReference>& memoryTrace, const unordered_map<string, MemoryMapping>& physicalMemoryMap, int numFrames)
{
    unordered_map<string, int> pageAccessCount; // Contador de accesos a cada página
    int pageFaults = 0;

    for (int i = 0; i < memoryTrace.size(); i++)
    {
        string address = memoryTrace[i].address;
        string page = address.substr(0, address.length() - 3); // Obtener la página (primeros dígitos de la dirección)

        if (pageAccessCount.find(page) == pageAccessCount.end())
        {
            // Página no registrada, inicializar contador
            pageAccessCount[page] = 0;
        }

        if (i >= numFrames && pageAccessCount.size() >= numFrames)
        {
            // Page fault: la página no está en memoria
            pageFaults++;

            // Encontrar la página más lejana en el futuro para reemplazarla
            string pageToRemove;
            int maxDistance = -1;

            for (const auto& entry : pageAccessCount)
            {
                string currentPage = entry.first;
                int currentDistance = memoryTrace.size(); // Valor inicial alto para asegurar que se actualice en el siguiente bucle

                for (int j = i + 1; j < memoryTrace.size(); j++)
                {
                    if (memoryTrace[j].address.substr(0, memoryTrace[j].address.length() - 3) == currentPage)
                    {
                        currentDistance = j;
                        break;
                    }
                }

                if (currentDistance > maxDistance)
                {
                    maxDistance = currentDistance;
                    pageToRemove = currentPage;
                }
            }

            // Eliminar la página seleccionada de la tabla
            auto it = pageAccessCount.find(pageToRemove);
            pageAccessCount.erase(it);
        }

        pageAccessCount[page]++;
    }

    return pageFaults;
}

void printSummary(const unordered_map<string, MemoryMapping>& physicalMemoryMap, const vector<MemoryReference>& memoryTrace, int pageFaults, int numFrames)
{
    int writesToDisk = 0;
    double eat = 0.0;

    for (const MemoryReference& reference : memoryTrace)
    {
        string address = reference.address;
        string page = address.substr(0, address.length() - 3); // Obtener la página (primeros dígitos de la dirección)

        if (physicalMemoryMap.find(page) != physicalMemoryMap.end())
        {
            // La página está en memoria física
            MemoryMapping mapping = physicalMemoryMap.at(page);
            if (reference.operation == 'W')
            {
                mapping.dirty = true; // Actualizar dirty bit en caso de escritura
            }
            // physicalMemoryMap[page] = mapping;
        }
        else
        {
            // La página no está en memoria física, se realizó un page fault
            writesToDisk += (reference.operation == 'W') ? 1 : 0;
        }
    }

    eat = pageFaults * 100.0; // Suponiendo un valor de 100 ns de acceso a memoria

    cout << "Resumen de la simulación:" << endl;
    cout << "=========================" << endl;
    cout << "Número de page faults: " << pageFaults << endl;
    cout << "Número de reemplazos realizados: " << numFrames << endl;
    cout << "Número de escrituras a disco: " << writesToDisk << endl;
    cout << "Estimación EAT (tiempo de acceso a memoria): " << eat << " ns" << endl;
}

int main()
{
    // Parámetros del sistema
    int frames[] = { 10, 50, 100 };
    string memoryFile = "physical_memory_map.txt";
    int numAddresses = 1000;

    // Cargar el trace de memoria
    vector<MemoryReference> memoryTrace;
    loadMemoryTrace(memoryFile, memoryTrace, numAddresses);

    // Realizar simulación para cada cantidad de frames
    for (int i = 0; i < sizeof(frames) / sizeof(frames[0]); i++)
    {
        int numFrames = frames[i];

        // Simulación FIFO
        cout << "Simulación FIFO para " << numFrames << " frames:" << endl;
        unordered_map<string, MemoryMapping> physicalMemoryMapFIFO;
        generatePhysicalMemoryMap(memoryTrace, physicalMemoryMapFIFO, numFrames);
        int pageFaultsFIFO = simulatePageFaultsFIFO(memoryTrace, numFrames);
        printSummary(physicalMemoryMapFIFO, memoryTrace, pageFaultsFIFO, numFrames);
        cout << endl;

        // Simulación LRU
        cout << "Simulación LRU para " << numFrames << " frames:" << endl;
        unordered_map<string, MemoryMapping> physicalMemoryMapLRU;
        generatePhysicalMemoryMap(memoryTrace, physicalMemoryMapLRU, numFrames);
        int pageFaultsLRU = simulatePageFaultsLRU(memoryTrace, numFrames);
        printSummary(physicalMemoryMapLRU, memoryTrace, pageFaultsLRU, numFrames);
        cout << endl;

        // Simulación OPT
        cout << "Simulación OPT para " << numFrames << " frames:" << endl;
        unordered_map<string, MemoryMapping> physicalMemoryMapOPT;
        generatePhysicalMemoryMap(memoryTrace, physicalMemoryMapOPT, numFrames);
        int pageFaultsOPT = simulatePageFaultsOPT(memoryTrace, physicalMemoryMapOPT, numFrames);
        printSummary(physicalMemoryMapOPT, memoryTrace, pageFaultsOPT, numFrames);
        cout << endl;
    }

    return 0;
}
