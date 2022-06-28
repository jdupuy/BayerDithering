#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define LOG(fmt, ...)  fprintf(stdout, fmt "\n", ##__VA_ARGS__); fflush(stdout);

bool IsPowerOfTwo(uint32_t x)
{
    return (x & (x - 1)) == 0;
}

uint32_t FindMSB(uint32_t x)
{
    uint32_t r = (x & 0xAAAAAAAAu) != 0u;

    r |= ((x & 0xFFFF0000u) != 0) << 4;
    r |= ((x & 0xFF00FF00u) != 0) << 3;
    r |= ((x & 0xF0F0F0F0u) != 0) << 2;
    r |= ((x & 0xCCCCCCCCu) != 0) << 1;

    return r;
}

uint32_t Dilate(uint32_t x) {
    x = (x | (x << 8)) & 0x00FF00FFu;
    x = (x | (x << 4)) & 0x0F0F0F0Fu;
    x = (x | (x << 2)) & 0x33333333u;
    x = (x | (x << 1)) & 0x55555555u;

    return x;
}

uint32_t BitReverse(uint32_t x)
{
    x = ((x & 0x55555555u) <<  1u) | ((x & 0xAAAAAAAAu) >>  1u);
    x = ((x & 0x33333333u) <<  2u) | ((x & 0xCCCCCCCCu) >>  2u);
    x = ((x & 0x0F0F0F0Fu) <<  4u) | ((x & 0xF0F0F0F0u) >>  4u);
    x = ((x & 0x00FF00FFu) <<  8u) | ((x & 0xFF00FF00u) >>  8u);
    x = ((x & 0x0000FFFFu) << 16u) | ((x & 0xFFFF0000u) >> 16u);

    return x;
}

uint32_t BayerMatrixCoefficient(uint32_t i, uint32_t j, uint32_t matrixSize)
{
    const uint32_t x = i ^ j;
    const uint32_t y = j;
    const uint32_t z = Dilate(x) | (Dilate(y) << 1);
    const uint32_t b = BitReverse(z) >> (32 - (matrixSize << 1));

    return b;
}

void Usage(const char *appName)
{
    LOG("%s -- Bayer Image Dithering", appName);
    LOG("usage: %s colorsPerChannel pathToFile", appName);
}

int main(int argc, const char **argv)
{
    struct Image {
        int32_t width, height, components;
        uint8_t *data;
    } input = {0, 0, 0, NULL}, output = {0, 0, 0, NULL};
    uint32_t bayerMatrixSize = 0;
    uint32_t colorsPerChannel = 4;
    const char *pathToFile = NULL;

    // parse command line
    if (argc != 3) {
        Usage(argv[0]);
        return EXIT_FAILURE;
    }
    colorsPerChannel = atoi(argv[1]);
    pathToFile = argv[2];

    // load image
    input.data = stbi_load(pathToFile, &input.width, &input.height, &input.components, 0);
    if (input.data == NULL) {
        LOG("error: loading failed");
        return EXIT_FAILURE;
    }
    if (input.width != input.height) {
        LOG("error: image must be square");
        return EXIT_FAILURE;
    }
    if (!IsPowerOfTwo(input.width)) {
        LOG("error: image size must be a power of two");
        return EXIT_FAILURE;
    }
    bayerMatrixSize = FindMSB(input.width);

    // init output
    output.width = input.width;
    output.height = input.height;
    output.components = input.components;
    output.data = malloc(sizeof(*output.data) * output.width * output.height * output.components);

    // dither
    for (int32_t y = 0; y < input.height; ++y) {
        for (int32_t x = 0; x < input.width; ++x) {
            for (int32_t c = 0; c < input.components; ++c) {
                const uint32_t dataID = c + input.components * (x + input.width * y);
                const uint8_t data = input.data[dataID];
                const float u = data / 255.0f;
                const float q = u * (colorsPerChannel - 1u);
                const float qmin = floorf(q) / (colorsPerChannel - 1u);
                const float qmax = ceilf(q) / (colorsPerChannel - 1u);
                const float v = BayerMatrixCoefficient(x, y, bayerMatrixSize)
                              / (float)(input.width * input.width);

                if ((u - qmin) / (qmax - qmin) > v) {
                    output.data[dataID] = roundf(qmax * 255.0f);
                } else {
                    output.data[dataID] = roundf(qmin * 255.0f);
                }
            }
        }
    }

    // export
    stbi_write_bmp("output.bmp", output.width, output.height, output.components, output.data);

    // cleanup
    free(input.data);
    free(output.data);

    return EXIT_SUCCESS;
}
