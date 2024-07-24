#include <arpa/inet.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/stat.h>

#define NBT_IMPLEMENTATION
#include "nbt.h"

#ifdef UNIT_TESTS
#define MAIN definitely_not_main
#else
#define MAIN main
#endif

static auto get_file_size(const std::string &filename) -> int64_t
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

static auto load_file(const std::string &file, size_t *size) -> char *
{
    if (!std::filesystem::exists(file))
        return nullptr;

    auto file_size = get_file_size(file);
    if (file_size == -1)
        return nullptr;

    char *file_contents = (char *) malloc(file_size);

    FILE *opened_file = fopen(file.c_str(), "r");

    *size = fread(file_contents, 1, file_size, opened_file);

    if (*size != (size_t) file_size) {
        dprintf(2, "Could not read everything from %s", file.c_str());
        free(file_contents);
        return nullptr;
    }
    return file_contents;
}

struct UserData {
    char *start;
    char *end;
};

static auto read_mem(void *ud, uint8_t *d, size_t s) -> size_t
{
    auto *u = (UserData *) ud;
    if (u->start > u->end)
        return 0;
    // TODO(huntears) Check this +1 because this is strange uwu
    const auto max = ((size_t) u->end) - ((size_t) u->start) + 1;
    const auto to_copy = std::min(max, s);
    memcpy(d, u->start, to_copy);
    u->start += to_copy;
    return to_copy;
}

constexpr uint32_t MAX_X_PER_REGION = 32;
constexpr uint32_t MAX_Z_PER_REGION = 32;
constexpr uint32_t NUM_CHUNKS_PER_REGION = MAX_X_PER_REGION * MAX_Z_PER_REGION;
constexpr uint64_t REGION_CHUNK_ALIGNMENT = 0x1000;
constexpr uint64_t REGION_CHUNK_ALIGNMENT_MASK = 0xFFF;

struct __attribute__((__packed__)) RegionLocation {
    uint32_t data{};

    [[nodiscard]]
    inline auto get_offset() const -> uint32_t
    {
        return ((data & 0x00FF0000) >> 16) | (data & 0x0000FF00) | ((data & 0x000000FF) << 16);
    }

    [[nodiscard]]
    inline auto get_size() const -> uint8_t
    {
        return data >> 24;
    }

    [[nodiscard]]
    inline auto is_empty() const -> bool
    {
        return data == 0;
    }

    RegionLocation(uint32_t offset, uint8_t size):
        data(
            ((offset & 0x00FF0000) >> 16) | (offset & 0x0000FF00) | ((offset & 0x000000FF) << 16)
            | (size << 24)
        )
    {
    }
};

struct __attribute__((__packed__)) RegionTimestamp {
    uint32_t data{};
};

struct __attribute__((__packed__)) RegionHeader {
    RegionLocation locationTable[NUM_CHUNKS_PER_REGION];
    RegionTimestamp timestampTable[NUM_CHUNKS_PER_REGION];
};

enum class region_chunk_compression_scheme : uint8_t {
    GZIP = 1,
    ZLIB = 2,
};

struct __attribute__((__packed__)) ChunkHeader {
    uint32_t length;
    region_chunk_compression_scheme compressionScheme;

    [[nodiscard]]
    inline auto get_length() const -> uint32_t
    {
        return ntohl(length);
    }

    [[nodiscard]]
    inline auto get_compression_scheme() const -> region_chunk_compression_scheme
    {
        return compressionScheme;
    }
};

auto MAIN() -> int
{
    for (std::string file; std::getline(std::cin, file);) {
        size_t file_size = 0;
        char *file_contents = load_file(file, &file_size);
        if (file_contents == nullptr) {
            perror("open");
            continue;
        }

        const auto *header = (const RegionHeader *) file_contents;

        for (uint16_t cx = 0; cx < MAX_X_PER_REGION; cx++) {
            for (uint16_t cz = 0; cz < MAX_Z_PER_REGION; cz++) {
                const uint16_t current_offset = cx + cz * MAX_X_PER_REGION;

                if (header->locationTable[current_offset].is_empty())
                    continue;

                const uint64_t chunk_offset = header->locationTable[current_offset].get_offset()
                    * REGION_CHUNK_ALIGNMENT;

                const auto *c_header = (const ChunkHeader *) (file_contents + chunk_offset);

                UserData ud = {
                    ((char *) c_header) + sizeof(*c_header),
                    ((char *) c_header) + sizeof(*c_header) + c_header->get_length() - 1,
                };

                nbt_reader_t reader = {
                    read_mem,
                    &ud,
                };

                auto *data = nbt_parse(reader, NBT_PARSE_FLAG_USE_ZLIB);
                assert(data->type == NBT_TYPE_COMPOUND);
                if (data->type != NBT_TYPE_COMPOUND) {
                    dprintf(2, "Chunk %d %d does not countain a compound!\n", cx, cz);
                } else {
                    // printf("Loaded chunk %d %d\n", cx, cz);
                }

                nbt_free_tag(data);
            }
        }
        printf("Loaded region from file %s completely\n", file.c_str());
        free(file_contents);
    }
    return EXIT_SUCCESS;
}
