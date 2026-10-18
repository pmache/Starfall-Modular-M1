#pragma once
#include "raylib.h"
#include <algorithm>
#include <cstdint>

// Original 5x7 bitmap lettering, one-pixel stems and a full pixel between glyphs.
// Fixed integer scaling avoids raylib's small-size default-font resampling.
namespace ui {
struct Glyph { char character; std::uint8_t rows[7]; };
inline constexpr Glyph glyphs[] = {
    {'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
    {'C',{14,17,16,16,16,17,14}}, {'D',{30,17,17,17,17,17,30}},
    {'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
    {'G',{14,17,16,23,17,17,15}}, {'H',{17,17,17,31,17,17,17}},
    {'I',{14,4,4,4,4,4,14}}, {'J',{7,2,2,2,18,18,12}},
    {'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
    {'M',{17,27,21,21,17,17,17}}, {'N',{17,25,25,21,19,19,17}},
    {'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}},
    {'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
    {'S',{15,16,16,14,1,1,30}}, {'T',{31,4,4,4,4,4,4}},
    {'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}},
    {'W',{17,17,17,21,21,21,10}}, {'X',{17,17,10,4,10,17,17}},
    {'Y',{17,17,10,4,4,4,4}}, {'Z',{31,1,2,4,8,16,31}},
    {'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}},
    {'2',{14,17,1,2,4,8,31}}, {'3',{30,1,1,14,1,1,30}},
    {'4',{2,6,10,18,31,2,2}}, {'5',{31,16,16,30,1,1,30}},
    {'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,8,8,8}},
    {'8',{14,17,17,14,17,17,14}}, {'9',{14,17,17,15,1,1,14}},
    {'/',{1,1,2,4,8,16,16}}, {'-',{0,0,0,31,0,0,0}},
    {'.',{0,0,0,0,0,12,12}}, {':',{0,12,12,0,12,12,0}},
    {'+',{0,4,4,31,4,4,0}}, {'!',{4,4,4,4,4,0,4}},
    {'?',{14,17,1,2,4,0,4}}, {' ',{0,0,0,0,0,0,0}}
};
inline const Glyph &glyph(char c) {
    if(c>='a'&&c<='z') c=static_cast<char>(c-'a'+'A');
    for(const Glyph &g:glyphs) if(g.character==c)return g;
    return glyphs[sizeof(glyphs)/sizeof(glyphs[0])-2];
}
inline int scaleFor(int size) { return std::max(1,size/8); }
inline int measure(const char *text,int size) {
    int length=0;for(;text[length];++length){}
    return length ? (length*6-1)*scaleFor(size):0;
}
inline void text(const char *value,int x,int y,int size,Color color) {
    const int scale=scaleFor(size);
    for(int i=0;value[i];++i) {
        const Glyph &g=glyph(value[i]);
        for(int row=0;row<7;++row)for(int col=0;col<5;++col)
            if(g.rows[row]&(1<<(4-col)))DrawRectangle(x+(i*6+col)*scale,y+row*scale,scale,scale,color);
    }
}
} // namespace ui
