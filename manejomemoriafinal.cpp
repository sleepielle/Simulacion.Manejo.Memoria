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

void generatePhysicalMemoryMap(const vector<MemoryReference> &physicalMemoryTrace, 
unordered_map<string, MemoryMapping> &physicalMemoryMap, int numFrames)
{
    int nextFrame = 0;

    for (const MemoryReference &reference : physicalMemoryTrace)
    {
        string address = reference.address;
        string page = address.substr(0, address.length() - 3); // Obtener la página (primeros dígitos de la dirección)

        if (physicalMemoryMap.find(page) == physicalMemoryMap.end())
        {
            // La página aún no ha sido asignada a un marco
            MemoryMapping mapping;
            mapping.page = ""; // No se utiliza en el mapa de memoria física
            mapping.frame = nextFrame;
            mapping.dirty = (reference.operation == 'W');
            physicalMemoryMap[page] = mapping;

            nextFrame = (nextFrame + 1) % numFrames;
        }
        else
        {
            // La página ya ha sido asignada a un marco
            MemoryMapping &mapping = physicalMemoryMap[page];
            mapping.dirty = mapping.dirty || (reference.operation == 'W');
        }
    }
    //Reinicia nextFrame al final de cada simulacion
    nextFrame = 0;
}

void loadMemoryTrace(vector<MemoryReference> &memoryTrace, int numAddresses)
{
    ifstream file("gcc.trace");
    string line;
    int count = 0;

    if (file.is_open())
    {
        while (getline(file, line) && (numAddresses == -1 || count < numAddresses))
        {
            istringstream iss(line);
            MemoryReference reference;
            if (iss >> reference.address >> reference.operation)
            {
                memoryTrace.push_back(reference);
                count++;
            }
        }
        file.close();
    }
}

int simulatePageFaultsFIFO(const vector<MemoryReference> &memoryTrace, int numFrames, int &replacements)
{
    unordered_map<string, MemoryMapping> pageTable; //pageTable PhysicalMemoryMap
    queue<string> frameQueue;
    int pageFaults = 0;
    int replace = 0;

    for (const MemoryReference &reference : memoryTrace)
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
                frameQueue.pop();
                pageTable.erase(pageToRemove);
                replace++;
            }

            // Agregar la nueva página a la tabla y a la cola de marcos
            MemoryMapping mapping;
            mapping.page = page;
            mapping.frame = to_string(frameQueue.size());
            mapping.dirty = (reference.operation == 'W');
            pageTable[page] = mapping;
            frameQueue.push(page);
        }
    }

    replacements = replace;

    return pageFaults;
}

int simulatePageFaultsLRU(const vector<MemoryReference> &memoryTrace, int numFrames, int &replacement)
{
    unordered_map<string, MemoryMapping> pageTable;//Page Table PhyscalMemoryMap
    list<string> frameList;
    int pageFaults = 0;
    int replace = 0;

    for (const MemoryReference &reference : memoryTrace)
    {
        string address = reference.address;
        string page = address.substr(0, address.length() - 3); // Obtener la página (primeros dígitos de la dirección)

        if (pageTable.find(page) == pageTable.end())
        {
            // Page fault: la página no está en memoria
            pageFaults++;

            if (frameList.size() >= numFrames)
            {
                // Se debe reemplazar una página usando LRU
                string pageToRemove = frameList.front();
                frameList.pop_front();
                pageTable.erase(pageToRemove);
                replace++;
            }

            // Agregar la nueva página a la tabla y al stack de marcos
            MemoryMapping mapping;
            mapping.page = page;
            mapping.frame = to_string(frameList.size());
            mapping.dirty = (reference.operation == 'W');
            pageTable[page] = mapping;
            frameList.push_back(page);
        }
        else
        {
            // La página ya está en memoria, actualizar su posición en la lista
            frameList.remove(page);
            frameList.push_back(page);
        }
    }

    replacement = replace;
    return pageFaults;
}

int simulatePageFaultsOPT(const vector<MemoryReference> &memoryTrace, int numFrames, int &replacements)
{
    unordered_map<string, MemoryMapping> pageTable; // pageTable physicalMemoryMap
    int pageFaults = 0;
    int replace = 0;

    for (int i = 0; i < memoryTrace.size(); i++)
    {
        string address = memoryTrace[i].address;
        string page = address.substr(0, address.length() - 3); // Obtener la página (primeros dígitos de la dirección)

        if (pageTable.find(page) == pageTable.end())
        {
            // Page fault: la página no está en memoria
            pageFaults++;

            if (pageTable.size() >= numFrames)
            {
                // Se debe reemplazar una página usando OPT
                string pageToRemove;
                int maxDistance = -1;

                // Recorrer las páginas en memoria y encontrar la página más lejana en el futuro
                for (const auto &entry : pageTable)
                {
                    string currentPage = entry.first;
                    int currentDistance = -1;

                    for (int j = i + 1; j < memoryTrace.size(); j++)
                    {
                        if (memoryTrace[j].address.substr(0, memoryTrace[j].address.length() - 3) == currentPage)
                        {
                            currentDistance = j;
                            break;
                        }
                    }

                    if (currentDistance == -1)
                    {
                        // No se encontraron referencias futuras de la página, por lo que puede ser reemplazada inmediatamente
                        pageToRemove = currentPage;
                        break;
                    }

                    if (currentDistance > maxDistance)
                    {
                        maxDistance = currentDistance;
                        pageToRemove = currentPage;
                    }
                }

                pageTable.erase(pageToRemove);
                replace++;
            }

            // Agregar la nueva página a la tabla
            MemoryMapping mapping;
            mapping.page = page;
            mapping.frame = "";
            mapping.dirty = (memoryTrace[i].operation == 'W');
            pageTable[page] = mapping;
        }
    }

    replacements = replace;

    return pageFaults;
}


void printSummary(const unordered_map<string, MemoryMapping> &physicalMemoryMap, 
const vector<MemoryReference> &memoryTrace,int pageFaults, int numFrames, int replace)
{
    int writesToDisk = 0;
    double eat = 0.0;

    for (const MemoryReference &reference : memoryTrace)
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

        if (reference.operation == 'W')
        {
            writesToDisk++; // Incrementar writesToDisk para cada operación de escritura
        }
    }

    eat = pageFaults * 100.0; // Suponiendo un valor de 100 ns de acceso a memoria
  
    std::cout << "+------------------------------------------------+" << std::endl;
    std::cout << "| Page Faults:                       |" << std::setw(10) << std::left << pageFaults << " |" << std::endl;
    std::cout << "| Reemplazos realizados:             |" << std::setw(10) << std::left << replace << " |" << std::endl;
    std::cout << "| Escrituras a disco:                |" << std::setw(10) << std::left << writesToDisk << " |" << std::endl;
    std::cout << "| EAT (tiempo de acceso a memoria):  |" << std::setw(10) << std::left << eat << " |" << std::endl;
    std::cout << "+------------------------------------------------+" << std::endl;
}


int main()
{
    int frames[] = {10, 50, 100};
    int numAddresses = -1; // Valor predeterminado para leer todo el archivo

    // Preguntar al usuario si desea leer todo el archivo o ingresar la cantidad de direcciones
    char choice;
    cout << "¿Desea leer todo el archivo gcc.trace? (y/n): ";
    cin >> choice;

    if (choice == 'n' || choice == 'N')
    {
        cout << "Ingrese la cantidad de direcciones a leer: ";
        cin >> numAddresses;
    }

    // Generar el trace de memoria aleatorio
    vector<MemoryReference> memoryTrace;
    loadMemoryTrace(memoryTrace, numAddresses);

cout <<"\n";
cout << "            Tabla Resumen           \n\n"<<endl;
    // Realizar simulación para cada cantidad de frames
    for (int i = 0; i < sizeof(frames) / sizeof(frames[0]); i++)
    {
        int numFrames = frames[i];


        // Simulación FIFO
        std::cout << "\033[1;31mSimulación FIFO para \033[0m" << numFrames << "\033[1;31m frames:\033[0m " << std::endl;
        unordered_map<string, MemoryMapping> physicalMemoryMapFIFO;
        generatePhysicalMemoryMap(memoryTrace, physicalMemoryMapFIFO, numFrames);
        int pageFaultsFIFO = 0;
        int replacementsFIFO = 0;
        pageFaultsFIFO = simulatePageFaultsFIFO(memoryTrace, numFrames, replacementsFIFO);
        printSummary(physicalMemoryMapFIFO, memoryTrace, pageFaultsFIFO, numFrames, replacementsFIFO);
        cout << endl;

        // Simulación LRU
         std::cout << "\033[1;35mSimulación LRU para \033[0m" << numFrames << "\033[1;35m frames:\033[0m " << std::endl;
        unordered_map<string, MemoryMapping> physicalMemoryMapLRU;
        generatePhysicalMemoryMap(memoryTrace, physicalMemoryMapLRU, numFrames);
        int pageFaultsLRU = 0;
        int replacementsLRU = 0;
        pageFaultsLRU = simulatePageFaultsLRU(memoryTrace, numFrames, replacementsLRU);
        printSummary(physicalMemoryMapLRU, memoryTrace, pageFaultsLRU, numFrames, replacementsLRU);
        cout << endl;

        // Simulación OPT
       std::cout << " \033[1;33mSimulación OPT para \033[0m" << numFrames << "\033[1;33m frames:\033[0m " << std::endl;
        unordered_map<string, MemoryMapping> physicalMemoryMapOPT;
        generatePhysicalMemoryMap(memoryTrace, physicalMemoryMapOPT, numFrames);
        int pageFaultsOPT = 0;
        int replacementsOPT = 0;
        pageFaultsOPT = simulatePageFaultsOPT(memoryTrace, numFrames, replacementsOPT);
        printSummary(physicalMemoryMapOPT, memoryTrace, pageFaultsOPT, numFrames, replacementsOPT);
        cout << endl; }
    return 0;}
