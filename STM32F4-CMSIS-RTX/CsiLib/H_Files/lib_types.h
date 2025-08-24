#ifndef _LIB_TYPES_H_
#define _LIB_TYPES_H_

#include "main.h"

typedef void (*LL_DMA_ClearFlag)(DMA_TypeDef *DMAx);
typedef uint32_t (*LL_DMA_IsActiveFlag)(DMA_TypeDef *DMAx);
typedef uint32_t (*LL_DMA_IsEnabledIT)(DMA_TypeDef *DMAx, uint32_t Stream);

typedef enum
{
    eDMA1 = 0,
    eDMA2,
    eDMAn,
}mico_dma_type;

typedef struct
{
    IRQn_Type           	irq_vector;
    uint8_t             	mpriority;
    uint8_t             	spriority;
    DMA_TypeDef         	*controller;    //DMA1/DMA2
	DMA_Stream_TypeDef  	*stream;        //DMA1_Stream0~DMA_Stream7
    uint32_t		  		stream_id;		//DMA1_Stream0~DMA_Stream7
    uint32_t            	channel;        //Channel0~Channel7
    LL_DMA_ClearFlag		clear_flags;
	LL_DMA_IsActiveFlag		isavtive_flags;
	LL_DMA_IsEnabledIT		isenable_it;
}dma_type;

typedef struct _filesystem_info_t
{
	int total_space;
	int free_space;
} filesystem_info;

typedef struct
{
    unsigned char event;
    unsigned char code;
    unsigned char value;
    unsigned char client;
}Event_Type;

typedef struct
{
	unsigned char event;
	unsigned char code;
	unsigned char address;			// ��ַ
	unsigned char function;			// ������
	unsigned short usRegAddr;		// �Ĵ�����ַ
	unsigned short usRegValue;		// �Ĵ�����ֵ
	unsigned short usRegCount;		// �Ĵ�������
}MRTU_Type;

typedef struct
{
	unsigned char event;
    unsigned char *rxbuf;
	unsigned char *txbuf;
	unsigned short rxlen;
	unsigned short txlen;
}Comm_Type;

typedef union
{
	unsigned short wFlags;
	struct
	{
		unsigned short bit00_flag:1;	// λ��00
		unsigned short bit01_flag:1;	// λ��01
		unsigned short bit02_flag:1;	// λ��02
		unsigned short bit03_flag:1;	// λ��03
		unsigned short bit04_flag:1;	// λ��04
		unsigned short bit05_flag:1;	// λ��05
		unsigned short bit06_flag:1;	// λ��06
		unsigned short bit07_flag:1;	// λ��07
		unsigned short bit08_flag:1;	// λ��08
		unsigned short bit09_flag:1;	// λ��09
		unsigned short bit10_flag:1;	// λ��10
		unsigned short bit11_flag:1;	// λ��11
		unsigned short bit12_flag:1;	// λ��12
		unsigned short bit13_flag:1;	// λ��13
		unsigned short bit14_flag:1;	// λ��14
		unsigned short bit15_flag:1;	// λ��15
	}bits;
}Flg_Type;

typedef struct
{
	unsigned char sec;	//��
	unsigned char min;	//��
	unsigned char hor;	//ʱ
	unsigned char day;	//��
	unsigned char mon;	//��
	unsigned char yer;	//��
	unsigned char wek;	//��
}Time_Type;

/* GPIO alternate function */
typedef uint8_t gpio_alternate_function_t;

/* Peripheral clock function */
typedef void (*peripheral_clock_function_t)(uint32_t clock, FunctionalState state );

typedef FunctionalState functional_state_t;
typedef uint32_t peripheral_clock_t;

#endif
