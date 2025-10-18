#include "ELnettle.h"

#include <cstring>
#include <stdexcept>

size_t ELnettle::prepare(Algorithm alg_) {
    size_t keySize = 0;
    alg = alg_;

    switch(alg) {
        case Algorithm::aes256:
            keySize = AES256_KEY_SIZE;
            break;
        case Algorithm::chacha20:
            keySize = CHACHA_KEY_SIZE;
            nonce.resize(CHACHA_NONCE96_SIZE);
            chacha_set_nonce96(&chachactx, nonce.data());
            break;
        default:
            throw std::runtime_error{"Nettle is not set up with support for the desired algorithm"};
            break;
    }


    return keySize;
}

void ELnettle::setKey(void* data, size_t keyLen) {
    key.resize(keyLen);
    std::memcpy(key.data(), data, keyLen);

    switch(alg) {
        case Algorithm::chacha20:
            chacha_set_key(&chachactx, key.data());
            break;
        default:
            break;
    }
}

void ELnettle::encrypt(void* source, size_t sourceSize, void* dest, size_t destSize) {
    switch(alg) {
        case Algorithm::aes256:
            aes256_set_encrypt_key(&aes256ctx, key.data());
            aes256_encrypt(&aes256ctx, sourceSize,
                    static_cast<std::uint8_t*>(dest),
                    static_cast<std::uint8_t*>(source));
            break;
        case Algorithm::chacha20:
            chacha_crypt32(&chachactx, sourceSize, 
                    static_cast<std::uint8_t*>(dest),
                    static_cast<std::uint8_t*>(source));
            break;
        default:
            throw std::runtime_error{"Tried to use nettle with an unsupported algorithm"};
            break;
    }
}

void ELnettle::decrypt(void* source, size_t sourceSize, void* dest, size_t destSize) {
    switch(alg) {
        case Algorithm::aes256:
            aes256_set_decrypt_key(&aes256ctx, key.data());
            aes256_decrypt(&aes256ctx, sourceSize,
                    static_cast<std::uint8_t*>(dest),
                    static_cast<std::uint8_t*>(source));
            break;
        case Algorithm::chacha20:
            chacha_crypt32(&chachactx, sourceSize, 
                    static_cast<std::uint8_t*>(dest),
                    static_cast<std::uint8_t*>(source));
            break;
        default:
            throw std::runtime_error{"Tried to use nettle with an unsupported algorithm"};
            break;
    }
}

static void reset_(Algorithm alg, void* ctx, std::uint8_t* nonce) {
    switch(alg) {
        case Algorithm::chacha20:
            // reset nonce
            chacha_set_nonce96(static_cast<chacha_ctx*>(ctx), nonce);
            break;
        default:
            break;
    }
}

void ELnettle::reset() {
    reset_(alg, &chachactx, nonce.data());
};

void ELnettle::reset(const std::string& nonce_) {
    if(nonce.size() != nonce_.size()) {
        throw std::runtime_error{"ERROR: ELnettle::reset nonce is the wrong size"};
    }
    std::memcpy(nonce.data(), nonce_.data(), nonce.size());

    reset_(alg, &chachactx, nonce.data());
}

std::string ELnettle::getNonce() {
    std::string ret;
    ret.resize(nonce.size());
    std::memcpy(ret.data(), nonce.data(), nonce.size());
    return ret;
}
