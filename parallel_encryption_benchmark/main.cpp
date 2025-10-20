#include <string>
#include <fstream>
#include <memory>
#include <chrono>
#include <iostream>

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
    bool shared = false;
    if(argc == 2) {
        std::string arg{argv[1]};
        if(arg == "-s" || arg == "--shared") {
            shared = true;
        }
    }

    MPI_Init(NULL, NULL);
    int process_count;
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    const std::size_t rankSize = FILE_SIZE / process_count;
    const std::size_t writeCount = rankSize / CHUNK_SIZE;

    if(my_rank == 0) std::cout << "Shared: " << shared << '\n';


    // prepare encryption
    std::unique_ptr<EncryptionLibrary> el{};
    el = std::make_unique<ELgcrypt>();
    el->prepare(Algorithm::aes256);
    std::string key = el->makeKey();
    el->setKey(key.data(), key.size());

    MPI_Barrier(MPI_COMM_WORLD);
    Timer totalTimer;
    totalTimer.reset();

    Timer nonceGenTimer;
    nonceGenTimer.reset();
    std::string nonce;
    if(!shared) {
        nonce = el->makeNonce();
        el->setNonce(nonce.data(), nonce.size());
    } else {
        if(my_rank == 0) {
            std::string nonce = el->makeNonce();
        }
        nonce.resize(el->getNonceSize());

        MPI_Bcast(nonce.data(), nonce.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
        // update nonce to be accurate
    }
    MPI_Barrier(MPI_COMM_WORLD);
    double nonceGenTime = nonceGenTimer.getElapsed();
    
    // prepare data
    std::string chunk(CHUNK_SIZE, '\0');
    MPI_File fh;
    MPI_File_open(MPI_COMM_WORLD, "outfile",
        MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fh);
    MPI_Offset offset = 0;
    // if shared and not rank 0, only append first nonce
    if(shared && my_rank != 0) {
        offset = static_cast<MPI_Offset>((my_rank * rankSize) + nonce.size());
    }
    // always append nonce if not shared
    else {
        offset = static_cast<MPI_Offset>(my_rank * (rankSize + nonce.size()));
    }
    MPI_File_set_view(fh, offset, MPI_BYTE, MPI_BYTE, "native", MPI_INFO_NULL);

    // time encryption
    Timer t;
    double encryptionTime = 0.0;
    double writeTime = 0.0;

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Status status;
    t.reset();
    // append if not shared, or if first rank
    if(!shared) {
        MPI_File_write_all(fh, nonce.data(), nonce.size(), MPI_BYTE, &status);
    } else if (my_rank == 0) {
        MPI_File_write(fh, nonce.data(), nonce.size(), MPI_BYTE, &status);
    }
    writeTime += t.getElapsed();

    for(int writeNum = 0; writeNum != writeCount; ++writeNum) {
        t.reset();
        std::string crypt;
        crypt.resize(CHUNK_SIZE);
        el->encrypt(chunk.data(), chunk.size(), crypt.data(), crypt.size());
        encryptionTime += t.getElapsed();

        t.reset();
        MPI_File_write_all(fh, crypt.data(), crypt.size(), MPI_BYTE, &status);
        writeTime += t.getElapsed();
     }

    MPI_File_close(&fh);
    
    MPI_Barrier(MPI_COMM_WORLD);
    double totalElapsed = totalTimer.getElapsed();

    // report timing
    if(my_rank == 0) {
        double encryptionTimeS = encryptionTime / (1000 * 1000 * 1000);
        double writeTimeS = writeTime / (1000 * 1000 * 1000);
        double totalElapsedS = totalElapsed / (1000 * 1000 * 1000);
        double nonceGenTimeS = nonceGenTime / (1000 * 1000 * 1000);
        double nonceSize = nonce.size() / 1000.0;
        if(!shared) {
            nonceSize *= process_count;
        }

        std::cout << "Encryption Time: " << encryptionTimeS << "s\n";
        std::cout << "Write Time: " << writeTimeS << "s\n";
        std::cout << "Total Time: " << totalElapsedS << "s\n";
        std::cout << "Nonce Generation Time: " << nonceGenTimeS << "s\n";
        std::cout << "Total Nonce Size: " << nonceSize << " KiB\n";
        
        std::ofstream logFile{"results.csv"};
        logFile << "metric, value\n";
        logFile << "encryption, " << encryptionTimeS << '\n';
        logFile << "write, " << writeTimeS << '\n';
        logFile << "total, " << totalElapsedS << '\n';
        logFile << "nonceTime, " << nonceGenTimeS << '\n';
        logFile << "nonceSize, " << nonceSize << '\n';
    }

    // Finalize the MPI environment.
    MPI_Finalize();
}
