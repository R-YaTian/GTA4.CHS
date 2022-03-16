#pragma once
#include "../common/stdinc.h"
#include "../common/common.h"

#if PLUGIN_USE_FREETYPE
#include "texture_sprite.h"
#include "grcTexturePC.h"

struct SSimpleImage
{
    unsigned width, height, stride;
    uchar* pixels;
};

struct SCharData
{
    unsigned texture_index; //是第几张纹理
    unsigned texture_x, texture_y; //字符矩形在纹理中的左上点坐标
    unsigned sprite_width, sprite_height; //freetype渲染出的大小

    //字体本身包含的绘制时定位信息
    int bitmap_top, bitmap_left;
    FT_Vector glyph_advance;
};

class CCharRenderer
{
public:
    static constexpr int FTPixelSize = 60;
    static constexpr int TextureWidth = 4096;
    static constexpr int TextureHeight = 4096;

    static float Fix26_6ToFloat(long fix_point);
    static float Fix16_16ToFloat(long fix_point);

    CCharRenderer();
    ~CCharRenderer();

    CCharRenderer(const CCharRenderer&) = delete;
    CCharRenderer& operator=(const CCharRenderer&) = delete;

    bool FTInitialized() const;
    bool FaceLoaded() const;
    void CloseFT();

    void SetTextureVirtualTable(void* v);

    bool LoadFont(const std::filesystem::path& filename, int face_index = 0);
    void UnloadFont();

    void CacheChars(const std::vector<char32_t>& chars); //将所有字符添加到字库纹理中
    const SCharData& GetCharData(GTAChar chr) const;

    grcTexturePC* GetGameTexture(unsigned index);

private:
    FT_Library m_ftLibrary;
    FT_Face m_ftFace;
    bool m_isPRCMap; //指示字体的字符集是简体中文还是unicode

    std::unordered_map<GTAChar, SCharData> m_charData; //字符信息

    void* m_textureVirtualTable;

    std::vector<grcTexturePC> m_gameTextures;        //grcTexturePC对象
    std::vector<std::vector<uchar>> m_textureAlphaPixels; //grcTexturePC的Alpha像素信息
    std::vector<std::vector<uchar>> m_textureDXT5Pixels; //grcTexturePC的DXT像素信息

    bool InitializeFT();
    static GTAChar Unicode2GB2312(GTAChar unic);
    bool RenderChar(GTAChar chr);

    //生成grcTexturePC对象
    void GenerateGameTexture(const std::vector<uchar>& pixels, unsigned width, unsigned height);
};

#endif
