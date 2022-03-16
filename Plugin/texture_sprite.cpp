#include "texture_sprite.h"

TextureSprite::TextureSprite(int width, int height)
    :m_TextureWidth(width), m_TextureHeight(height), m_FreeX(PadSize), m_FreeY(PadSize), m_LineHeight(0), m_TextureIndex(0)
{

}

TextureSprite::AppendResult TextureSprite::AppendSprite(int sprite_width, int sprite_height)
{
    if (
        sprite_width < 1 ||
        sprite_height < 1 ||
        sprite_height + 2 * PadSize > m_TextureHeight ||
        sprite_width + 2 * PadSize > m_TextureWidth
        )
    {
        throw std::invalid_argument("Wrong data.");
    }

    AppendResult result;

    //先判断宽度够不够
    if (m_TextureWidth - m_FreeX - PadSize < sprite_width)
    {
        //新开一行
        m_FreeX = PadSize;
        m_FreeY += m_LineHeight;
        m_FreeY += PadSize;
        m_LineHeight = 0;
    }

    //然后判断高度够不够
    if (m_TextureHeight - m_FreeY - PadSize < sprite_height)
    {
        //添加到下一张纹理
        ++m_TextureIndex;
        m_FreeX = PadSize;
        m_FreeY = PadSize;
        m_LineHeight = 0;
    }
    
    result.texture_index = m_TextureIndex;
    result.x = m_FreeX;
    result.y = m_FreeY;

    m_LineHeight = std::max(m_LineHeight, sprite_height);
    m_FreeX += sprite_width;
    m_FreeX += PadSize;

    return result;
}
