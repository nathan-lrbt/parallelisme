// GPU version of main_std_cpu.cpp, compiled with: nvc++ -std=c++20 -stdpar=gpu -O3
// Only the lines marked "GPU:" differ from the CPU version.
//
// Arguments:
//   width height max_iter xmin xmax ymin ymax output.bmp
//
// There is no num_threads argument: the standard gives no way to set it.
// With libstdc++ (TBB backend), limit the cores externally, e.g. taskset -c 0-3 ./mandelbrot_par ...

/*
* For Mandelbrot:
* ./mandelbrot_par 4000 3000 300 -2.2 1.0 -1.5 1.5 mandelbrot.bmp
*/

#include <vector>
#include <string>
#include <cmath>
#include <chrono>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <array>
#include <cstdint>
#include <execution>
#include <numeric>

// Little-endian 16-bit helper
void put_u16le(std::vector<uint8_t>& buf, uint16_t v) {
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

// Little-endian 32-bit helper
void put_u32le(std::vector<uint8_t>& buf, uint32_t v) {
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

// Writes an RGB24 image buffer (width*height*3 bytes, row-major, top-to-bottom,
// interleaved R,G,B) to an uncompressed 24-bit BMP file.
bool write_bmp(
    const std::string& filename,
    size_t width, size_t height,
    const std::vector<uint8_t>& img
) {
    const size_t row_size = ((width * 3 + 3) / 4) * 4; // rows padded to 4 bytes
    const size_t pixel_data_size = row_size * height;
    const uint32_t file_size = static_cast<uint32_t>(54 + pixel_data_size);

    std::vector<uint8_t> header;
    header.reserve(54);

    // BITMAPFILEHEADER (14 bytes)
    header.push_back('B');
    header.push_back('M');
    put_u32le(header, file_size);
    put_u32le(header, 0);  // reserved
    put_u32le(header, 54); // pixel data offset

    // BITMAPINFOHEADER (40 bytes)
    put_u32le(header, 40); // header size
    put_u32le(header, static_cast<uint32_t>(width));
    put_u32le(header, static_cast<uint32_t>(height)); // positive = bottom-up
    put_u16le(header, 1);  // color planes
    put_u16le(header, 24); // bits per pixel
    put_u32le(header, 0);  // compression: BI_RGB (none)
    put_u32le(header, static_cast<uint32_t>(pixel_data_size));
    put_u32le(header, 2835); // x pixels per meter (~72 DPI)
    put_u32le(header, 2835); // y pixels per meter (~72 DPI)
    put_u32le(header, 0);    // colors in palette
    put_u32le(header, 0);    // important colors

    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) {
        return false;
    }
    ofs.write(reinterpret_cast<const char*>(header.data()), header.size());

    // BMP stores rows bottom-to-top and pixels as B,G,R.
    std::vector<uint8_t> row(row_size, 0);
    for (size_t y = 0; y < height; ++y) {
        const size_t src_y = height - 1 - y;
        for (size_t x = 0; x < width; ++x) {
            const size_t src_idx = (src_y * width + x) * 3;
            row[x * 3 + 0] = img[src_idx + 2]; // B
            row[x * 3 + 1] = img[src_idx + 1]; // G
            row[x * 3 + 2] = img[src_idx + 0]; // R
        }
        ofs.write(reinterpret_cast<const char*>(row.data()), row.size());
    }

    return static_cast<bool>(ofs);
}

void palette_rgb(int n, int max_iter, uint8_t& r, uint8_t& g, uint8_t& b) {
    if (n >= max_iter) { r = g = b = 0; return; }

    // GPU: no `static` on these arrays, local static variables cannot be used in device code
    // Smooth gradient (navy -> blue -> white -> orange -> near black -> navy)
    // interpolated between a few fixed control colors.
    constexpr auto stops = std::to_array<double>({0.0, 0.16, 0.42, 0.6425, 0.8575});
    constexpr auto colors = std::to_array<std::array<uint8_t, 3>>({
        {0, 7, 100},
        {32, 107, 203},
        {237, 255, 255},
        {255, 170, 0},
        {0, 2, 0},
    });
    constexpr size_t count = stops.size();

    const double t = static_cast<double>(n) / static_cast<double>(max_iter);

    size_t i = 0;
    while (i + 1 < count && t > stops[i + 1]) {
        ++i;
    }
    const size_t j = (i + 1) % count;
    const double span = (j == 0 ? 1.0 : stops[j]) - stops[i];
    const double local_t = span > 0.0 ? (t - stops[i]) / span : 0.0;

    // GPU: std::lround() is not available in device code (link error __nv_lround).
    // The values are non-negative, so (long)(x + 0.5) gives the same result.
    r = static_cast<uint8_t>((long)(colors[i][0] + local_t * (colors[j][0] - colors[i][0]) + 0.5));
    g = static_cast<uint8_t>((long)(colors[i][1] + local_t * (colors[j][1] - colors[i][1]) + 0.5));
    b = static_cast<uint8_t>((long)(colors[i][2] + local_t * (colors[j][2] - colors[i][2]) + 0.5));
}

int main(int argc, char** argv) {
    if (argc < 9) {
        std::cerr << "Usage: " << argv[0]
                  << " width height max_iter xmin xmax ymin ymax output.bmp"
                  << std::endl;
        return 1;
    }
    const size_t width = std::atoi(argv[1]);
    const size_t height = std::atoi(argv[2]);
    const size_t max_iter = std::atoi(argv[3]);
    const double xmin = std::atof(argv[4]);
    const double xmax = std::atof(argv[5]);
    const double ymin = std::atof(argv[6]);
    const double ymax = std::atof(argv[7]);
    const std::string out_name = argv[8];

    if (xmin >= xmax || ymin >= ymax) {
        std::cerr << "Invalid window." << std::endl;
        return 1;
    }

    std::vector<uint8_t> img(width * height * 3, 0);

    // Index of each pixel, the loop is a for_each over them
    // Created before the timer, as the OpenMP version has no equivalent
    // NOTE: std::views::iota avoids this 64 MB vector
    std::vector<size_t> indices(width * height, 0);
    std::iota(indices.begin(), indices.end(), 0);

    // GPU: the lambda is copied to the device, it cannot capture `img` by reference.
    // Capture a raw pointer by value instead.
    auto* const img_ptr = img.data();

    const auto start = std::chrono::high_resolution_clock::now();

    std::for_each(std::execution::par_unseq, indices.cbegin(), indices.cend(), [=](size_t i) {
        const size_t y = i / width;
        const size_t x = i % width;

        // Mandelbrot set
        // GPU: Replace the <complex> header because nvc++ 25.3 does not support it
        // (c and z are kept as real and imaginary parts).
        // Use -1 to make the range fully included (could use pixel center)
        const double cr = xmin + (xmax - xmin) * static_cast<double>(x) / static_cast<double>(width - 1);
        const double ci = ymin + (ymax - ymin) * static_cast<double>(y) / static_cast<double>(height - 1);
        double zr = 0.0;
        double zi = 0.0;

        size_t n = 0;
        // Iterate: |z| <= 2  <=>  |z|^2 <= 4 (no square root needed)
        while (zr * zr + zi * zi <= 4.0 && n < max_iter) {
            // z = z * z + c
            const double zr_next = zr * zr - zi * zi + cr;
            zi = 2.0 * zr * zi + ci;
            zr = zr_next;
            ++n;
        }
        uint8_t r,g,b;
        palette_rgb(n, max_iter, r, g, b);
        const size_t idx = (y * width + x) * 3;
        img_ptr[idx + 0] = r;
        img_ptr[idx + 1] = g;
        img_ptr[idx + 2] = b;
    });

    const auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Elapsed time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms" << std::endl;

    // if (!write_bmp(out_name, width, height, img)) {
    //     std::cerr << "Error: failed to write file " << out_name << "." << std::endl;
    //     return 1;
    // }

    return 0;
}
