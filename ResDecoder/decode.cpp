/*
*	Cloud Wu's JPEG decoder
*
*			2000/6/19 第 2 版
*
*		允许非赢利性质的自由使用, 但如果使用此代码的全部或部分
*		请署上 Cloud Wu (云风)
*
*		商业使用请向作者直接联系
*
*		www.codingnow.com
*		cloudwu@263.net
*
*		DU 的解码
*/
#include "pch.h"
#include "jpegint.h"

// 每个 DU 里的数据按下面次序(倒序)重排列过, 得到原始次序

/*
BYTE jpeg_zigzag[64]={
0, 63,62,55,47,54,61,60,
53,46,39,31,38,45,52,59,
58,51,44,37,30,23,15,22,
29,36,43,50,57,56,49,42,
35,28,21,14,7, 6, 13,20,
27,34,41,48,40,33,26,19,
12,5, 4,11,18,25,32,24,
17,10,3,2, 9, 16,8, 1
};
*/

// ANN 算法需要转置矩阵

BYTE jpeg_zigzag[64] = {
	0,63,55,62,61,54,47,39,
	46,53,60,59,52,45,38,31,
	23,30,37,44,51,58,57,50,
	43,36,29,22,15,7,14,21,
	28,35,42,49,56,48,41,34,
	27,20,13,6,5,12,19,26,
	33,40,32,25,18,11,4,3,
	10,17,24,16,9,2,1,8
};

void jpeg_preprocess(BYTE *stream)
{
	__asm {
		mov esi, stream;
		mov edi, stream;
		cld;
		ALIGN 4
			_loop:
		lodsb;
		cmp al, 0xff;
		jz _getFF;
		stosb;
		jmp _loop;
	_getFF:
		lodsb;
		test al, al;
		jnz _not0;
		mov byte ptr[edi], 0xff;
		inc edi;
		jmp _loop;
	_not0:
		cmp al, 0xd9;	// 结束?
		jz _end;
		cmp al, 0xff;
		jz _getFF;
		// 去掉 RSTi 标记
		mov byte ptr[edi], 0xff;
		inc edi;
		//		stosb;
		jmp _loop;
		/*
		mov bl,al;
		and al,0xf8;
		cmp al,0xd0;
		jz _loop;
		mov byte ptr [edi],bl;
		inc edi;
		jmp _loop;
		*/
	_end:
	}
}

BYTE jpeg_bit, *jpeg_stream;

void jpeg_decode_DU(short *buf, int com)
{
	short *QTB = jpeg_qtable[jpeg_head.component[com].qtb];
	JPEG_HUFFMANTABLE *ACT = &jpeg_htable[jpeg_head.component[com].act],
		*DCT = &jpeg_htable[jpeg_head.component[com].dct];
	short jpeg_dc = jpeg_DC[com];

	__asm {
		// DC 的解码
		mov edx, DCT;
		mov esi, jpeg_stream;
		mov edi, QTB;
		cld;
		mov ch, byte ptr[edx + JPEG_HUFFMANTABLE.num];
		mov edx, [edx]JPEG_HUFFMANTABLE.htb;

		mov cl, jpeg_bit;
		lodsd;
		mov ebx, eax;
		mov eax, [esi];
		bswap ebx;
		bswap eax;
		shld ebx, eax, cl;
	_loop_dct:
		mov cl, [edx]JPEG_HUFFMANCODE.len;
		xor eax, eax;
		shld eax, ebx, cl;
		cmp ax, [edx]JPEG_HUFFMANCODE.code;
		jz _decode_dct;
		add edx, 4;
		dec ch;
		jmp _loop_dct;
	_decode_dct:
		mov ch, cl;
		mov cl, jpeg_bit;	// 刚才用掉 cl 个bit
		shr ebx, cl;
		add cl, ch; // 一共用掉 bit 数
		cmp cl, 32;	// 用完 4 字节?
		jl _lt4byte_dc;
		sub cl, 32;
		lodsd;
		bswap eax;
		mov ebx, eax;
	_lt4byte_dc:
		mov eax, [esi];
		bswap eax;
		shld ebx, eax, cl;

		mov ch, cl;
		mov cl, [edx]JPEG_HUFFMANCODE.num;
		xor eax, eax;
		shld eax, ebx, cl;

		xchg ch, cl;
		shr ebx, cl;
		add cl, ch;
		xchg ch, cl;

		dec cl;
		bt eax, cl;
		jc _get_diff;
		inc cl;
		mov edx, 1;
		inc eax;
		shl edx, cl;
		sub eax, edx;
	_get_diff:
		add ax, jpeg_dc;
		mov edx, com;
		mov cl, ch;
		mov[jpeg_DC + edx * 2], ax;
		imul word ptr[edi];
		mov edx, buf;
		add edi, 2;
		mov[edx], ax;
		// AC 的解码. cl 是前面被使用掉的 bit 数, ebx 是上4byte 

		cmp cl, 32;	// 用完 4 字节?
		jl _lt4byte_dc0;
		sub cl, 32;
		lodsd;
		bswap eax;
		mov ebx, eax;
	_lt4byte_dc0:

		mov eax, [esi];
		bswap eax;
		shld ebx, eax, cl;

		mov jpeg_bit, cl;	//用掉 cl 个 bit

		mov cx, 63;	// 处理 63 个 AC

	_loop_decode_act:
		mov edx, ACT;
		bswap ecx;
		mov ch, byte ptr[edx + JPEG_HUFFMANTABLE.num];
		mov edx, [edx]JPEG_HUFFMANTABLE.htb;

	_loop_act:
		mov cl, [edx]JPEG_HUFFMANCODE.len;
		xor eax, eax;
		shld eax, ebx, cl;
		cmp ax, [edx]JPEG_HUFFMANCODE.code;
		jz _decode_act;
		add edx, 4;
		dec ch;
		jmp _loop_act;
	_decode_act:
		// [edx]JPEG_HUFFMANCODE.num 是接出来的码
		mov ch, cl;
		mov cl, jpeg_bit;	// 刚才用掉 cl 个bit
		shr ebx, cl;
		add cl, ch; // 一共用掉 bit 数
		cmp cl, 32;	// 用完 4 字节?
		jl _lt4byte;
		sub cl, 32;
		lodsd;
		bswap eax;
		mov ebx, eax;
	_lt4byte:
		mov eax, [esi];
		bswap eax;
		shld ebx, eax, cl;
		mov jpeg_bit, cl;//使用了下4byte的 cl bit

		movzx eax, byte ptr[edx]JPEG_HUFFMANCODE.num;
		bswap ecx;
		test ax, ax;
		jz _decode_end;

		ror eax, 4;
		cmp cx, ax;
		jz _decode_end;
		sub cx, ax;
		rol eax, 4;
		bswap ecx;
		mov cl, al;
		mov ch, al;
		and cl, 0xf;
		shr ch, 4;
		jz _skip_set0;

		movzx eax, ch;
		bswap ecx;
		//-----------
		lea edi, [edi + 2 * eax];
		add ax, cx;
		bswap ecx;
	_set0:
		movzx edx, [eax + jpeg_zigzag];
		add edx, edx;
		add edx, buf;
		dec eax;
		dec ch;
		mov word ptr[edx], 0;
		jnz _set0;

	_skip_set0:
		xor eax, eax;
		shld eax, ebx, cl;
		dec cl;
		bt eax, cl;
		jc _get_ac;
		mov edx, 2;
		inc eax;
		shl edx, cl;
		sub eax, edx;
	_get_ac:
		inc cl;
		imul word ptr[edi];
		// dx==0 时, 正数,无进位
		test dx, dx;
		jz _2byte;
		// dx==0xffff 时, 负数,无进位
		inc dx;
		jz _2byte;

		mov ax, 0x7fff;
		bt dx, 15;
		adc ax, 0;
	_2byte:

		bswap ecx;
		movzx edx, cx;
		movzx edx, [edx + jpeg_zigzag];
		add edx, edx
			add edx, buf;
		add edi, 2;
		mov[edx], ax;

		bswap ecx;

		mov ch, cl;
		mov cl, jpeg_bit;
		shr ebx, cl;
		add cl, ch;	// 累计使用 bit 数
		cmp cl, 32;	// 用完 4 字节?
		jl _lt4byte_2;
		sub cl, 32;
		lodsd;
		bswap eax;
		mov ebx, eax;
	_lt4byte_2:
		mov eax, [esi];
		bswap eax;
		shld ebx, eax, cl;
		mov jpeg_bit, cl;//使用了下4byte的 cl bit
		bswap ecx;

		dec cx;
		jnz _loop_decode_act;
		jmp _end;
	_decode_end:
		movzx ecx, cx;
		xor eax, eax;
		mov edi, buf;
	_end_loop:
		movzx edx, [ecx + jpeg_zigzag];
		dec ecx;
		mov[edi + edx * 2], ax;
		jnz _end_loop;
	_end:
		sub esi, 4;
		mov jpeg_stream, esi;
	}
	// iDCT 解码
	jpeg_idct(buf);
}
