#include "renderer.h"

#if PLUGIN_USE_FREETYPE

CCharRenderer::CCharRenderer()
    :m_ftLibrary(nullptr),
    m_ftFace(nullptr),
    m_isPRCMap(false)
{
    InitializeFT();
}

CCharRenderer::~CCharRenderer()
{
    CloseFT();
}

bool CCharRenderer::InitializeFT()
{
    FT_Library library;

    if (FT_Init_FreeType(&library) == 0)
    {
        m_ftLibrary = library;
        return true;
    }
    else
    {
        return false;
    }
}

GTAChar CCharRenderer::Unicode2GB2312(GTAChar unic)
{
    wchar_t wide_buffer[2];
    char narrow_buffer[10];
    char default_char = 0;

    BOOL usedDefaultChar;

    wide_buffer[0] = unic;
    wide_buffer[1] = 0;
    std::fill(std::begin(narrow_buffer), std::end(narrow_buffer), 0);

    auto length = WideCharToMultiByte(936, 0, wide_buffer, -1, narrow_buffer, 9, &default_char, &usedDefaultChar);

    if (length == 2)
    {
        return narrow_buffer[0];
    }
    else if (length == 3)
    {
        narrow_buffer[1] = std::exchange(narrow_buffer[0], narrow_buffer[1]);

        return *reinterpret_cast<GTAChar*>(narrow_buffer);
    }
    else
    {
        return 0;
    }
}

bool CCharRenderer::FTInitialized() const
{
    return m_ftLibrary != nullptr;
}

bool CCharRenderer::FaceLoaded() const
{
    return m_ftFace != nullptr;
}

void CCharRenderer::CloseFT()
{
    if (FTInitialized())
    {
        FT_Done_FreeType(m_ftLibrary);

        m_ftFace = nullptr;
        m_ftLibrary = nullptr;
        m_charData.clear();
        m_isPRCMap = false;
    }
}

void CCharRenderer::SetTextureVirtualTable(void* v)
{
    m_textureVirtualTable = v;
}

bool CCharRenderer::LoadFont(const std::filesystem::path& filename, int face_index)
{
    if (!FTInitialized())
        return false;

    UnloadFont();

    FT_Face face;

    if (FT_New_Face(m_ftLibrary, filename.string().c_str(), face_index, &face) == 0)
    {
        //要求是可缩放字体
        if ((face->face_flags & FT_FACE_FLAG_SCALABLE) == 0)
        {
            FT_Done_Face(face);
            return false;
        }

        //检查是否正确选择了FT_ENCODING_UNICODE，否则选择FT_ENCODING_PRC，两者都失败则作为加载失败
        if (face->charmap == nullptr || face->charmap->encoding != FT_ENCODING_UNICODE)
        {
            if (FT_Select_Charmap(face, FT_ENCODING_PRC) == 0)
            {
                m_isPRCMap = true;
            }
            else
            {
                FT_Done_Face(face);
                return false;
            }
        }
        else
        {
            m_isPRCMap = false;
        }

        m_ftFace = face;
        FT_Set_Pixel_Sizes(m_ftFace, FTPixelSize, FTPixelSize);
        return true;
    }
    else
    {
        return false;
    }
}

void CCharRenderer::UnloadFont()
{
    if (FaceLoaded())
    {
        FT_Done_Face(m_ftFace);
        m_ftFace = nullptr;
    }
}

void CCharRenderer::CacheChars(const std::vector<char32_t>& chars)
{
    //不允许重复生成
    if (!m_textureAlphaPixels.empty() || !m_gameTextures.empty() || !m_charData.empty())
    {
        return;
    }

    TextureSprite sprite_info(TextureWidth, TextureHeight);

    for (auto chr : chars)
    {
        if (!RenderChar(chr))
        {
            continue;
        }

        auto bitmap_pixels = m_ftFace->glyph->bitmap.buffer;
        auto bitmap_width = m_ftFace->glyph->bitmap.width;
        auto bitmap_height = m_ftFace->glyph->bitmap.rows;
        auto bitmap_stride = std::abs(m_ftFace->glyph->bitmap.pitch);
        auto bitmap_top = m_ftFace->glyph->bitmap_top;
        auto bitmap_left = m_ftFace->glyph->bitmap_left;
        auto glyph_advance = m_ftFace->glyph->advance;

        auto append_result = sprite_info.AppendSprite(bitmap_width, bitmap_height);

        SCharData char_data;

        char_data.texture_index = append_result.texture_index;
        char_data.texture_x = append_result.x;
        char_data.texture_y = append_result.y;
        char_data.sprite_width = bitmap_width;
        char_data.sprite_height = bitmap_height;
        char_data.bitmap_top = bitmap_top;
        char_data.bitmap_left = bitmap_left;
        char_data.glyph_advance = glyph_advance;

        m_charData.emplace(chr, char_data);

        // 将FreeType渲染图的像素复制到大图中
        while (m_textureAlphaPixels.size() < append_result.texture_index + 1)
        {
            m_textureAlphaPixels.emplace_back(TextureWidth * TextureHeight, 0);
        }

        auto texture_ptr = m_textureAlphaPixels[char_data.texture_index].data();

        for (unsigned row_index = 0; row_index < bitmap_height; ++row_index)
        {
            std::memcpy(texture_ptr + (row_index + char_data.texture_y) * TextureWidth + char_data.texture_x,
                bitmap_pixels + row_index * bitmap_stride,
                bitmap_width);
        }
    }

    for (auto& pixels : m_textureAlphaPixels)
    {
        GenerateGameTexture(pixels, TextureWidth, TextureHeight);
    }
}

const SCharData& CCharRenderer::GetCharData(GTAChar chr) const
{
    auto char_it = m_charData.find(chr);

    if (char_it == m_charData.end())
    {
        return m_charData.find(L'？')->second;
    }
    else
    {
        return char_it->second;
    }
}

void CCharRenderer::GenerateGameTexture(const std::vector<uchar>& pixels, unsigned width, unsigned height)
{
    grcTexturePC texture;

    //pixels只包含alpha通道，先转换成rgba图像，再进行压缩
    std::vector<uchar> rgba_pixels(pixels.size() * 4, 255u);
    std::vector<uchar> compressed_pixels(squish::GetStorageRequirements(width, height, squish::kDxt5));

    for (std::size_t i = 0; i < pixels.size(); ++i)
    {
        rgba_pixels[i * 4 + 3] = pixels[i];
    }

    squish::CompressImage(rgba_pixels.data(), width, height, compressed_pixels.data(), squish::kDxt5);

    texture.vtbl = m_textureVirtualTable;
    texture.m_wWidth = static_cast<ushort>(width);
    texture.m_wHeight = static_cast<ushort>(height);
    texture.m_wStride = static_cast<ushort>(width);

    m_textureDXT5Pixels.emplace_back(std::move(compressed_pixels));
    texture.m_pPixelData = m_textureDXT5Pixels.back().data();

    m_gameTextures.push_back(texture);
}

grcTexturePC* CCharRenderer::GetGameTexture(unsigned index)
{
    return &m_gameTextures[index];
}

bool CCharRenderer::RenderChar(GTAChar chr)
{
    GTAChar actual_code = chr;

    if (m_isPRCMap)
    {
        actual_code = Unicode2GB2312(chr);
    }

    return actual_code != 0 &&
        FT_Load_Char(m_ftFace, actual_code, FT_LOAD_RENDER) == 0 &&
        m_ftFace->glyph->bitmap.width > 0 &&
        m_ftFace->glyph->bitmap.rows > 0;
}

float CCharRenderer::Fix26_6ToFloat(long fix_point)
{
    return static_cast<float>(fix_point) / 64.0f;
}

float CCharRenderer::Fix16_16ToFloat(long fix_point)
{
    return static_cast<float>(fix_point) / 65536.0f;
}

#endif
