#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

class PatternSearchMultithreaded {
private:
    std::string texto;
    std::vector<std::string> patrones;
    std::vector<std::atomic<int>> resultados;
    std::mutex outputMutex;

public:
    bool cargarArchivos(const std::string& archivoTexto, const std::string& archivoPatrones) {
        // Cargar archivo de texto
        std::ifstream fileTexto(archivoTexto, std::ios::binary);
        if (!fileTexto) {
            std::cerr << "Error: No se pudo abrir " << archivoTexto << std::endl;
            return false;
        }
        
        // Leer todo el contenido del archivo
        fileTexto.seekg(0, std::ios::end);
        size_t size = fileTexto.tellg();
        fileTexto.seekg(0, std::ios::beg);
        
        texto.resize(size);
        fileTexto.read(&texto[0], size);
        fileTexto.close();
        
        std::cout << "Archivo de texto cargado: " << size << " caracteres" << std::endl;
        
        // Cargar archivo de patrones
        std::ifstream filePatrones(archivoPatrones);
        if (!filePatrones) {
            std::cerr << "Error: No se pudo abrir " << archivoPatrones << std::endl;
            return false;
        }
        
        std::string patron;
        while (std::getline(filePatrones, patron)) {
            if (!patron.empty()) {
                patrones.push_back(patron);
            }
        }
        filePatrones.close();
        
        std::cout << "Patrones cargados: " << patrones.size() << std::endl;
        
        // Inicializar vector de resultados atómicos
        resultados.resize(patrones.size());
        for (auto& resultado : resultados) {
            resultado.store(0);
        }
        
        return true;
    }
    
    void buscarPatronEnHilo(int indicePatron) {
        const std::string& patron = patrones[indicePatron];
        int count = 0;
        size_t pos = 0;
        
        // Búsqueda del patrón
        while ((pos = texto.find(patron, pos)) != std::string::npos) {
            count++;
            pos++;  // Avanzar una posición para encontrar ocurrencias solapadas
        }
        
        // Almacenar resultado de forma thread-safe
        resultados[indicePatron].store(count);
        
        // Output thread-safe para mostrar progreso
        {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "Hilo " << indicePatron << " completado - Patrón: \"" 
                      << patron << "\" - Ocurrencias: " << count << std::endl;
        }
    }
    
    double buscarPatronesMultihilo() {
        std::cout << "\n=== BÚSQUEDA MULTIHILO ===" << std::endl;
        std::cout << "Número de hilos disponibles: " << std::thread::hardware_concurrency() << std::endl;
        
        auto inicio = std::chrono::high_resolution_clock::now();
        
        // Crear un hilo para cada patrón
        std::vector<std::thread> hilos;
        
        for (size_t i = 0; i < patrones.size(); i++) {
            hilos.emplace_back(&PatternSearchMultithreaded::buscarPatronEnHilo, this, i);
        }
        
        // Esperar a que todos los hilos terminen
        for (auto& hilo : hilos) {
            hilo.join();
        }
        
        auto fin = std::chrono::high_resolution_clock::now();
        auto duracion = std::chrono::duration_cast<std::chrono::milliseconds>(fin - inicio);
        
        double tiempoSegundos = duracion.count() / 1000.0;
        
        std::cout << "\nTiempo de ejecución multihilo: " << duracion.count() << " ms" << std::endl;
        std::cout << "Tiempo de ejecución multihilo: " << tiempoSegundos << " segundos" << std::endl;
        
        return tiempoSegundos;
    }
    
    void mostrarResultados() {
        std::cout << "\n=== RESULTADOS MULTIHILO ===" << std::endl;
        for (size_t i = 0; i < patrones.size(); i++) {
            std::cout << "el patron " << i << " aparece " << resultados[i].load() << " veces" << std::endl;
        }
    }
    
    std::vector<int> getResultados() {
        std::vector<int> res;
        for (const auto& resultado : resultados) {
            res.push_back(resultado.load());
        }
        return res;
    }
};

int main() {
    PatternSearchMultithreaded searcher;
    
    // Cargar archivos
    if (!searcher.cargarArchivos("texto.txt", "patrones.txt")) {
        return 1;
    }
    
    // Realizar búsqueda con multihilos
    double tiempoMultihilo = searcher.buscarPatronesMultihilo();
    searcher.mostrarResultados();
    
    return 0;
}