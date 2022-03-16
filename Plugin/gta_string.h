#pragma once
#include "../common/stdinc.h"
#include "../common/common.h"

namespace gta_string
{
    unsigned gtaWcslen(const GTAChar* str);

    //�浵����
    uchar* gtaTruncateString(uchar* dst, const GTAChar* src, unsigned size);
    void gtaExpandString(const uchar* src, GTAChar* dst);
    //��ʾ�浵������ʾʱ���ڸ�ʽ���Ĵ浵����

    void gtaExpandString2(GTAChar* dst, const uchar* src, int a8);
    //TBoGT��������ѡ�����
    void gtaExpandString3(GTAChar* dst, const uchar* src, int a8);

    //�ʼ�/��ҳ
    void gtaTruncateString2(const GTAChar* src, uchar* dst);
    void gtaExpandStringGxt(const uchar* src, GTAChar* dst);

    //���տ��ַ�������ת����utf8
    void __stdcall gtaMailAppendWideStringAsUtf8(int id, const GTAChar* str);

    uchar* gtaUTF8strncpy(uchar* dest, const uchar* source, unsigned size);
    void* gtaSpecialMemmove(uchar* dest, const uchar* source, unsigned size);

    unsigned gtaUTF8Strlen(const uchar* str);
}
