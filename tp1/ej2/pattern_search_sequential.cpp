#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>

class PatternSearchSequential {
private:
    std::string texto;
    std::vector<std::string> patrones;
    std::vector<int> resultados;

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
        
        // Inicializar vector de resultados
        resultados.resize(patrones.size(), 0);
        
        return true;
    }
    
    int contarOcurrencias(const std::string& patron) {
        int count = 0;
        size_t pos = 0;
        
        while ((pos = texto.find(patron, pos)) != std::string::npos) {
            count++;
            pos++;  // Avanzar una posición para encontrar ocurrencias solapadas
        }
        
        return count;
    }
    
    void buscarPatrones() {
        std::cout << "\n=== BÚSQUEDA SECUENCIAL ===" << std::endl;
        
        auto inicio = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < patrones.size(); i++) {
            resultados[i] = contarOcurrencias(patrones[i]);
            std::cout << "Procesando patrón " << i << "..." << std::endl;
        }
        
        auto fin = std::chrono::high_resolution_clock::now();
        auto duracion = std::chrono::duration_cast<std::chrono::milliseconds>(fin - inicio);
        
        std::cout << "\nTiempo de ejecución secuencial: " << duracion.count() << " ms" << std::endl;
        std::cout << "Tiempo de ejecución secuencial: " << duracion.count() / 1000.0 << " segundos" << std::endl;
    }
    
    void mostrarResultados() {
        std::cout << "\n=== RESULTADOS SECUENCIAL ===" << std::endl;
        for (size_t i = 0; i < patrones.size(); i++) {
            std::cout << "el patron " << i << " aparece " << resultados[i] << " veces" << std::endl;
        }
    }
    
    std::vector<int> getResultados() const {
        return resultados;
    }
    
    double getTiempoEjecucion() {
        auto inicio = std::chrono::high_resolution_clock::now();
        buscarPatrones();
        auto fin = std::chrono::high_resolution_clock::now();
        auto duracion = std::chrono::duration_cast<std::chrono::milliseconds>(fin - inicio);
        return duracion.count() / 1000.0;
    }
};

int main() {
    PatternSearchSequential searcher;
    
    // Cargar archivos
    if (!searcher.cargarArchivos("texto.txt", "patrones.txt")) {
        return 1;
    }
    
    // Realizar búsqueda secuencial
    searcher.buscarPatrones();
    searcher.mostrarResultados();
    
    return 0;
}