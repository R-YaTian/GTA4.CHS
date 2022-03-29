#include "font.h"
#include "plugin.h"
#include "gta_string.h"

static const float fChsWidth = 32.0f;
static const float fChsWidthFix = -1.0f;
static const float fSpriteWidth = 64.0f;
static const float fSpriteHeight = 80.0f;
static const float fTextureResolution = 4096.0f;
static const float fTextureRowsCount = 51.2f;
static const float fTextureColumnsCount = 64.0f;
static const float fRatio = 4.0f;

static grcTexturePC* pChsTexture;

const GTAChar* CFont::SkipWord_Prolog(std::uintptr_t address)
{
    auto ptr = reinterpret_cast<const GTAChar*>(address & 0x7FFFFFFF);

    if ((address & 0x80000000) == 0)
    {
        ptr = SkipWord(ptr);
    }

    return ptr;
}

bool CFont::IsSpecialPunctuationMark(GTAChar chr)
{
#if 0
    return false;
#else
    //—、。《》「」『』！，－：；？～
    return
        //chr == L'—' ||
        chr == L'、' ||
        chr == L'。' ||
        //chr == L'《' ||
        chr == L'》' ||
        //chr == L'「' ||
        chr == L'」' ||
        //chr == L'『' ||
        chr == L'』' ||
        chr == L'！' ||
        chr == L'，' ||
        //chr == L'－' ||
        chr == L'：' ||
        chr == L'；' ||
        chr == L'？' ||
        chr == L'～' ||
        chr == L'…'
        ;
#endif
}

void CFont::SkipSpecialPunctuationMarks(const GTAChar*& str)
{
    while (IsSpecialPunctuationMark(*str))
    {
        ++str;
    }
}

void CFont::AddSpecialPunctuationMarksWidth(const GTAChar*& str, float* width)
{
    while (IsSpecialPunctuationMark(*str))
    {
        *width += GetCharacterSizeNormalDispatch(*str++ - 0x20);
    }
}

void* __fastcall CFont::LoadTextureCB(void* pDictionary, int, uint hash)
{
    void* result = plugin.game.Dictionary_grcTexturePC_GetElementByKey(pDictionary, hash);

    pChsTexture = plugin.game.Dictionary_grcTexturePC_GetElementByKey(pDictionary, plugin.game.Hash_HashStringFromSeediCase("font_chs"));

#if PLUGIN_USE_FREETYPE
    //预先渲染文本中出现的字符
    plugin.renderer.SetTextureVirtualTable(plugin.game.game_addr.pTexturePCVirtualTable);
    plugin.renderer.CacheChars(plugin.char_table.GetChars());
#endif

    return result;
}

const GTAChar* CFont::SkipWord(const GTAChar* str)
{
    if (str == nullptr)
    {
        return str;
    }

    auto begin = str;
    auto current = str;

    while (true)
    {
        GTAChar chr = *current;

        if (chr == ' ' || chr == '~' || chr == 0)
        {
            break;
        }

        if (!IsNativeChar(chr))
        {
            if (current == begin)
            {
                ++current;
            }

            break;
        }
        else
        {
            ++current;
        }
    }

    return current;
}

const GTAChar* CFont::SkipSpaces(const GTAChar* text)
{
    if (text == nullptr)
        return text;

    while (*text == ' ')
    {
        ++text;
    }

    return text;
}

float CFont::GetMaxWordWidth(const GTAChar* text)
{
    if (text == nullptr)
        return 0.0f;

    float max_word_width = 0.0f;

    while (*text != 0)
    {
        float word_width = GetStringWidthRemake(text, false);

        if (word_width > max_word_width)
            max_word_width = word_width;

        text = SkipSpaces(SkipWord(text));
    }

    return max_word_width;
}

float CFont::GetStringWidthRemake(const GTAChar* str, bool get_all)
{
    float current_width = 0.0f, max_width = 0.0f;
    bool had_word = false;
    auto render_index = plugin.game.Font_GetRenderIndex();
    auto& using_details = plugin.game.game_addr.pFont_Details[render_index];

    TokenStruct token_data;
    GTAChar token_string[64];

    if (str == nullptr)
    {
        return 0.0f;
    }

    while (true)
    {
        auto chr = *str;

        if (chr == 0)
        {
            break;
        }
        else if (chr == ' ')
        {
            if (!get_all)
                break;
        }
        else if (!IsNativeChar(chr))
        {
            //汉字
            //有可能是英语单词+汉字，要断开
            if (had_word && !get_all)
            {
                break;
            }
        }
        else if (chr == '~')
        {
            //token
            if (had_word && !get_all)
                break;

            //91BEC3
            token_string[0] = 0;

            int token_type = plugin.game.Font_ParseToken(++str, token_string, &token_data);

            if (token_data.f110 == 0)
            {
                //91C0B4
                if (token_type < 256 || token_type > 300)
                {
                    //91C116
                    if (gta_string::gtaWcslen(token_string) > 0)
                    {
                        //91C122
                        plugin.game.Font_AddTokenStringWidth(token_string, &current_width, render_index);
                        had_word = true;
                    }
                }
                else
                {
                    //91C0C3
                    current_width += plugin.game.game_addr.pFont_ButtonWidths[token_type - 255]
                        * using_details.fBlipScaleX;
                    had_word = true;
                }
            }
            else
            {
                //91BF11
                for (int token_index = 0; token_index < 4; ++token_index)
                {
                    switch (token_data.a110[token_index])
                    {
                    case 1:
                    {
                        //91BF26
                        current_width += plugin.game.game_addr.pFont_ButtonWidths[token_data.f0[token_index] - 255]
                            * using_details.fBlipScaleX;
                        had_word = true;

                        break;
                    }

                    case 2:
                    {
                        //91BF53
                        plugin.game.Font_AddTokenStringWidth(token_data.f10[token_index], &current_width, render_index);
                        had_word = true;
                        break;
                    }

                    default:
                    {
                        break;
                    }
                    }
                }
            }

            //91BF9E
            if (token_type >= 1000)
            {
                //91BFA6
                current_width += *plugin.game.game_addr.pFont_BlipWidth
                    * using_details.fBlipScaleX;
                had_word = true;
            }

            //91BFD3
            while (*str != '~')
            {
                //91BFE0
                if (*str == 'n')
                {
                    max_width = std::max(max_width, current_width);
                    current_width = 0.0f;
                }

                ++str;
            }

            //91C013
            //跳过后边的'~'
            ++str;

            //处理token后立即判断分词，配合ProcessString逻辑
            if (!get_all)
            {
                break;
            }

            continue;
        }

        //累加字符宽度
        current_width += GetCharacterSizeNormalDispatch(chr - 0x20);
        had_word = true;
        ++str;

        //计算汉字宽度之后立即判断一次分词
        if (!IsNativeChar(chr) && !get_all)
        {
            break;
        }
    }

    return std::max(current_width, max_width);
}

float CFont::GetCHSCharacterSizeNormal()
{
    auto render_index = plugin.game.Font_GetRenderIndex();
    auto& using_details = plugin.game.game_addr.pFont_Details[render_index];
    auto& using_font_data = plugin.game.game_addr.pFont_Datas[using_details.nFont];

    float extra_width = using_font_data.fWidthOfSpaceBetweenChars[using_details.nExtraWidthIndex];

    return (((fChsWidth + fChsWidthFix + extra_width) / *plugin.game.game_addr.pFont_ResolutionX + using_details.fEdgeSize2) *
        using_details.fScaleX);
}

float CFont::GetCharacterSizeNormalDispatch(GTAChar chr)
{
    if (IsNativeChar(chr + 0x20))
    {
        return plugin.game.Font_GetCharacterSizeNormal(chr);
    }
    else
    {
#if PLUGIN_USE_FREETYPE
        if (plugin.renderer.FaceLoaded())
        {
            return GetCHSCharacterSizeNormalFreetype(chr + 0x20);
        }
        else
        {
#endif

            return GetCHSCharacterSizeNormal();

#if PLUGIN_USE_FREETYPE
        }
#endif
}
}

float CFont::GetCHSCharacterSizeDrawing(bool use_extra_width)
{
    float extra_width = 0.0f;

    auto render_state = plugin.game.game_addr.pFont_RenderState;
    auto& using_font_data = plugin.game.game_addr.pFont_Datas[render_state->nFont];

    if (use_extra_width)
    {
        extra_width = using_font_data.fWidthOfSpaceBetweenChars[render_state->nExtraWidthIndex];
    }

    return ((fChsWidth + fChsWidthFix + extra_width) / *plugin.game.game_addr.pFont_ResolutionX + render_state->fEdgeSize) *
        render_state->fScaleX;
}

float CFont::GetCharacterSizeDrawingDispatch(GTAChar chr, bool use_extra_width)
{
    if (IsNativeChar(chr + 0x20))
    {
        return plugin.game.Font_GetCharacterSizeDrawing(chr, use_extra_width);
    }
    else
    {

#if PLUGIN_USE_FREETYPE
        if (plugin.renderer.FaceLoaded())
        {
            return GetCHSCharacterSizeDrawingFreetype(chr + 0x20, use_extra_width);
        }
        else
        {
#endif

            return GetCHSCharacterSizeDrawing(use_extra_width);

#if PLUGIN_USE_FREETYPE
        }
#endif
    }
}

void CFont::PrintCHSChar(float x, float y, GTAChar chr)
{
    CRect screen_rect, texture_rect;

    auto [row, column] = plugin.char_table.GetCharPos(chr);
    auto render_state = plugin.game.game_addr.pFont_RenderState;

    float texture_sprite_width = fSpriteWidth / fTextureResolution;

    float screen_character_width = (fChsWidth / *plugin.game.game_addr.pFont_ResolutionX + render_state->fEdgeSize) *
        render_state->fScaleX;

    float screen_character_height = render_state->fScaleY * 0.06558f;

    screen_rect.bottom_left.x = x;
    screen_rect.bottom_left.y = y + screen_character_height;
    screen_rect.top_right.x = x + screen_character_width;
    screen_rect.top_right.y = y;

    texture_rect.top_right.y = (row - 0.045f) * fSpriteHeight / fTextureResolution + 8.0f / fTextureResolution;
    if (texture_rect.top_right.y > 1.0f)
    {
        texture_rect.top_right.y = 1.0f;
    }
    texture_rect.bottom_left.y = (row - 0.045f) * fSpriteHeight / fTextureResolution + 79.0f / fTextureResolution - 0.001f / fRatio + 0.0048f / fRatio;
    texture_rect.bottom_left.x = column / fTextureColumnsCount;
    texture_rect.top_right.x = column / fTextureColumnsCount + texture_sprite_width;

    switch (render_state->nFont)
    {
    case 0:
    case 1:
    case 3:
        plugin.game.Graphics_SetRenderState(pChsTexture);
        break;

    default:
        break;
    }

    plugin.game.Font_Render2DPrimitive(&screen_rect, &texture_rect, render_state->field_18, false);
}

#if PLUGIN_USE_FREETYPE
void CFont::PrintCHSCharFreetype(float x, float y, GTAChar chr)
{
    auto& char_data = plugin.renderer.GetCharData(chr);
    auto render_state = plugin.game.game_addr.pFont_RenderState;

    //原版游戏实际截取的图块在64*80格子中的坐标是
    //x 0.0
    //y 4.4
    //w 64.0
    //h 78.4912

    //通过尝试得出的relative_char_rect在64*78.4912矩形中的偏移
    float x_delta = 0.0f;
    float y_delta = 60.0f;

    //先计算字符图片在64*78.4912矩形内的位置
    CRect relative_char_rect;
    relative_char_rect.bottom_left.x = static_cast<float>(char_data.bitmap_left) + x_delta;
    relative_char_rect.top_right.x = relative_char_rect.bottom_left.x + static_cast<float>(char_data.sprite_width);
    relative_char_rect.top_right.y = static_cast<float>(-char_data.bitmap_top) + y_delta;
    relative_char_rect.bottom_left.y = relative_char_rect.top_right.y + static_cast<float>(char_data.sprite_height);

    //截取纹理的位置
    CRect texture_rect;
    texture_rect.bottom_left.x = static_cast<float>(char_data.texture_x) / CCharRenderer::TextureWidth;
    texture_rect.bottom_left.y = static_cast<float>(char_data.texture_y + char_data.sprite_height) / CCharRenderer::TextureHeight;
    texture_rect.top_right.x = static_cast<float>(char_data.texture_x + char_data.sprite_width) / CCharRenderer::TextureWidth;
    texture_rect.top_right.y = static_cast<float>(char_data.texture_y) / CCharRenderer::TextureHeight;

    float old_screen_character_width = (fChsWidth / *plugin.game.game_addr.pFont_ResolutionX + render_state->fEdgeSize) *
        render_state->fScaleX;

    float old_screen_character_height = render_state->fScaleY * 0.06558f;

    //64*78.4912图块在屏幕上的位置
    CRect old_screen_rect;
    old_screen_rect.bottom_left.x = x;
    old_screen_rect.bottom_left.y = y + old_screen_character_height;
    old_screen_rect.top_right.x = x + old_screen_character_width;
    old_screen_rect.top_right.y = y;

    auto flt_proj = [](float old_lb, float old_ub, float old_val, float new_lb, float new_ub)
    {
        auto old_range = old_ub - old_lb;
        auto old_diff = old_val - old_lb;
        auto new_range = new_ub - new_lb;

        return old_diff / old_range * new_range + new_lb;
    };

    //将virtual_char_rect投影到old_screen_rect中，得到real_screen_rect
    CRect real_screen_rect;

    real_screen_rect.top_right.x = flt_proj(0.0f, 64.0f,
        relative_char_rect.top_right.x,
        old_screen_rect.bottom_left.x, old_screen_rect.top_right.x);

    real_screen_rect.top_right.y = flt_proj(0.0f, 78.4912f,
        relative_char_rect.top_right.y,
        old_screen_rect.top_right.y, old_screen_rect.bottom_left.y);

    real_screen_rect.bottom_left.x = flt_proj(0.0f, 64.0f,
        relative_char_rect.bottom_left.x,
        old_screen_rect.bottom_left.x, old_screen_rect.top_right.x);

    real_screen_rect.bottom_left.y = flt_proj(0.0f, 78.4912f,
        relative_char_rect.bottom_left.y,
        old_screen_rect.top_right.y, old_screen_rect.bottom_left.y);

    switch (render_state->nFont)
    {
    case 0:
    case 1:
    case 3:
        plugin.game.Graphics_SetRenderState(plugin.renderer.GetGameTexture(char_data.texture_index));
        break;

    default:
        break;
    }

    plugin.game.Font_Render2DPrimitive(&real_screen_rect, &texture_rect, plugin.game.game_addr.pFont_RenderState->field_18, false);
}

float CFont::GetCHSCharacterWidthFreetype(GTAChar chr)
{
    if (chr == 0x3000)
    {
        return fChsWidth;
    }
    else
    {
        return CCharRenderer::Fix26_6ToFloat(plugin.renderer.GetCharData(chr).glyph_advance.x) / 2.0f; //64像素对应32宽度，所以除以2
    }
}

float CFont::GetCHSCharacterSizeNormalFreetype(GTAChar chr)
{
    auto render_index = plugin.game.Font_GetRenderIndex();
    auto& using_details = plugin.game.game_addr.pFont_Details[render_index];
    auto& using_font_data = plugin.game.game_addr.pFont_Datas[using_details.nFont];

    float extra_width = using_font_data.fWidthOfSpaceBetweenChars[using_details.nExtraWidthIndex];

    return (((GetCHSCharacterWidthFreetype(chr) + extra_width) / *plugin.game.game_addr.pFont_ResolutionX +
        using_details.fEdgeSize2) * using_details.fScaleX);
}

float CFont::GetCHSCharacterSizeDrawingFreetype(GTAChar chr, bool use_extra_width)
{
    float extra_width = 0.0f;

    auto render_state = plugin.game.game_addr.pFont_RenderState;
    auto& using_font_data = plugin.game.game_addr.pFont_Datas[render_state->nFont];

    if (use_extra_width)
    {
        extra_width = using_font_data.fWidthOfSpaceBetweenChars[render_state->nExtraWidthIndex];
    }

    return (((GetCHSCharacterWidthFreetype(chr) + extra_width) / *plugin.game.game_addr.pFont_ResolutionX +
        render_state->fEdgeSize) * render_state->fScaleX);
}
#endif

void CFont::PrintCharDispatch(float x, float y, GTAChar chr, bool buffered)
{
    if (plugin.game.game_addr.pFont_RenderState->TokenType != 0 || IsNativeChar(chr + 0x20))
    {
        plugin.game.Font_PrintChar(x, y, chr, buffered);
    }
    else
    {
        if ((chr + 0x20) == 0x3000)
        {
            return;
        }

        if (y < -0.06558f || y > 1.0f)
        {
            return;
        }

#if PLUGIN_USE_FREETYPE
        if (plugin.renderer.FaceLoaded())
        {
            if (-(GetCHSCharacterSizeDrawingFreetype(chr + 0x20, true) / plugin.game.game_addr.pFont_RenderState->fScaleX) > x || x > 1.0f)
            {
                return;
            }

            PrintCHSCharFreetype(x, y, chr + 0x20);
        }
        else
        {
#endif

            if (-(GetCHSCharacterSizeDrawing(true) / plugin.game.game_addr.pFont_RenderState->fScaleX) > x || x > 1.0f)
            {
                return;
            }

            PrintCHSChar(x, y, chr + 0x20);

#if PLUGIN_USE_FREETYPE
        }
#endif
    }
}
