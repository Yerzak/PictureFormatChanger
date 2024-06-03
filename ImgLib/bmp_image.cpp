#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

    // to calculate padding from width
    static int GetBMPStride(int w) {
        return 4 * ((w * 3 + 3) / 4);
    }

    static const int BITMAP_FILE_HEADER_SIZE = 14;
    static const int BITMAP_INFO_HEADER_SIZE = 40;
    static const int BITMAP_HEADER_SIZE = BITMAP_FILE_HEADER_SIZE + BITMAP_INFO_HEADER_SIZE;
    PACKED_STRUCT_BEGIN BitmapFileHeader{//this macro turns off structure's padding and means structure's beginning, changing "struct" key-word 
    public:
        BitmapFileHeader(int image_width, int image_height)
        : header_data_weight(BITMAP_HEADER_SIZE + (GetBMPStride(image_width) * image_height))//data-size = padding * image_height
        {//c-tor. Not "explicit" to possibility with manual changing header_data_weight 
        }
        // Bitmap File Header fields
        char label[2] = {'B', 'M'};//format label
        uint32_t header_data_weight;//amount of headers sizes
        uint32_t reserved_space = 0;// initialized by zero's
        uint32_t indention = BITMAP_HEADER_SIZE;// between file's beginning and data-beginning - full-header-size
    }
        PACKED_STRUCT_END//this macro ends the part without padding

        PACKED_STRUCT_BEGIN BitmapInfoHeader{//this macro turns off structure's padding and means structure's beginning, changing "struct" key-word
        public:
            BitmapInfoHeader(int image_width, int image_height)
            : width_in_pixels(image_width), height_in_pixels(image_height), bytes_in_data(GetBMPStride(image_width) * image_height) {
            }
            // Bitmap Info Header fields
            uint32_t info_header_weight = BITMAP_INFO_HEADER_SIZE;//size of the second part of the header
            int32_t width_in_pixels;
            int32_t height_in_pixels;
            uint16_t color_plane_count = 1;
            uint16_t pit_in_pixel_count = 24;
            uint32_t compression_type = 0;
            uint32_t bytes_in_data;
            int32_t horizontal_resolution = 11811;// count of pixels in 1 meter
            int32_t vertical_resolution = 11811;//count of pixels in 1 meter
            int32_t color_count = 0;//of used colores
            int32_t valuable_colors = 0x1000000;//number of sagnificant colors
    }
        PACKED_STRUCT_END//this macro ends the part without padding

        // write this method
        bool SaveBMP(const Path& file, const Image& image) {
        ofstream out(file, ios::binary);//flag for binary data 
        const int w = image.GetWidth();
        const int h = image.GetHeight();
        BitmapFileHeader bfh(w, h);//creating first header part
        BitmapInfoHeader bih(w, h);//creating second header part
        out.write(reinterpret_cast<const char*>(&bfh), BITMAP_FILE_HEADER_SIZE);//recording first header part
        out.write(reinterpret_cast<const char*>(&bih), BITMAP_INFO_HEADER_SIZE);//recording second header part
        auto stride = GetBMPStride(w);
        std::vector<char> buff(stride);//In saving and loading create vector<char> with size, which will be equal to padding
        for (int y = h - 1; y >= 0; --y) {//we shall record from down to up
            const Color* line = image.GetLine(y);
            for (int x = 0; x < w; ++x) {
                buff[x * 3 + 0] = static_cast<char>(line[x].b);//bytes record with reverse order
                buff[x * 3 + 1] = static_cast<char>(line[x].g);
                buff[x * 3 + 2] = static_cast<char>(line[x].r);
            }
            /*padding is not always equal "width * 3": padding may be added to every query to make number of bytes multiplyof four. 
            We need to fill padding with zero bytes. Padding-calculation formula is given below.*/
            
            out.write(buff.data(), stride);
        }
        return out.good();
    }

    // write this method
    Image LoadBMP(const Path& file) {
        //opening stream with binary-flag
        // cause we shall read data from binary-code
        ifstream ifs(file, ios::binary);
        //bytes has recorded to file and read from file in order for its description in structure-body
        //strucures is going in sequence in order, we wrote. Then will go pixel's description
        int32_t w, h, bytes_to_ignore;//creating local variables
        bytes_to_ignore = sizeof(BitmapFileHeader) + sizeof(uint32_t); //ignoring the part of header, that consist of BitmapFileHeader and first field of BitmapInfoHeader
        ifs.ignore(bytes_to_ignore);
        ifs.read(reinterpret_cast<char*>(&w), sizeof(w));//reading width - 4 bytes
        ifs.read(reinterpret_cast<char*>(&h), sizeof(h));//reading height - 4 bytes
        Image result(w, h, Color::Black());//creating image-prototype, which will be filled
        auto stride = GetBMPStride(w);
        std::vector<char> buff(stride);//In saving and loading create vector<char> with size, which will be equal to padding
        bytes_to_ignore = sizeof(uint32_t) * 3 + sizeof(int32_t) * 4;//ignoring last part of header - useless fields of BitmapInfoHeader
        ifs.ignore(bytes_to_ignore);
        for (int y = h - 1; y >= 0; --y) {//reading from down to up
            Color* line = result.GetLine(y);
            ifs.read(buff.data(), stride);//our buff has stride-equal size
            for (int x = 0; x < w; ++x) {
                line[x].b = static_cast<byte>(buff[x * 3 + 0]);//bytes are in reverse order
                line[x].g = static_cast<byte>(buff[x * 3 + 1]);
                line[x].r = static_cast<byte>(buff[x * 3 + 2]);
            }
        }
        return result;
    }

}  // namespace img_lib