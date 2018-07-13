#define AD_MIPMAP_IMPLEMENTATION
#include "ad_mipmap.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool save_whole_file(const char* path, const void* file_contents, uint64_t file_size)
{
    FILE* file = fopen(path, "wb");
    if(!file)
    {
        return false;
    }
    size_t written = fwrite(file_contents, 1, file_size, file);
    if(written != file_size)
    {
        return false;
    }
    fclose(file);
    return true;
}

#pragma pack(push, bmp, 1)

typedef struct BmpFileHeader
{
    char type[2];
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} BmpFileHeader;

typedef struct BmpInfoHeader
{
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t pixels_per_meter_x;
    int32_t pixels_per_meter_y;
    uint32_t colours_used;
    uint32_t important_colours;
} BmpInfoHeader;

#pragma pack(pop, bmp)

typedef enum Compression
{
    COMPRESSION_NONE = 0,
} Compression;

static unsigned int pad_multiple_of_four(unsigned int x)
{
    return (x + 3) & ~0x3;
}

static bool bmp_write_file(const char* path, const uint8_t* pixels, int width, int height)
{
    unsigned int bytes_per_pixel = 3;
    unsigned int row_bytes = bytes_per_pixel * width;
    unsigned int padded_width = pad_multiple_of_four(row_bytes);
    unsigned int pixel_data_size = padded_width * height;
    int row_padding = padded_width - row_bytes;

    BmpInfoHeader info =
    {
        .size = sizeof(info),
        .width = width,
        .height = height, // negative indicates rows are ordered top-to-bottom
        .planes = 1,
        .bits_per_pixel = 8 * bytes_per_pixel,
        .compression = COMPRESSION_NONE,
        .image_size = width * height * info.bits_per_pixel,
        .pixels_per_meter_x = 0,
        .pixels_per_meter_y = 0,
        .colours_used = 0,
        .important_colours = 0,
    };

    BmpFileHeader header =
    {
        .type[0] = 'B',
        .type[1] = 'M',
        .size = sizeof(header) + sizeof(info) + pixel_data_size,
        .reserved1 = 0,
        .reserved2 = 0,
        .offset = sizeof(header) + sizeof(info),
    };

    // Create the file in-memory.
    uint64_t file_size = header.size;
    uint8_t* file_contents = (uint8_t*) calloc(file_size, 1);
    uint8_t* hand = file_contents;

    memcpy(hand, &header, sizeof(header));
    hand += sizeof(header);
    memcpy(hand, &info, sizeof(info));
    hand += sizeof(info);

    for(int i = 0; i < height; i += 1)
    {
        uint64_t row_size = bytes_per_pixel * width;
        memcpy(hand, &pixels[bytes_per_pixel * width * i], row_size);
        hand += row_size;
        memset(hand, 0, row_padding);
        hand += row_padding;
    }

    save_whole_file(path, file_contents, file_size);

    free(file_contents);

    return true;
}

static void draw_sine_waves(uint8_t* pixels, int width, int height)
{
    const float frequency = 0.067f;
    const int bytes_per_pixel = 3;

    for(int y = 0; y < height; y += 1)
    {
        int row_start = bytes_per_pixel * width * y;
        float phase_y = sinf(frequency * y);

        for(int x = 0; x < width; x += 1)
        {
            int pixel_index = (bytes_per_pixel * x) + row_start;
            float phase = phase_y + sinf(frequency * x);
            float r = fmodf(phase, 0.4f) * 2.5f;
            pixels[pixel_index    ] = 0xff * r;
            pixels[pixel_index + 1] = 0xff * r;
            pixels[pixel_index + 2] = 0xff * r;
        }
    }
}

int main(int argc, const char** argv)
{
    AdmBitmap bitmap =
    {
        .width = 451,
        .height = 244,
        .bytes_per_pixel = 3,
    };
    bitmap.pixels = calloc(bitmap.width * bitmap.height, bitmap.bytes_per_pixel);

    draw_sine_waves((uint8_t*) bitmap.pixels, bitmap.width, bitmap.height);

    int levels;
    AdmBitmap* mipmaps = adm_generate_mipmaps(&levels, &bitmap);

    char name[11] = "sine00.bmp";
    for(int i = 0; i < levels; i += 1)
    {
        name[4] = '0' + (i / 10);
        name[5] = '0' + (i % 10);
        AdmBitmap mipmap = mipmaps[i];
        bmp_write_file(name, (const uint8_t*) mipmap.pixels, mipmap.width, mipmap.height);
    }

    adm_free_mipmaps(mipmaps, levels);

    return 0;
}
