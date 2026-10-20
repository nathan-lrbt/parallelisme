// Arguments:
//   width height max_iter xmin xmax ymin ymax num_threads output.bmp

/*
* For Mandelbrot (num_threads = 0 keeps the default):
* ./mandelbrot_omp 4000 3000 300 -2.2 1.0 -1.5 1.5 0 mandelbrot.bmp
*/

#include <vector>
#include <string>
#include <cmath>
#include <chrono>
#include <iostream>
#include <fstream>
#include <complex>
#include <omp.h>

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

    // Smooth gradient (navy -> blue -> white -> orange -> near black -> navy)
    // interpolated between a few fixed control colors.
    static constexpr auto stops = std::to_array<double>({0.0, 0.16, 0.42, 0.6425, 0.8575});
    static constexpr auto colors = std::to_array<std::array<uint8_t, 3>>({
        {0, 7, 100},
        {32, 107, 203},
        {237, 255, 255},
        {255, 170, 0},
        {0, 2, 0},
    });
    static constexpr size_t count = stops.size();

    const double t = static_cast<double>(n) / static_cast<double>(max_iter);

    size_t i = 0;
    while (i + 1 < count && t > stops[i + 1]) {
        ++i;
    }
    const size_t j = (i + 1) % count;
    const double span = (j == 0 ? 1.0 : stops[j]) - stops[i];
    const double local_t = span > 0.0 ? (t - stops[i]) / span : 0.0;

    r = static_cast<uint8_t>(std::lround(colors[i][0] + local_t * (colors[j][0] - colors[i][0])));
    g = static_cast<uint8_t>(std::lround(colors[i][1] + local_t * (colors[j][1] - colors[i][1])));
    b = static_cast<uint8_t>(std::lround(colors[i][2] + local_t * (colors[j][2] - colors[i][2])));
}

int main(int argc, char** argv) {
    if (argc < 10) {
        std::cerr << "Usage: " << argv[0]
                  << " width height max_iter xmin xmax ymin ymax num_threads output.bmp"
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
    const int num_threads = std::atoi(argv[8]);
    const std::string out_name = argv[9];

    if (xmin >= xmax || ymin >= ymax) {
        std::cerr << "Invalid window." << std::endl;
        return 1;
    }
    if (num_threads > 0) {
        omp_set_num_threads(num_threads);
    }

    std::vector<uint8_t> img(width * height * 3, 0);

    const auto start = std::chrono::high_resolution_clock::now();

    #pragma omp parallel for collapse(2)
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            // Mandelbrot set
            const std::complex<double> c {
                // Use -1 to make the range fully included (could use pixel center)
                xmin + (xmax - xmin) * static_cast<double>(x) / static_cast<double>(width - 1),
                ymin + (ymax - ymin) * static_cast<double>(y) / static_cast<double>(height - 1)
            };
            std::complex<double> z { 0.0, 0.0 };

            size_t n = 0;
            // Iterate
            while (std::abs(z) <= 2.0 && n < max_iter) {
                z = z * z + c;
                ++n;
            }
            uint8_t r,g,b;
            palette_rgb(n, max_iter, r, g, b);
            const size_t idx = (y * width + x) * 3;
            img[idx + 0] = r;
            img[idx + 1] = g;
            img[idx + 2] = b;
        }
    }

    const auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Elapsed time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms" << std::endl;

    if (!write_bmp(out_name, width, height, img)) {
        std::cerr << "Error: failed to write file " << out_name << "." << std::endl;
        return 1;
    }

    return 0;
}
