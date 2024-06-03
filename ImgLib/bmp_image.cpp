#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

    // функция вычисления отступа по ширине
    static int GetBMPStride(int w) {
        return 4 * ((w * 3 + 3) / 4);
    }

    static const int BITMAP_FILE_HEADER_SIZE = 14;
    static const int BITMAP_INFO_HEADER_SIZE = 40;
    static const int BITMAP_HEADER_SIZE = BITMAP_FILE_HEADER_SIZE + BITMAP_INFO_HEADER_SIZE;
    PACKED_STRUCT_BEGIN BitmapFileHeader{//макрос отключает padding у структуры и обозначает начало структуры, заменяя ключевое слово struct
    public:
        BitmapFileHeader(int image_width, int image_height)
        : header_data_weight(BITMAP_HEADER_SIZE + (GetBMPStride(image_width) * image_height))//размер данных определяется как отступ, умноженный на высоту изображения
        {//к-тор структуры. Не объявляем explicit, чтобы можно было поле header_data_weight задать отдельно        
        }
        // поля заголовка Bitmap File Header
        char label[2] = {'B', 'M'};//подпись
        uint32_t header_data_weight;//суммарный вес заголовка и данных
        uint32_t reserved_space = 0;//зарезервированное пространство, инициализированное нулями
        uint32_t indention = BITMAP_HEADER_SIZE;//промежуток между началом файла и началом данных - размер заголовка
    }
        PACKED_STRUCT_END//макрос завершают участок с отключенным padding

        PACKED_STRUCT_BEGIN BitmapInfoHeader{//макрос отключает padding у структуры и обозначает начало структуры, заменяя ключевой слово struct
        public:
            BitmapInfoHeader(int image_width, int image_height)
            : width_in_pixels(image_width), height_in_pixels(image_height), bytes_in_data(GetBMPStride(image_width) * image_height) {
            }
            // поля заголовка Bitmap Info Header
            uint32_t info_header_weight = BITMAP_INFO_HEADER_SIZE;//размер второй части заголовка
            int32_t width_in_pixels;//ширина изображения в пикселях
            int32_t height_in_pixels;//высота изображения в пикселях
            uint16_t color_plane_count = 1;//количество цветовых плоскостей
            uint16_t pit_in_pixel_count = 24;
            uint32_t compression_type = 0;//тип сжатия
            uint32_t bytes_in_data;//количество бит на пиксель
            int32_t horizontal_resolution = 11811;//горизонтальное расширение - количество пикселей на метр
            int32_t vertical_resolution = 11811;//вертикальное расширение - количество пикселей на метр
            int32_t color_count = 0;//количество использованных цветов
            int32_t valuable_colors = 0x1000000;//количество значимых цветов
    }
        PACKED_STRUCT_END//макрос завершают участок с отключенным padding

        // напишите эту функцию
        bool SaveBMP(const Path& file, const Image& image) {
        ofstream out(file, ios::binary);//флаг для бинарников    
        const int w = image.GetWidth();
        const int h = image.GetHeight();
        BitmapFileHeader bfh(w, h);//создаем первую часть заголовка
        BitmapInfoHeader bih(w, h);//создаем вторую часть заголовка
        out.write(reinterpret_cast<const char*>(&bfh), BITMAP_FILE_HEADER_SIZE);//записываем первую часть заголовка
        out.write(reinterpret_cast<const char*>(&bih), BITMAP_INFO_HEADER_SIZE);//записываем вторую часть заголовка
        auto stride = GetBMPStride(w);
        std::vector<char> buff(stride);//При сохранении и загрузке создайте vector<char> с размером, равным отступу
        for (int y = h - 1; y >= 0; --y) {//записывать будем снизу вверх
            const Color* line = image.GetLine(y);
            for (int x = 0; x < w; ++x) {
                buff[x * 3 + 0] = static_cast<char>(line[x].b);//биты указываются в обратном порядке
                buff[x * 3 + 1] = static_cast<char>(line[x].g);
                buff[x * 3 + 2] = static_cast<char>(line[x].r);
            }
            /*отступ не всегда равен утроенной ширине: к каждой строке может добавляться padding, чтобы количество байт стало кратно четырём. Padding нужно заполнить нулевыми байтами. Формула для вычисления отступа приведена ниже.*/
            /*if (int padding = stride - (w * 3) > 0) {
                for (int p = 0; p < padding; ++p) {
                    buff[w * 3 + p] = '\0';
                }
            }*/
            out.write(buff.data(), stride);
        }
        return out.good();
    }

    // напишите эту функцию
    Image LoadBMP(const Path& file) {
        // открываем поток с флагом ios::binary
        // поскольку будем читать данные в двоичном формате
        ifstream ifs(file, ios::binary);
        //байты записываются в файл и читаются из него по порядку их описания в теле структуры
        //структуры идут одна за другой в том же порядке, в котором мы их туда записали, потом идет описание пикселей
        int32_t w, h, bytes_to_ignore;//создаем локальные переменные
        bytes_to_ignore = sizeof(BitmapFileHeader) + sizeof(uint32_t); //игнорируем всю часть заголовка с BitmapFileHeader и первое поле BitmapInfoHeader
        ifs.ignore(bytes_to_ignore);
        ifs.read(reinterpret_cast<char*>(&w), sizeof(w));//читаем широту - 4 байта
        ifs.read(reinterpret_cast<char*>(&h), sizeof(h));//читаем высоту - 4 байта
        Image result(w, h, Color::Black());//создаем прообраз картинки, который будем заполнять
        auto stride = GetBMPStride(w);
        std::vector<char> buff(stride);//При сохранении и загрузке создайте vector<char> с размером, равным отступу
        bytes_to_ignore = sizeof(uint32_t) * 3 + sizeof(int32_t) * 4;//игнорируем оставшуюся часть заголовка - ненужные поля структуры BitmapInfoHeader
        ifs.ignore(bytes_to_ignore);
        for (int y = h - 1; y >= 0; --y) {//читаем снизу вверх
            Color* line = result.GetLine(y);
            ifs.read(buff.data(), stride);//учитываем, что "При сохранении и загрузке создайте vector<char> с размером, равным отступу"
            for (int x = 0; x < w; ++x) {
                line[x].b = static_cast<byte>(buff[x * 3 + 0]);//биты указываются в обратном порядке
                line[x].g = static_cast<byte>(buff[x * 3 + 1]);
                line[x].r = static_cast<byte>(buff[x * 3 + 2]);
            }
        }
        return result;
    }

}  // namespace img_lib