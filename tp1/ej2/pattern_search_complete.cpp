#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <iomanip>

class PatternSearchComplete {
private:
    std::string texto;
    std::vector<std::string> patrones;
    std::mutex outputMutex;

    // Función auxiliar para esperar input del usuario
    void esperarInput(const std::string& mensaje) {
        std::cout << "\n" << mensaje << std::endl;
        std::cout << "Presiona ENTER para continuar...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::getchar();
    }

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
    
    std::pair<std::vector<int>, double> busquedaSecuencial() {
        std::cout << "\n=== BÚSQUEDA SECUENCIAL ===" << std::endl;
        std::cout << "Este método procesará cada patrón uno por uno en un solo hilo." << std::endl;
        
        esperarInput("¿Listo para ejecutar la búsqueda secuencial?");
        
        std::vector<int> resultados(patrones.size());
        
        auto inicio = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < patrones.size(); i++) {
            resultados[i] = contarOcurrencias(patrones[i]);
            std::cout << "Procesando patrón " << (i + 1) << " de " << patrones.size() 
                      << " secuencialmente..." << std::endl;
        }
        
        auto fin = std::chrono::high_resolution_clock::now();
        auto duracion = std::chrono::duration_cast<std::chrono::milliseconds>(fin - inicio);
        
        double tiempoSegundos = duracion.count() / 1000.0;
        
        std::cout << "\n✓ BÚSQUEDA SECUENCIAL COMPLETADA" << std::endl;
        std::cout << "Tiempo de ejecución secuencial: " << duracion.count() << " ms" << std::endl;
        std::cout << "Tiempo de ejecución secuencial: " << std::fixed << std::setprecision(3) 
                  << tiempoSegundos << " segundos" << std::endl;
        
        return {resultados, tiempoSegundos};
    }
    
    void buscarPatronEnHilo(int indicePatron, std::vector<std::atomic<int>>& resultados) {
        const std::string& patron = patrones[indicePatron];
        int count = contarOcurrencias(patron);
        
        resultados[indicePatron].store(count);
        
        // Output thread-safe para mostrar progreso
        {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "Hilo " << indicePatron << " completado - Patrón: \"" 
                      << patron.substr(0, std::min(20, (int)patron.length())) 
                      << (patron.length() > 20 ? "..." : "") 
                      << "\" - Ocurrencias: " << count << std::endl;
        }
    }
    
    std::pair<std::vector<int>, double> busquedaMultihilo() {
        std::cout << "\n=== BÚSQUEDA MULTIHILO ===" << std::endl;
        std::cout << "Este método procesará todos los patrones simultáneamente usando múltiples hilos." << std::endl;
        std::cout << "Número de hilos disponibles en el sistema: " << std::thread::hardware_concurrency() << std::endl;
        std::cout << "Número de hilos que se usarán: " << patrones.size() << " (uno por patrón)" << std::endl;
        
        esperarInput("¿Listo para ejecutar la búsqueda multihilo?");
        
        std::vector<std::atomic<int>> resultadosAtomicos(patrones.size());
        for (auto& resultado : resultadosAtomicos) {
            resultado.store(0);
        }
        
        auto inicio = std::chrono::high_resolution_clock::now();
        
        // Crear un hilo para cada patrón
        std::vector<std::thread> hilos;
        
        for (size_t i = 0; i < patrones.size(); i++) {
            hilos.emplace_back(&PatternSearchComplete::buscarPatronEnHilo, this, i, std::ref(resultadosAtomicos));
        }
        
        // Esperar a que todos los hilos terminen
        for (auto& hilo : hilos) {
            hilo.join();
        }
        
        auto fin = std::chrono::high_resolution_clock::now();
        auto duracion = std::chrono::duration_cast<std::chrono::milliseconds>(fin - inicio);
        
        double tiempoSegundos = duracion.count() / 1000.0;
        
        std::cout << "\n✓ BÚSQUEDA MULTIHILO COMPLETADA" << std::endl;
        std::cout << "Tiempo de ejecución multihilo: " << duracion.count() << " ms" << std::endl;
        std::cout << "Tiempo de ejecución multihilo: " << std::fixed << std::setprecision(3) 
                  << tiempoSegundos << " segundos" << std::endl;
        
        // Convertir resultados atómicos a vector normal
        std::vector<int> resultados;
        for (const auto& resultado : resultadosAtomicos) {
            resultados.push_back(resultado.load());
        }
        
        return {resultados, tiempoSegundos};
    }
    
    void mostrarResultados(const std::vector<int>& resultados, const std::string& titulo) {
        std::cout << "\n=== " << titulo << " ===" << std::endl;
        for (size_t i = 0; i < resultados.size(); i++) {
            std::cout << "El patrón " << (i + 1) << " aparece " << resultados[i] << " veces" << std::endl;
        }
    }
    
    void calcularSpeedup(double tiempoSecuencial, double tiempoMultihilo) {
        double speedup = tiempoSecuencial / tiempoMultihilo;
        double eficiencia = speedup / patrones.size();
        
        std::cout << "\n=== ANÁLISIS DE RENDIMIENTO ===" << std::endl;
        std::cout << "Tiempo secuencial: " << std::fixed << std::setprecision(3) 
                  << tiempoSecuencial << " segundos" << std::endl;
        std::cout << "Tiempo multihilo:  " << std::fixed << std::setprecision(3) 
                  << tiempoMultihilo << " segundos" << std::endl;
        std::cout << "Speedup: " << std::fixed << std::setprecision(3) << speedup << "x" << std::endl;
        std::cout << "Eficiencia: " << std::fixed << std::setprecision(3) 
                  << (eficiencia * 100) << "%" << std::endl;
        std::cout << "Número de hilos utilizados: " << patrones.size() << std::endl;
        std::cout << "Mejora de rendimiento: " << std::fixed << std::setprecision(1) 
                  << ((speedup - 1) * 100) << "%" << std::endl;
        
        if (speedup > 1) {
            std::cout << "✓ La paralelización fue efectiva" << std::endl;
        } else {
            std::cout << "✗ La paralelización no mejoró el rendimiento" << std::endl;
        }
    }
    
    bool verificarResultados(const std::vector<int>& res1, const std::vector<int>& res2) {
        if (res1.size() != res2.size()) return false;
        
        for (size_t i = 0; i < res1.size(); i++) {
            if (res1[i] != res2[i]) {
                std::cout << "Diferencia en patrón " << (i + 1) << ": secuencial=" 
                          << res1[i] << ", multihilo=" << res2[i] << std::endl;
                return false;
            }
        }
        return true;
    }
    
    void mostrarMenu() {
        std::cout << "\n=== MENÚ DE OPCIONES ===" << std::endl;
        std::cout << "1. Ejecutar búsqueda secuencial" << std::endl;
        std::cout << "2. Ejecutar búsqueda multihilo" << std::endl;
        std::cout << "3. Ejecutar ambas y comparar" << std::endl;
        std::cout << "4. Salir" << std::endl;
        std::cout << "Selecciona una opción (1-4): ";
    }
    
    void ejecutarInteractivo() {
        int opcion;
        std::vector<int> resultadosSecuencial, resultadosMultihilo;
        double tiempoSecuencial = 0, tiempoMultihilo = 0;
        bool secuencialEjecutado = false, multihiloEjecutado = false;
        
        while (true) {
            mostrarMenu();
            std::cin >> opcion;
            
            switch (opcion) {
                case 1: {
                    auto [resultados, tiempo] = busquedaSecuencial();
                    resultadosSecuencial = resultados;
                    tiempoSecuencial = tiempo;
                    secuencialEjecutado = true;
                    mostrarResultados(resultados, "RESULTADOS BÚSQUEDA SECUENCIAL");
                    break;
                }
                case 2: {
                    auto [resultados, tiempo] = busquedaMultihilo();
                    resultadosMultihilo = resultados;
                    tiempoMultihilo = tiempo;
                    multihiloEjecutado = true;
                    mostrarResultados(resultados, "RESULTADOS BÚSQUEDA MULTIHILO");
                    break;
                }
                case 3: {
                    std::cout << "\nEjecutando comparación completa..." << std::endl;
                    
                    auto [resSeq, tiempoSeq] = busquedaSecuencial();
                    auto [resMul, tiempoMul] = busquedaMultihilo();
                    
                    resultadosSecuencial = resSeq;
                    resultadosMultihilo = resMul;
                    tiempoSecuencial = tiempoSeq;
                    tiempoMultihilo = tiempoMul;
                    secuencialEjecutado = true;
                    multihiloEjecutado = true;
                    
                    // Verificar resultados
                    if (verificarResultados(resultadosSecuencial, resultadosMultihilo)) {
                        std::cout << "\n✓ Verificación: Ambas implementaciones producen resultados idénticos" << std::endl;
                    } else {
                        std::cout << "\n✗ Error: Las implementaciones producen resultados diferentes" << std::endl;
                    }
                    
                    mostrarResultados(resultadosSecuencial, "RESULTADOS FINALES");
                    calcularSpeedup(tiempoSecuencial, tiempoMultihilo);
                    break;
                }
                case 4:
                    std::cout << "Saliendo del programa..." << std::endl;
                    return;
                default:
                    std::cout << "Opción inválida. Por favor selecciona 1-4." << std::endl;
                    break;
            }
            
            // Si ambos métodos han sido ejecutados, mostrar comparación
            if (secuencialEjecutado && multihiloEjecutado && opcion != 3) {
                std::cout << "\n¿Deseas ver la comparación de rendimiento? (s/n): ";
                char respuesta;
                std::cin >> respuesta;
                if (respuesta == 's' || respuesta == 'S') {
                    if (verificarResultados(resultadosSecuencial, resultadosMultihilo)) {
                        std::cout << "\n✓ Verificación: Ambas implementaciones producen resultados idénticos" << std::endl;
                    }
                    calcularSpeedup(tiempoSecuencial, tiempoMultihilo);
                }
            }
        }
    }
};

int main() {
    PatternSearchComplete searcher;
    
    std::cout << "=== BÚSQUEDA DE PATRONES EN TEXTO ===" << std::endl;
    std::cout << "Comparación entre implementación secuencial y multihilo" << std::endl;
    std::cout << "Versión interactiva con control de usuario" << std::endl;
    
    // Cargar archivos
    if (!searcher.cargarArchivos("texto.txt", "patrones.txt")) {
        std::cerr << "\nError al cargar los archivos. Asegúrate de que:" << std::endl;
        std::cerr << "1. El archivo 'texto.txt' existe en el directorio actual" << std::endl;
        std::cerr << "2. El archivo 'patrones.txt' existe en el directorio actual" << std::endl;
        std::cerr << "\nTambién puedes crear archivos de prueba:" << std::endl;
        std::cerr << "- texto.txt: Un archivo con texto para buscar" << std::endl;
        std::cerr << "- patrones.txt: Un archivo con patrones a buscar (uno por línea)" << std::endl;
        return 1;
    }
    
    std::cout << "\nArchivos cargados exitosamente." << std::endl;
    std::cout << "\nPara monitorear el uso de CPU durante la ejecución:" << std::endl;
    std::cout << "- En Linux: usar 'htop' o 'top' en otra terminal" << std::endl;
    std::cout << "- En Windows: usar el Administrador de tareas" << std::endl;
    std::cout << "- En macOS: usar 'Activity Monitor'" << std::endl;
    
    // Ejecutar menú interactivo
    searcher.ejecutarInteractivo();
    
    return 0;
}