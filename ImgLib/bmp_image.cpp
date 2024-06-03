#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

    // ������� ���������� ������� �� ������
    static int GetBMPStride(int w) {
        return 4 * ((w * 3 + 3) / 4);
    }

    static const int BITMAP_FILE_HEADER_SIZE = 14;
    static const int BITMAP_INFO_HEADER_SIZE = 40;
    static const int BITMAP_HEADER_SIZE = BITMAP_FILE_HEADER_SIZE + BITMAP_INFO_HEADER_SIZE;
    PACKED_STRUCT_BEGIN BitmapFileHeader{//������ ��������� padding � ��������� � ���������� ������ ���������, ������� �������� ����� struct
    public:
        BitmapFileHeader(int image_width, int image_height)
        : header_data_weight(BITMAP_HEADER_SIZE + (GetBMPStride(image_width) * image_height))//������ ������ ������������ ��� ������, ���������� �� ������ �����������
        {//�-��� ���������. �� ��������� explicit, ����� ����� ���� ���� header_data_weight ������ ��������        
        }
        // ���� ��������� Bitmap File Header
        char label[2] = {'B', 'M'};//�������
        uint32_t header_data_weight;//��������� ��� ��������� � ������
        uint32_t reserved_space = 0;//����������������� ������������, ������������������ ������
        uint32_t indention = BITMAP_HEADER_SIZE;//���������� ����� ������� ����� � ������� ������ - ������ ���������
    }
        PACKED_STRUCT_END//������ ��������� ������� � ����������� padding

        PACKED_STRUCT_BEGIN BitmapInfoHeader{//������ ��������� padding � ��������� � ���������� ������ ���������, ������� �������� ����� struct
        public:
            BitmapInfoHeader(int image_width, int image_height)
            : width_in_pixels(image_width), height_in_pixels(image_height), bytes_in_data(GetBMPStride(image_width) * image_height) {
            }
            // ���� ��������� Bitmap Info Header
            uint32_t info_header_weight = BITMAP_INFO_HEADER_SIZE;//������ ������ ����� ���������
            int32_t width_in_pixels;//������ ����������� � ��������
            int32_t height_in_pixels;//������ ����������� � ��������
            uint16_t color_plane_count = 1;//���������� �������� ����������
            uint16_t pit_in_pixel_count = 24;
            uint32_t compression_type = 0;//��� ������
            uint32_t bytes_in_data;//���������� ��� �� �������
            int32_t horizontal_resolution = 11811;//�������������� ���������� - ���������� �������� �� ����
            int32_t vertical_resolution = 11811;//������������ ���������� - ���������� �������� �� ����
            int32_t color_count = 0;//���������� �������������� ������
            int32_t valuable_colors = 0x1000000;//���������� �������� ������
    }
        PACKED_STRUCT_END//������ ��������� ������� � ����������� padding

        // �������� ��� �������
        bool SaveBMP(const Path& file, const Image& image) {
        ofstream out(file, ios::binary);//���� ��� ����������    
        const int w = image.GetWidth();
        const int h = image.GetHeight();
        BitmapFileHeader bfh(w, h);//������� ������ ����� ���������
        BitmapInfoHeader bih(w, h);//������� ������ ����� ���������
        out.write(reinterpret_cast<const char*>(&bfh), BITMAP_FILE_HEADER_SIZE);//���������� ������ ����� ���������
        out.write(reinterpret_cast<const char*>(&bih), BITMAP_INFO_HEADER_SIZE);//���������� ������ ����� ���������
        auto stride = GetBMPStride(w);
        std::vector<char> buff(stride);//��� ���������� � �������� �������� vector<char> � ��������, ������ �������
        for (int y = h - 1; y >= 0; --y) {//���������� ����� ����� �����
            const Color* line = image.GetLine(y);
            for (int x = 0; x < w; ++x) {
                buff[x * 3 + 0] = static_cast<char>(line[x].b);//���� ����������� � �������� �������
                buff[x * 3 + 1] = static_cast<char>(line[x].g);
                buff[x * 3 + 2] = static_cast<char>(line[x].r);
            }
            /*������ �� ������ ����� ��������� ������: � ������ ������ ����� ����������� padding, ����� ���������� ���� ����� ������ ������. Padding ����� ��������� �������� �������. ������� ��� ���������� ������� ��������� ����.*/
            /*if (int padding = stride - (w * 3) > 0) {
                for (int p = 0; p < padding; ++p) {
                    buff[w * 3 + p] = '\0';
                }
            }*/
            out.write(buff.data(), stride);
        }
        return out.good();
    }

    // �������� ��� �������
    Image LoadBMP(const Path& file) {
        // ��������� ����� � ������ ios::binary
        // ��������� ����� ������ ������ � �������� �������
        ifstream ifs(file, ios::binary);
        //����� ������������ � ���� � �������� �� ���� �� ������� �� �������� � ���� ���������
        //��������� ���� ���� �� ������ � ��� �� �������, � ������� �� �� ���� ��������, ����� ���� �������� ��������
        int32_t w, h, bytes_to_ignore;//������� ��������� ����������
        bytes_to_ignore = sizeof(BitmapFileHeader) + sizeof(uint32_t); //���������� ��� ����� ��������� � BitmapFileHeader � ������ ���� BitmapInfoHeader
        ifs.ignore(bytes_to_ignore);
        ifs.read(reinterpret_cast<char*>(&w), sizeof(w));//������ ������ - 4 �����
        ifs.read(reinterpret_cast<char*>(&h), sizeof(h));//������ ������ - 4 �����
        Image result(w, h, Color::Black());//������� �������� ��������, ������� ����� ���������
        auto stride = GetBMPStride(w);
        std::vector<char> buff(stride);//��� ���������� � �������� �������� vector<char> � ��������, ������ �������
        bytes_to_ignore = sizeof(uint32_t) * 3 + sizeof(int32_t) * 4;//���������� ���������� ����� ��������� - �������� ���� ��������� BitmapInfoHeader
        ifs.ignore(bytes_to_ignore);
        for (int y = h - 1; y >= 0; --y) {//������ ����� �����
            Color* line = result.GetLine(y);
            ifs.read(buff.data(), stride);//���������, ��� "��� ���������� � �������� �������� vector<char> � ��������, ������ �������"
            for (int x = 0; x < w; ++x) {
                line[x].b = static_cast<byte>(buff[x * 3 + 0]);//���� ����������� � �������� �������
                line[x].g = static_cast<byte>(buff[x * 3 + 1]);
                line[x].r = static_cast<byte>(buff[x * 3 + 2]);
            }
        }
        return result;
    }

}  // namespace img_lib