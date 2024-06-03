#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <vector>

namespace img_lib {

    struct Size {
        int width;
        int height;
    };

    struct Color {
        static Color Black() {
            return { std::byte{0}, std::byte{0}, std::byte{0}, std::byte{255} };
        }

        std::byte r, g, b, a;
    };

    class Image {
    public:
        // создаЄт пустое изображение
        Image() = default;

        // создаЄт изображение заданного размера, заполн€€ его заданным цветом
        Image(int w, int h, Color fill);

        // геттеры дл€ отдельного пиксел€ изображени€
        Color GetPixel(int x, int y) const {
            return const_cast<Image*>(this)->GetPixel(x, y);
        }
        Color& GetPixel(int x, int y) {
            assert(x < GetWidth() && y < GetHeight() && x >= 0 && y >= 0);
            return GetLine(y)[x];
        }

        // геттер дл€ заданной строки изображени€
        Color* GetLine(int y);
        const Color* GetLine(int y) const;

        int GetWidth() const;
        int GetHeight() const;

        // шаг задаЄт смещение соседних строк изображени€
        // он обычно совпадает с шириной, но может быть больше неЄ
        int GetStep() const;

        // будем считать изображение корректным, если
        // его площадь положительна
        explicit operator bool() const {
            return GetWidth() > 0 && GetHeight() > 0;
        }

        bool operator!() const {
            return !operator bool();
        }

    private:
        int width_ = 0;
        int height_ = 0;
        int step_;

        std::vector<Color> pixels_;
    };

}  // namespace img_lib