/*
 *	Cloud Wu's JPEG decoder
 *
 *			2000/3/4 �� 1 ��
 *
 *		�����Ӯ�����ʵ�����ʹ��, �����ʹ�ô˴����ȫ���򲿷�
 *		������ Cloud Wu (�Ʒ�)
 *
 *		��ҵʹ����������ֱ����ϵ
 *
 *		www.codingnow.com
 *		cloudwu@263.net
 *	
 *		JPEG �ڲ�ͷ�ļ�
 */

#ifndef _JPEG_
#define _JPEG_

// һ��ɫ������� (��ʱ�����޸�)
typedef unsigned short int PIXEL;  // short int

// ��Ϸ����ʹ�õĲ�ѹ����λͼ�ṹ
typedef struct {
	int w,h,pitch;      //λͼ��͸��Լ�ÿ��ʵ���ֽ���
	int cl,ct,cr,cb;    //λͼ���þ��ε����ϽǶ�������
	//���þ��εĿ�͸�
	PIXEL *line[1];     //����λͼʱ��̬�����С
} BMP;
typedef BMP* lpBMP;

BMP *load_jpg(char *filename);
BMP *Unpak_jpg(unsigned char *inbuf,unsigned int insize);
BMP *Unpak_mapx(unsigned char *inbuf, unsigned int insize);
unsigned char * MapHandler(unsigned char* Buffer, unsigned int inSize, unsigned int* outSize);
int DecompressMask(void* in, void* out);
void ByteSwap(unsigned short& value);

extern unsigned char *rgb_data;
extern unsigned char *compressed_data;
extern unsigned char *whole_data;
// �ͷŷ�ѹ����λͼ
#define destroy_bitmap(a) free(a)

int init_jpeg(void);
#define free_jpeg NULL
#define active_jpeg NULL

#endif