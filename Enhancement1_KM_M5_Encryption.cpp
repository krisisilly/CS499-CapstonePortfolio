// Encryption.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <ctime>
#include <vector>
#include <string>
#include <optional>
#include <stdexcept>
#include <array>
#include <cstdint>
#include <string_view>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <iterator>

namespace sc {
    // error helper
    [[noreturn]] void die(const std::string& msg) { throw std::runtime_error(msg); }

    // utility
    std::vector<std::uint8_t> random_bytes(std::size_t n) {
        std::vector<std::uint8_t> out(n);
        if (n && RAND_bytes(out.data(), (int)n) != 1) die("RAND_bytes failed");
        return out;
    }

    std::vector<std::uint8_t> read_all_bytes(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) die("Failed to open for read: " + path);
        f.seekg(0, std::ios::end);
        auto sz = f.tellg();
        if (sz < 0) die("tellg failed");
        std::vector<std::uint8_t> buf((size_t)sz);
        f.seekg(0, std::ios::beg);
        if (sz > 0) f.read((char*)buf.data(), (std::streamsize)sz);
        if (!f) die("Read failed: " + path);
        return buf;
    }

    void write_all_bytes(const std::string& path, const std::vector<std::uint8_t>& data) {
        std::ofstream f(path, std::ios::binary);
        if (!f) die("Failed to open for write: " + path);
        if (!data.empty()) f.write((const char*)data.data(), (std::streamsize)data.size());
        if (!f) die("Write failed: " + path);
    }

    struct KdfParams {
        std::vector<std::uint8_t> salt; std::uint32_t iterations = 200000;
    };

    std::vector<std::uint8_t>pbkdf2(std::string_view pass, const KdfParams& p) {
        if (p.salt.size() != 16) die("Salt must be 16 bytes");
        std::vector<std::uint8_t> key(32);
        if (PKCS5_PBKDF2_HMAC(pass.data(), (int)pass.size(), p.salt.data(), (int)p.salt.size(),
            (int)p.iterations, EVP_sha256(), (int)key.size(), key.data()) != 1) {
            die("PBKDF2 failed");
        }
        return key;
    }

    struct EncResult {
        std::vector<std::uint8_t> nonce;
        std::vector<std::uint8_t> tag;
        std::vector<std::uint8_t> ciphertext;
        KdfParams kdf;
    };

    EncResult encrypt_aead(const std::vector<std::uint8_t>& plaintext,
        std::string_view passphrase,
        const std::optional<std::vector<std::uint8_t>>& aad,
        std::uint32_t iterations) {
        EncResult out;
        out.kdf.salt = random_bytes(16);
        out.kdf.iterations = iterations;
        out.nonce = random_bytes(12);

        auto key = pbkdf2(passphrase, out.kdf);

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) die("EVP_CIPHER_CTX_new failed");

        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) die("EncryptInit failed");
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, (int)out.nonce.size(), nullptr) != 1) die("Set IV len failed");
        if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), out.nonce.data()) != 1) die("EncryptInit key/iv failed");

        int len = 0;
        if (aad && !aad->empty()) {
            if (EVP_EncryptUpdate(ctx, nullptr, &len, aad->data(), (int)aad->size()) != 1) die("AAD update failed");
        }

        out.ciphertext.resize(plaintext.size());
        int outlen = 0;
        if (EVP_EncryptUpdate(ctx, out.ciphertext.data(), &outlen, plaintext.data(), (int)plaintext.size()) != 1) die("EncryptUpdate failed");
        int total = outlen;

        if (EVP_EncryptFinal_ex(ctx, out.ciphertext.data() + total, &outlen) != 1) die("EncryptFinal failed");
        total += outlen;
        out.ciphertext.resize((size_t)total);

        out.tag.resize(16);
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, (int)out.tag.size(), out.tag.data()) != 1) die("Get GCM tag failed");

        EVP_CIPHER_CTX_free(ctx);
        return out;
    }

    std::vector<std::uint8_t> decrypt_aead(const EncResult& enc,
        std::string_view passphrase,
        const std::optional<std::vector<std::uint8_t>>& aad) {
        auto key = pbkdf2(passphrase, enc.kdf);

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) die("EVP_CIPHER_CTX_new failed");

        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) die("DecryptInit failed");
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, (int)enc.nonce.size(), nullptr) != 1) die("Set IV len failed");
        if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), enc.nonce.data()) != 1) die("DecryptInit key/iv failed");

        int len = 0;
        if (aad && !aad->empty()) {
            if (EVP_DecryptUpdate(ctx, nullptr, &len, aad->data(), (int)aad->size()) != 1) die("AAD update failed");
        }

        std::vector<std::uint8_t> plain(enc.ciphertext.size());
        int outlen = 0;
        if (EVP_DecryptUpdate(ctx, plain.data(), &outlen, enc.ciphertext.data(), (int)enc.ciphertext.size()) != 1) die("DecryptUpdate failed");
        int total = outlen;

        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, (int)enc.tag.size(), (void*)enc.tag.data()) != 1) die("Set GCM tag failed");

        if (EVP_DecryptFinal_ex(ctx, plain.data() + total, &outlen) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            die("Authentication failed (wrong passphrase or tampered data)");
        }
        total += outlen;
        plain.resize((size_t)total);

        EVP_CIPHER_CTX_free(ctx);
        return plain;
    }

    static void write_u32_be(std::ofstream& f, std::uint32_t v) {
        std::array<unsigned char, 4> b{
            (unsigned char)((v >> 24) & 0xFF),
            (unsigned char)((v >> 16) & 0xFF),
            (unsigned char)((v >> 8) & 0xFF),
            (unsigned char)(v & 0xFF)
        };
        f.write((const char*)b.data(), 4);
    }

    static std::uint32_t read_u32_be(std::ifstream& f) {
        std::array<unsigned char, 4> b{};
        f.read((char*)b.data(), 4);
        if (!f) die("Unexpected EOF (iterations)");
        return (std::uint32_t(b[0]) << 24) | (std::uint32_t(b[1]) << 16) | (std::uint32_t(b[2]) << 8) | std::uint32_t(b[3]);
    }

    void write_container(const std::string& path, const EncResult& enc) {
        std::ofstream f(path, std::ios::binary);
        if (!f) die("Failed to open for write: " + path);
        const char magic[4] = { 'S','C','F','1' };
        f.write(magic, 4);
        if (enc.kdf.salt.size() != 16) die("Salt must be 16 bytes");
        f.write((const char*)enc.kdf.salt.data(), 16);
        write_u32_be(f, enc.kdf.iterations);
        if (enc.nonce.size() != 12) die("Nonce must be 12 bytes");
        f.write((const char*)enc.nonce.data(), 12);
        if (enc.tag.size() != 16) die("Tag must be 16 bytes");
        f.write((const char*)enc.tag.data(), 16);
        if (!enc.ciphertext.empty()) f.write((const char*)enc.ciphertext.data(), (std::streamsize)enc.ciphertext.size());
        if (!f) die("Write container failed: " + path);
    }

    EncResult read_container(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) die("Failed to open for read: " + path);
        char magic[4]; f.read(magic, 4);
        if (!f || std::string(magic, magic + 4) != "SCF1") die("Invalid container magic");
        EncResult enc;
        enc.kdf.salt.resize(16); f.read((char*)enc.kdf.salt.data(), 16); if (!f) die("EOF salt");
        enc.kdf.iterations = read_u32_be(f);
        enc.nonce.resize(12);    f.read((char*)enc.nonce.data(), 12);    if (!f) die("EOF nonce");
        enc.tag.resize(16);      f.read((char*)enc.tag.data(), 16);      if (!f) die("EOF tag");
        std::vector<char> rest((std::istreambuf_iterator<char>(f)), {});
        enc.ciphertext.assign(rest.begin(), rest.end());
        return enc;
    }

}

std::string read_file(const std::string& filename)
{
    // TODO: implement loading the file into a string
    std::ifstream file(filename);
    std::stringstream buffer;

    if (file)
    {
        buffer << file.rdbuf(); // this will read the file to buffer.
        file.close();
    }
    else
    {
        std::cerr << "Error reading the file: " << filename << std::endl;
    }

    return buffer.str(); // returns file content. 
}

std::string get_student_name(const std::string& string_data)
{
    std::string student_name;

    // find the first newline
    size_t pos = string_data.find('\n');
    // did we find a newline
    if (pos != std::string::npos)
    { // we did, so copy that substring as the student name
        student_name = string_data.substr(0, pos);
    }

    return student_name;
}

void save_data_file(const std::string& filename, const std::string& student_name, const std::string& /*key*/, const std::string& data)
{
    //  TODO: implement file saving
    //  file format
    std::ofstream file(filename);
    if (file)
    {
        //  Line 1: student name
        file << student_name << '\n';
        //  Line 2: timestamp (yyyy-mm-dd)
        std::time_t t = std::time(nullptr);
        std::tm tm;
        localtime_s(&tm, &t);
        file << std::put_time(&tm, "%Y-%m-%d") << '\n';

        //  Line 4+: data
        file << data;

        file.close();

    }
    else
    {
        std::cerr << "Error writing to the file: " << filename << std::endl;
    }
}



int main(int argc, char** argv)
{
    try {
        if (argc < 2) {
            std::cerr << "Usage: "
                << "encrypt -i <in> -o <out> [--iterations N] [--aad TEXT]\n"
                << "       or\n"
                << "decrypt -i <in> -o <out> [--aad TEXT]\n";
            return 1;
        }
        std::string mode = argv[1];
        std::string in, out;
        std::optional<std::string> aad;
        std::uint32_t iterations = 200000;

        for (int i = 2; i < argc; ++i) {
            std::string s = argv[i];
            if (s == "-i" && i + 1 < argc) in = argv[++i];
            else if (s == "-o" && i + 1 < argc) out = argv[++i];
            else if (s == "--aad" && i + 1 < argc) aad = std::string(argv[++i]);
            else if (s == "--iterations" && i + 1 < argc) iterations = (std::uint32_t)std::stoul(argv[++i]);
            else { std::cerr << "Unknown or incomplete option: " << s << "\n"; return 1; }
        }
        if (in.empty() || out.empty()) { std::cerr << "-i and -o are required\n"; return 1; }
        std::cerr << "Passphrase (visible): ";
        std::string pass; std::getline(std::cin, pass);

        if (mode == "encrypt") {
            // read plaintext (text or binary)
            auto plain = sc::read_all_bytes(in);
            std::optional<std::vector<std::uint8_t>> aadv = std::nullopt;
            if (aad) aadv = std::vector<std::uint8_t>(aad->begin(), aad->end());
            auto enc = sc::encrypt_aead(plain, pass, aadv, iterations);
            sc::write_container(out, enc);
            std::cout << "Encrypted " << in << " -> " << out << "\n";
        }
        else if (mode == "decrypt") {
            auto enc = sc::read_container(in);
            std::optional<std::vector<std::uint8_t>> aadv = std::nullopt;
            if (aad) aadv = std::vector<std::uint8_t>(aad->begin(), aad->end());
            auto plain = sc::decrypt_aead(enc, pass, aadv);
            sc::write_all_bytes(out, plain);
            std::cout << "Decrypted " << in << " -> " << out << "\n";
        }
        else {
            std::cerr << "First arg must be 'encrypt' or 'decrypt'\n";
            return 1;
        }
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu
