#ifndef _LIB_DEFS_H_
#define _LIB_DEFS_H_
	
#define _BIT(bitno)									(1ul<<bitno)  
#define _BIT_SET(val,bitno)							(val|=_BIT(bitno))
#define _BIT_CLR(val,bitno)							(val&=~_BIT(bitno))
#define _BIT_XOR(val,bitno)							(val^=_BIT(bitno))
#define _BIT_TST(val,bitno)							(val&_BIT(bitno))

/***********************uctsk_event************************/

// for core task
#define EVENT_CORE_FORMAT                           0x00	// �ļ�ϵͳ��ʽ��
#define EVENT_CORE_FILNUM                           0x01	// ��ȡ�ļ�����
#define EVENT_CORE_FILIFO                           0x02	// ��ȡ�ļ���Ϣ(�ļ���+�ļ���С+�ļ�CRC32У��ֵ)
#define EVENT_CORE_DLSTAT                           0x03	// ���ش�������
#define EVENT_CORE_DLFILE                           0x04	// ���ش����ļ�
#define EVENT_CORE_DLFISH                           0x05	// ���ش������
#define EVENT_CORE_ULSTAT                           0x06	// �ϴ���������
#define EVENT_CORE_ULFILE                           0x07	// �ϴ������ļ�
#define EVENT_CORE_ULFISH                           0x08	// �ϴ��������
 
// for info task
#define EVENT_INFO_SETRTC                           0x20
#define EVENT_INFO_GETRTC                           0x21
#define EVENT_INFO_SETSYS                           0x22
#define EVENT_INFO_GETSYS                           0x23
#define EVENT_INFO_SETIFO                           0x24
#define EVENT_INFO_GETIFO                           0x25
#define EVENT_INFO_SETNET                           0x26
#define EVENT_INFO_GETNET                           0x27
#define EVENT_INFO_SETPID                           0x28
#define EVENT_INFO_GETPID                           0x29
#define EVENT_INFO_BLOCK10                          0x2A
#define EVENT_INFO_BLOCK11                          0x2B
#define EVENT_INFO_BLOCK12                          0x2C
#define EVENT_INFO_BLOCK13                          0x2D
#define EVENT_INFO_BLOCK14                          0x2E
#define EVENT_INFO_BLOCK15                          0x2F
#define EVENT_INFO_BLOCK16                          0x30
#define EVENT_INFO_BLOCK17                          0x31
#define EVENT_INFO_BLOCK18                          0x32

#endif
