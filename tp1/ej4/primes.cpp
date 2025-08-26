// primes_var.cpp
#include <bits/stdc++.h>
#include <thread>
#include <atomic>
using namespace std;

using Clock = chrono::high_resolution_clock;
using ms    = chrono::duration<double, std::milli>;

static inline void wait_enter(const string& msg) {
    cout << msg; cout.flush();
    string line; getline(cin, line);
}

struct PrimeStats {
    unsigned long long count = 0;
    vector<long long> top10_desc; // 10 mayores (desc)
};

static vector<int> sieve_base(long long limit) {
    vector<uint8_t> is_prime(limit + 1, 1);
    if (limit >= 0) is_prime[0] = 0;
    if (limit >= 1) is_prime[1] = 0;
    for (long long p = 2; p * p <= limit; ++p) {
        if (is_prime[(size_t)p]) {
            for (long long j = p * p; j <= limit; j += p) is_prime[(size_t)j] = 0;
        }
    }
    vector<int> primes;
    for (long long i = 2; i <= limit; ++i) if (is_prime[(size_t)i]) primes.push_back((int)i);
    return primes;
}

// ----------------- SECUENCIAL -----------------
static PrimeStats primes_sequential(long long N) {
    vector<uint8_t> is_prime((size_t)N, 1);
    if (N > 0) is_prime[0] = 0;
    if (N > 1) is_prime[1] = 0;

    for (long long p = 2; p * p < N; ++p) {
        if (is_prime[(size_t)p]) {
            for (long long j = p * p; j < N; j += p) is_prime[(size_t)j] = 0;
        }
    }

    PrimeStats st;
    for (long long i = 2; i < N; ++i) if (is_prime[(size_t)i]) ++st.count;

    for (long long i = N - 1; i >= 2 && (int)st.top10_desc.size() < 10; --i) {
        if (is_prime[(size_t)i]) st.top10_desc.push_back(i);
    }
    return st;
}

// ----------------- MULTIHILO: INTERVALOS VARIABLES + COLA DINÁMICA -----------------
struct ThreadResult {
    unsigned long long count = 0;
    vector<long long> tails; // colecciona hasta 10 de cada bloque
};

// Tamaño de bloque “creciente” con la posición (no constante).
static inline long long tile_len_for(long long lo, long long end, long long total) {
    // f ∈ [0,1]: progreso del rango
    double f = (total > 0) ? double(lo - 2) / double(total) : 0.0;
    // Límites de tamaño de bloque (en cantidad de números)
    const long long MIN_TILE = 1LL << 18; // 262,144
    const long long MAX_TILE = 1LL << 21; // 2,097,152
    long long len = (long long)(MIN_TILE + (MAX_TILE - MIN_TILE) * f);
    if (len < MIN_TILE) len = MIN_TILE;
    if (len > MAX_TILE) len = MAX_TILE;
    // Ajuste si nos pasamos del final
    if (lo + len - 1 > end) len = end - lo + 1;
    return max(0LL, len);
}

static bool claim_next_chunk(atomic<long long>& cursor, long long end, long long total,
                             long long& lo, long long& hi)
{
    while (true) {
        long long cur = cursor.load(memory_order_relaxed);
        if (cur > end) return false;

        long long len = tile_len_for(cur, end, total);
        if (len <= 0) return false;

        long long next = cur + len;
        if (cursor.compare_exchange_weak(cur, next, memory_order_acq_rel, memory_order_relaxed)) {
            lo = cur;
            hi = cur + len - 1;
            return true;
        }
        // Si falla el CAS, reintentar.
    }
}

static void sieve_segment(long long lo, long long hi,
                          const vector<int>& base_primes,
                          unsigned long long& count_out,
                          vector<long long>& tail_out)
{
    const size_t L = (size_t)(hi - lo + 1);
    vector<uint8_t> seg(L, 1);
    if (lo == 0) seg[0] = 0;
    if (lo <= 1 && 1 <= hi) seg[(size_t)(1 - lo)] = 0;

    for (int p : base_primes) {
        long long pp = 1LL * p * p;
        if (pp > hi) break;
        long long start = max(pp, ((lo + p - 1) / (long long)p) * (long long)p);
        for (long long j = start; j <= hi; j += p) seg[(size_t)(j - lo)] = 0;
    }

    unsigned long long cnt = 0;
    for (size_t i = 0; i < L; ++i) if (seg[i]) ++cnt;
    count_out = cnt;

    // Guardar hasta 10 mayores del bloque
    for (long long j = hi; j >= lo && (int)tail_out.size() < 10; --j) {
        if (seg[(size_t_]()
