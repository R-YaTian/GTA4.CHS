#pragma once
#include "../common/stdinc.h"

//包含向字库贴图添加字符图时计算坐标的逻辑
class TextureSprite
{
public:
    static constexpr int PadSize = 1; //图片之间留出的像素宽度

    struct AppendResult
    {
        unsigned texture_index; //原图满了，添加到下一张贴图则为true
        int x, y; //被添加的字符在贴图中的位置
    };

    TextureSprite(int width, int height);

    //向大图里增加字符小图，返回添加到的位置
    AppendResult AppendSprite(int sprite_width, int sprite_height);

private:
    int m_TextureWidth, m_TextureHeight;
    int m_FreeX, m_FreeY; //下一个能添加的位置
    int m_LineHeight; //当前行的高度
    unsigned m_TextureIndex;   //指示添加到第几张纹理
};
