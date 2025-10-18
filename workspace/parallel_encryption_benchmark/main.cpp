#include <string>
#include <fstream>
#include <memory>
#include <chrono>
#include <print>

#include <mpi.h>

#include <EncryptionLibrary.h>
#include <ELgcrypt.h>


constexpr std::size_t CHUNK_SIZE{4096};
constexpr std::size_t FILE_SIZE{17179869184}; // 16 gigs

class Timer {
public:
    void reset() {
        start = std::chrono::high_resolution_clock::now();
    }
    double getElapsed() {
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - start);
        return elapsed.count();
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

int main(int argc, char** argv) {
    MPI_Init(NULL, NULL);
    int process_count;
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    const std::size_t rankSize = FILE_SIZE / process_count;

    // prepare encryption
    std::unique_ptr<EncryptionLibrary> el{};
    el = std::make_unique<ELgcrypt>();
    size_t keySize = el->prepare(Algorithm::aes256);
    std::string key = EncryptionLibrary::MakeKey(keySize);
    el->setKey(key.data(), key.size());
    const std::string nonce{el->getNonce()};

    // prepare data
    std::string chunk(CHUNK_SIZE, '\0');
    std::ofstream outFile{"outfile", std::ios::out | std::ios::trunc | std::ios::binary };
    outFile.seekp((nonce.size() + rankSize) * my_rank);

    // time encryption
    Timer t;
    double encryptionTime = 0.0;
    double writeTime = 0.0;
    const std::size_t writeCount = rankSize / CHUNK_SIZE;

    Timer totalTimer;
    totalTimer.reset();

    t.reset();
    outFile << nonce;
    writeTime += t.getElapsed();

    for(int writeNum = 0; writeNum != writeCount; ++writeNum) {
        t.reset();
        std::string crypt;
        crypt.resize(CHUNK_SIZE);
        el->encrypt(chunk.data(), chunk.size(), crypt.data(), crypt.size());
        encryptionTime += t.getElapsed();

        t.reset();
        outFile << chunk;
        writeTime += t.getElapsed();
     }

    outFile.close();
    MPI_Barrier(MPI_COMM_WORLD);
    double totalElapsed = totalTimer.getElapsed();

    // report timing
    if(my_rank == 0) {
        std::println("Encryption Time: {} s", encryptionTime / (1000 * 1000 * 1000));
        std::println("Write Time: {} s", writeTime / (1000 * 1000 * 1000));
        std::println("Total Time: {} s", totalElapsed / (1000 * 1000 * 1000));
        std::println("Total Nonce Size: {} KiB", (nonce.size() * process_count) / 1000.0);
    }

    // Finalize the MPI environment.
    MPI_Finalize();
}
