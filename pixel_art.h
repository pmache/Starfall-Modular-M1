#pragma once
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// Original, hand-authored pixel stencils. '.' is transparent. One cell = one
// logical 480x270 canvas pixel. No smoothing, external assets or random drawing.
namespace art {
inline constexpr const char *fighter[] = {
    "...........oo...................",
    "..........oSSo..................",
    ".........oSHHo..................",
    "........oSHHSo..................",
    "...oooooSHHSSoooo...............",
    "..oSSSSSHHSSSDDDDo..............",
    ".oDHHHHHHSSSSDDDSSoooo..........",
    "oDDDDSSSHHHHHHHHHHHHSSooo.......",
    "oDEEEDSSSHHHWWWWWHHHHHHSSoo....",
    "oDEWWEDSSHHWCCCCCHHHHHHHHHHooo.",
    "oDEEEDSSSHHHWWWWWHHHHHHSSoo....",
    "oDDDDSSSHHHHHHHHHHHHSSooo.......",
    ".oDSSSSSSSSSSDDDSSoooo..........",
    "..oDDDDDSSSSSDDDDo..............",
    "...oooooSSSSSoooo...............",
    "........oSDDSo..................",
    ".........oSDDo..................",
    "..........oDDo..................",
    "...........oo..................."
};
inline constexpr const char *hornet[] = {
    "..................oo......",
    "...............oooHSo.....",
    "............oooHHHSDo.....",
    "......ooooooHHHSSDDDo.....",
    "....ooSSSSSHHSSDDDoo......",
    "..ooSHHHHHSSSSDDoo........",
    ".oSHHHHSSSSSSSSDDDoo......",
    "oSHHWWHSSSDDDEEEDDDo......",
    "oHHWCCWHHSDDEWWEEDDo.....",
    "oSHHWWHSSSDDDEEEDDDo......",
    ".oSSSSSSSSSSSSSDDDoo......",
    "..ooSDDDDDSSSSDDoo........",
    "....ooDDDDSSSDDDDDoo......",
    "......ooooooSSSSDDDDo.....",
    "............oooSDDDDo.....",
    "...............oooDDo.....",
    "..................oo......"
};
inline constexpr const char *manta[] = {
    ".........oo...............",
    ".......ooHSo..............",
    ".....ooHHSSo..............",
    "...ooHHSSSSo..............",
    "..oHHSSDDDSSoo............",
    ".oHSSDDDDSSHHHooo.........",
    "oHSDDDSSHWWWHHHSSooo......",
    "oHSDSSHWWCCWWHHHSDEEoo....",
    "oHSSHWWCCCCWWHHSSDEWEo....",
    "oSSDDSSWWCCWWHSSDDEEoo....",
    ".oSDDDDSSWWWSSSSSooo......",
    "..oSDDDDDSSSSSooo.........",
    "...ooSDDDSSooo............",
    ".....ooSDDDo..............",
    ".......ooDDo..............",
    ".........oo..............."
};
inline constexpr const char *bomber[] = {
    "........oooooooo.........",
    "......ooSHHHHHSDo........",
    ".....oSHHSSSSSSDDo.......",
    "....oSHSSDDDDSSDDo.......",
    "...oSHSSDooDDSSDDDo......",
    "..oSHSSDo..oDSSDEEDo.....",
    ".oSHHHSSooooDSSDEWEDo....",
    "oSHWWWHHSSSSSSSDEEDDo....",
    "oHWCCCWHHSSSSSSDDDDo.....",
    "oHWCCCWHHSSSSSSDDDDo.....",
    "oSHWWWHHSSSSSSSDEEDDo....",
    ".oSSSSSSooooDSSDEWEDo....",
    "..oSSSDDo..oDSSDEEDo.....",
    "...oSSDDDooDDSSDDDo......",
    "....oSSDDDDDDSSDDo.......",
    ".....oSSSDDDDDDDDo.......",
    "......ooSSSSDDDDo........",
    "........oooooooo........."
};
inline constexpr const char *crown[] = {
    "......................oooooo............................",
    "....................ooHHHHSSoo..........................",
    "..................ooHHSSDDDDSSooo.......................",
    ".................oHHSSDDoooDDSSHHooo....................",
    "...............ooHHSSDDDo.oDDSSHHHSSooo.................",
    ".............ooHHSSDDDDDo.oDDSSSSSSSSSDoo...............",
    "...........ooHHSSDDDDDDDo.oDDSSDDDDSSSSDDo..............",
    ".........ooHHSSSSSSDDDDDo.oDDSSDDDDoDDSSDo..............",
    "........oHHHHHHHHSSSDDDo..oDDSDDDo..oDSDo..............",
    ".......oHHWWWWWWWHHSSDo...oDDSDDo....oDDo..............",
    "......oHHWCCCCCWWHHSSDo...oDSSDo......oo...............",
    ".....oHHWCCCCCCWWHHSSSDoooDSSDo........................",
    ".....oHHWWWWWWWWWHHSSSSSSSSDo..........................",
    "......ooHHHHHHHHHHSSSSSSSSoo...........................",
    "........ooHHSSSSSSSSSSSSoo.............................",
    "..........ooSSSSSSSSSSoo...............................",
    ".........ooHHHHHHSSSSSSoo..............................",
    ".......ooHHWWWWWHHHSSSSSSoo............................",
    "......oHHWWWWWWWWWHHSSSSSSDo...........................",
    ".....oHHWWWCCCCWWWWHHSSSSSSDo..........................",
    "....oHHWWCCCSSSCCWWWHHSSSDDDDo.........................",
    "...oHHWWCCSSDDDSSCCWWHHSSDDEEEoo.......................",
    "..oHHWWCCSSDoooDDSSCWWHHSDDEWWEo......................",
    ".oHHWWCCSSDo...oDSSCWWHHSDDEWWEo......................",
    "oHHWWCCSSDo.....oDSSCWWHHSDDEWWEo.....................",
    ".oHHWWCCSSDo...oDSSCWWHHSDDEWWEo......................",
    "..oHHWWCCSSDoooDDSSCWWHHSDDEWWEo......................",
    "...oSSWWCCSSDDDSSCCWWHSSSDDEEEoo.......................",
    "....oSSWWCCCSSSCCWWWHSSSSDDDDo.........................",
    ".....oSSWWWCCCCWWWWHSSSSSSSDo..........................",
    "......oSSWWWWWWWWWHSSSSSSSDo...........................",
    ".......ooSSWWWWWSSSSSSSSSoo............................",
    ".........ooSSSSSSSSSSSSoo..............................",
    "..........ooSSSSSSSSSSoo...............................",
    "........ooSSSSSSSSSSSSSSoo.............................",
    "......ooSSSSSSSSSSSSSSSSSSoo...........................",
    ".....oSSWWWWWWWWWSSSSSSSSSSDo..........................",
    ".....oSSWCCCCCCWWSSSSSDoooDSSDo........................",
    "......oSSWCCCCCWWSSSSDo...oDSSDo......oo...............",
    ".......oSSWWWWWWWSSSSDo...oDDSDDo....oDDo..............",
    "........oSSSSSSSSSSSDDDo..oDDSDDDo..oDSDo..............",
    ".........ooSSSSSSSSDDDDDo.oDDSSDDDDoDDSSDo..............",
    "...........ooSSSSDDDDDDDo.oDDSSDDDDSSSSDDo..............",
    ".............ooSSSSDDDDDo.oDDSSSSSSSSSDoo...............",
    "...............ooSSSSDDDo.oDDSSSSSSSSooo.................",
    ".................oSSSSDDoooDDSSSSooo....................",
    "..................ooSSSSDDDDSSooo.......................",
    "....................ooSSSSSSoo..........................",
    "......................oooooo............................"
};

struct Palette { Color outline, dark, shade, highlight, white, core, engine; };
inline Palette playerPalette(bool powered) {
    return {{8,15,30,255},{25,42,67,255},{58,91,118,255},{129,176,184,255},
        {229,239,205,255}, powered ? Color{89,235,174,255}:Color{243,164,72,255},{255,105,68,255}};
}
inline Palette enemyPalette(int kind) {
    if (kind == 0) return {{23,12,29,255},{66,24,54,255},{137,44,77,255},{236,92,112,255},
        {255,223,169,255},{255,167,63,255},{192,72,119,255}};
    if (kind == 1) return {{9,21,34,255},{18,47,61,255},{28,94,106,255},{66,167,162,255},
        {188,243,204,255},{241,161,68,255},{68,175,194,255}};
    return {{24,18,33,255},{61,38,58,255},{128,81,73,255},{208,147,97,255},
        {255,224,168,255},{246,106,89,255},{224,131,77,255}};
}
inline Color color(char c, const Palette &p) {
    switch(c) {
    case 'o': return p.outline; case 'D': return p.dark; case 'S': return p.shade;
    case 'H': return p.highlight; case 'W': return p.white; case 'C': return p.core;
    default: return p.engine;
    }
}
template<std::size_t N>
inline void sprite(const char *const (&rows)[N], int x, int y, const Palette &palette, int scale=1) {
    for (std::size_t row=0; row<N; ++row)
        for (int col=0; rows[row][col]; ++col)
            if (rows[row][col]!='.') DrawRectangle(x+col*scale, y+static_cast<int>(row)*scale, scale, scale, color(rows[row][col],palette));
}
template<std::size_t N>
inline bool validStencil(const char *const (&rows)[N]) {
    int opaque=0;
    for(const char *row:rows) {
        if(std::strlen(row)>64) return false;
        for(int i=0;row[i];++i) {
            if(!std::strchr(".oDSHWC E",row[i]) || row[i]==' ') return false;
            opaque += row[i]!='.';
        }
    }
    return N<=64 && opaque>100;
}
inline void flame(int x, int y, float time, Color edge, bool left) {
    const int length=7+(static_cast<int>(time*18)%3)*3;
    const int direction=left ? -1:1;
    for(int dx=0;dx<length;++dx) {
        const int half=dx<3?2:dx<7?1:0;
        for(int dy=-half;dy<=half;++dy)
            DrawPixel(x+direction*dx,y+dy,dx<4&&dy==0?Color{255,240,182,255}:edge);
    }
}
inline void ship(Vector2 p,bool powered,float time) {
    const int x=static_cast<int>(std::round(p.x)),y=static_cast<int>(std::round(p.y));
    flame(x-15,y,time,{255,133,66,255},true);
    sprite(fighter,x-15,y-9,playerPalette(powered));
}
inline void enemy(Vector2 p,int kind,bool carrier,float time) {
    const int x=static_cast<int>(std::round(p.x)),y=static_cast<int>(std::round(p.y));
    flame(x+9,y,time,{204,72,111,255},false);
    const Palette palette=enemyPalette(kind);
    if(kind==0) sprite(hornet,x-12,y-8,palette);
    else if(kind==1) sprite(manta,x-12,y-8,palette);
    else sprite(bomber,x-12,y-9,palette);
    if(carrier) {
        const Color c={121,244,156,255};
        DrawLine(x-15,y-12,x-11,y-12,c);DrawLine(x-15,y-12,x-15,y-8,c);
        DrawLine(x+15,y+12,x+11,y+12,c);DrawLine(x+15,y+12,x+15,y+8,c);
    }
}
inline void boss(Vector2 p,float time) {
    const int x=static_cast<int>(std::round(p.x)),y=static_cast<int>(std::round(p.y));
    Palette palette={{11,17,33,255},{22,35,56,255},{48,72,94,255},{87,131,149,255},
        {234,196,146,255},{219,83,75,255},{123,174,159,255}};
    flame(x+29,y-30,time,{215,104,72,255},false);
    flame(x+29,y+30,time+.12f,{215,104,72,255},false);
    sprite(crown,x-44,y-48,palette,2);
    const int pulse=static_cast<int>(time*6)%3;
    // Recessed central reactor, animated independently of the armored body.
    for(int dy=-5;dy<=5;++dy)for(int dx=-5;dx<=5;++dx) {
        const int d=dx*dx+dy*dy;
            if(d<=20)DrawRectangle(x-12+dx*2,y+dy*2,2,2,d<5+pulse?Color{255,235,178,255}:Color{242,115,69,255});
    }
}
inline unsigned hash(unsigned value) {
    value ^= value >> 16; value *= 0x7feb352du; value ^= value >> 15;
    value *= 0x846ca68bu; return value ^ (value >> 16);
}
inline void backdrop(float time) {
    ClearBackground({6,10,23,255});
    // Low-contrast dithered gas bands, far behind gameplay and stars.
    for(int y=18;y<270;y+=2)for(int x=0;x<480;x+=2) {
        const float band=std::sin(x*.009f+y*.018f)+std::cos(x*.017f-y*.012f);
        const unsigned noise=hash(static_cast<unsigned>(x+y*480));
        if(band>0.6f&&(noise%5)<3)DrawPixel(x,y,{17,24,45,255});
        else if(band<-.8f&&(noise%4)==0)DrawPixel(x,y,{23,20,43,255});
    }
    // Shaded ice moon. All shading is quantized to a deliberately small palette.
    const int cx=369-static_cast<int>(time*0.6f)%36, cy=92, radius=64;
    for(int y=-radius;y<=radius;++y)for(int x=-radius;x<=radius;++x) {
        const int distance=x*x+y*y;if(distance>radius*radius)continue;
        const unsigned noise=hash(static_cast<unsigned>((x+radius)+(y+radius)*129));
        Color c={18,27,42,255};
        const float lit=-x*.65f-y*.4f+std::sqrt(static_cast<float>(radius*radius-distance))*.45f;
        if(lit>48)c={40,58,74,255};else if(lit>28)c={31,46,63,255};else if(lit>8)c={23,36,52,255};
        if((noise%23)==0&&lit>0)c={32,48,65,255};
        if(distance>(radius-2)*(radius-2)&&lit>3)c={55,79,95,255};
        // Broad shaded impact basins instead of random high-contrast static.
        for(const Vector2 crater:{Vector2{-27,-20},Vector2{6,22},Vector2{19,-28}}) {
            const float dx=x-crater.x,dy=y-crater.y;
            if(dx*dx+dy*dy<90&&lit>0)c={30,45,59,255};
            if(dx*dx+dy*dy<50&&lit>0)c={21,34,48,255};
        }
        DrawPixel(cx+x,cy+y,c);
    }
    // Two scrolling debris layers with independent speeds and silhouettes.
    for(int layer=0;layer<2;++layer)for(int i=0;i<8;++i) {
        const unsigned seed=hash(static_cast<unsigned>(i+layer*19));
        const int span=600;
        int x=static_cast<int>((seed%600)+600-static_cast<int>(time*(3+layer*8))%span)%span-60;
        const int y=layer==0?225+static_cast<int>((seed>>8)%34):30+static_cast<int>((seed>>8)%200);
        const int size=layer==0?13:3;
        const Color dark=layer==0?Color{17,29,43,255}:Color{29,41,55,255};
        for(int dy=-size;dy<=size;++dy)for(int dx=-size;dx<=size;++dx)
            if(dx*dx+dy*dy<size*size&&((dx+dy+size*2)%9!=0))DrawPixel(x+dx,y+dy,dark);
        DrawLine(x-size/2,y-size/2,x+size/3,y-size/2,{38,53,66,255});
    }
}
inline void explosion(Vector2 p,float age,float size) {
    const int frame=std::min(6,static_cast<int>(age*14));
    const float radius=(3+frame*2)*size;
    const int bound=static_cast<int>(radius+3);
    for(int dy=-bound;dy<=bound;++dy)for(int dx=-bound;dx<=bound;++dx) {
        const float d=std::sqrt(static_cast<float>(dx*dx+dy*dy));
        const float angle=std::atan2(static_cast<float>(dy),static_cast<float>(dx));
        const float edge=radius+(std::sin(angle*7+frame)*1.8f);
        if(d>edge|| (frame>2&&d<radius*(frame-2)*.16f))continue;
        if(frame>4&&hash(static_cast<unsigned>((dx+bound)+(dy+bound)*73))%3==0)continue;
        const Color c=d<edge*.4f?Color{255,244,194,255}:d<edge*.68f?Color{255,192,92,255}:
            d<edge*.86f?Color{246,103,62,255}:Color{151,48,65,255};
        DrawPixel(static_cast<int>(p.x)+dx,static_cast<int>(p.y)+dy,c);
    }
}
} // namespace art
