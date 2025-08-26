// matmul.cpp
#include <bits/stdc++.h>
#include <thread>
using namespace std;

using Clock = chrono::high_resolution_clock;
using ms    = chrono::duration<double, std::milli>;

struct Corners { float tl, tr, bl, br; };

Corners corners_of(const vector<float>& M, int N) {
    return { M[0], M[N-1], M[(size_t)(N-1)*N], M[(size_t)(N-1)*N + (N-1)] };
}

void print_corners(const string& name, const Corners& c) {
    cout << fixed << setprecision(6);
    cout << name << " esquinas => "
         << "TL=" << c.tl << "  TR=" << c.tr
         << "  BL=" << c.bl << "  BR=" << c.br << "\n";
}

double matmul_serial_sum(vector<float>& C,
                         const vector<float>& A,
                         const vector<float>& B,
                         int N)
{
    fill(C.begin(), C.end(), 0.0f);
    double total = 0.0;

    for (int i = 0; i < N; ++i) {
        float* __restrict c_row = C.data() + (size_t)i * N;
        for (int k = 0; k < N; ++k) {
            const float a = A[(size_t)i * N + k];
            const float* __restrict b_row = B.data() + (size_t)k * N;
            for (int j = 0; j < N; ++j) c_row[j] += a * b_row[j];
        }
        for (int j = 0; j < N; ++j) total += c_row[j];
    }
    return total;
}

void worker_block(const vector<float>& A, const vector<float>& B,
                  vector<float>& C, int N, int i0, int i1, double& partial_sum)
{
    double local_sum = 0.0;
    for (int i = i0; i < i1; ++i) {
        float* __restrict c_row = C.data() + (size_t)i * N;
        for (int j = 0; j < N; ++j) c_row[j] = 0.0f;

        for (int k = 0; k < N; ++k) {
            const float a = A[(size_t)i * N + k];
            const float* __restrict b_row = B.data() + (size_t)k * N;
            for (int j = 0; j < N; ++j) c_row[j] += a * b_row[j];
        }
        for (int j = 0; j < N; ++j) local_sum += c_row[j];
    }
    partial_sum = local_sum;
}

double matmul_parallel_sum(vector<float>& C,
                           const vector<float>& A,
                           const vector<float>& B,
                           int N, int num_threads)
{
    num_threads = max(1, min(num_threads, N));
    vector<thread> ths;
    vector<double> partial(num_threads, 0.0);

    int base = N / num_threads, rem = N % num_threads, start = 0;
    for (int t = 0; t < num_threads; ++t) {
        int rows = base + (t < rem ? 1 : 0);
        int i0 = start, i1 = i0 + rows; start = i1;
        ths.emplace_back(worker_block, cref(A), cref(B), ref(C), N, i0, i1, ref(partial[t]));
    }
    for (auto& th : ths) th.join();

    double total = 0.0;
    for (double s : partial) total += s;
    return total;
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc < 2) {
        cerr << "Uso: " << argv[0] << " N [hilos] [valorA] [valorB]\n";
        cerr << "Ej.: " << argv[0] << " 3000 16 0.1 0.2\n";
        return 1;
    }

    const int N = stoi(argv[1]);
    int hw = (int)thread::hardware_concurrency(); if (hw <= 0) hw = 8;
    int num_threads = (argc >= 3) ? stoi(argv[2]) : min(20, hw);
    float a_val = (argc >= 4) ? (float)atof(argv[3]) : 0.1f;
    float b_val = (argc >= 5) ? (float)atof(argv[4]) : 0.2f;

    cout << "N=" << N << "  hilos=" << num_threads
         << "  A=" << a_val << "  B=" << b_val << "\n";

    vector<float> A((size_t)N * N, a_val);
    vector<float> B((size_t)N * N, b_val);
    vector<float> C((size_t)N * N, 0.0f);

    print_corners("A", corners_of(A, N));
    print_corners("B", corners_of(B, N));

    auto wait_enter = [&](const string& msg){
        cout << msg; cout.flush();
        string line; getline(cin, line);
    };

    // ---- Pausa antes de ejecutar SECUENCIAL ----
    wait_enter("\nPresione ENTER para ejecutar la versión SECUENCIAL...");
    auto t0 = Clock::now();
    double sum_seq = matmul_serial_sum(C, A, B, N);
    auto t1 = Clock::now();
    double ms_seq = chrono::duration_cast<ms>(t1 - t0).count();

    print_corners("C (secuencial)", corners_of(C, N));
    cout << fixed << setprecision(6);
    cout << "Sumatoria (secuencial) = " << sum_seq << "\n";
    cout << "Tiempo secuencial: " << ms_seq/1000.0 << " s (" << ms_seq << " ms)\n";

    // ---- Pausa antes de ejecutar MULTIHILO ----
    wait_enter("\nPresione ENTER para ejecutar la versión MULTIHILO...");
    auto t2 = Clock::now();
    double sum_par = matmul_parallel_sum(C, A, B, N, num_threads);
    auto t3 = Clock::now();
    double ms_par = chrono::duration_cast<ms>(t3 - t2).count();

    print_corners("C (multihilo)", corners_of(C, N));
    cout << "Sumatoria (multihilo) = " << sum_par << "\n";
    cout << "Tiempo multihilo: " << ms_par/1000.0 << " s (" << ms_par << " ms)\n";

    double rel_diff = fabs(sum_seq - sum_par) / max(1.0, fabs(sum_seq));
    if (rel_diff > 1e-5) {
        cout << "ADVERTENCIA: diferencia relativa entre sumatorias = " << rel_diff << "\n";
    }

    if (ms_par > 0.0) {
        double speedup = ms_seq / ms_par;
        cout << "Speedup = " << speedup << "x\n";
    }

    double esperado = (double)N * N * (double)(N * a_val * b_val);
    cout << "\n(Referencia teórica con matrices constantes) "
         << "elemento C[i,j]≈" << (N * a_val * b_val)
         << "  sumatoria≈" << esperado << "\n";

    return 0;
}
