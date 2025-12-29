#include "mb.h"
// #include "Com_debug.h"
// ��������Ĵ��������������ڴ洢ʮ·����Ĵ�����ֵ
uint16_t REG_INPUT_BUF[REG_INPUT_SIZE] = { 0,0,0,0,0,0,0,0,0,0 };

// �������ּĴ��������������ڴ洢ʮ·���ּĴ�����ֵ
uint16_t REG_HOLD_BUF[REG_HOLD_SIZE] = { 0,0,500,0,0,0,0,0,0,0 };//set_nums

// ����ʮ·��Ȧ�Ĵ�С
uint8_t REG_COILS_BUF[REG_COILS_SIZE] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };//[2] ����������� 1���� 0�ر�

// ������ɢ��������������ʼ�������ڴ洢ʮ·��ɢ����״̬
uint8_t REG_DISC_BUF[REG_DISC_SIZE] = { 0,0,0,0,0,0,0,0,0,0 };//ֻ�� [3]=>����ת

/**
 * @brief CMD4������ص�����
 *
 * �ú������ڴ���MODBUSЭ���е�CMD4�������ȡ����Ĵ�����
 * ����ָ����ַ��Χ�ڵ�����Ĵ�����ֵ���Ƶ��������С�
 *
 * @param pucRegBuffer ָ�����ڴ洢�Ĵ���ֵ�Ļ�������ָ�롣
 * @param usAddress Ҫ��ȡ����ʼ�Ĵ�����ַ��
 * @param usNRegs Ҫ��ȡ�ļĴ���������
 *
 * @return ����ִ�н���Ĵ�����롣
 */
eMBErrorCode eMBRegInputCB(UCHAR* pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
    printf("д�Ĵ����յ�������\n");
    // ����Ĵ�����������0��ʼ
    USHORT usRegIndex = usAddress - 1;

    // �Ƿ���⣺�����ʷ�Χ�Ƿ񳬳��Ĵ�����������С
    if ((usRegIndex + usNRegs) > REG_INPUT_SIZE)
    {
        return MB_ENOREG;
    }

    // ѭ����ȡ�Ĵ���ֵ��д�뻺����
    while (usNRegs > 0)
    {
        // ���Ĵ����ĸ�8λд�뻺����
        *pucRegBuffer++ = (unsigned char)(REG_INPUT_BUF[usRegIndex] >> 8);
        // ���Ĵ����ĵ�8λд�뻺����
        *pucRegBuffer++ = (unsigned char)(REG_INPUT_BUF[usRegIndex] & 0xFF);
        usRegIndex++;
        usNRegs--;
    }

    return MB_ENOERR;
}

/**
 * @brief CMD6��3��16������ص�����
 *
 * �ú������ڴ���ModbusЭ���е� Holding Registers ��д����
 * �����������ģʽ������д����ָ���ļĴ���������Ӧ�Ĳ�����
 *
 * @param pucRegBuffer �Ĵ������ݻ����������ڶ�ȡ��д��Ĵ������ݡ�
 * @param usAddress ������ʵ���ʼ�Ĵ�����ַ��
 * @param usNRegs ������ʵļĴ���������
 * @param eMode ����ģʽ�������� MB_REG_WRITE��д�Ĵ������� MB_REG_READ�����Ĵ�������
 *
 * @return ����ִ�н��������ɹ��򷵻� MB_ENOERR�����򷵻���Ӧ�Ĵ�����롣
 */
eMBErrorCode eMBRegHoldingCB(UCHAR* pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode)
{
    // ����Ĵ���������Modbus ��ַ�� 1 ��ʼ������������ 0 ��ʼ�������Ҫ�� 1��
    USHORT usRegIndex = usAddress - 1;

    // �Ƿ���⣺�����ʷ�Χ�Ƿ񳬳��Ĵ�����������С��
    if ((usRegIndex + usNRegs) > REG_HOLD_SIZE)
    {
        return MB_ENOREG;
    }

    // д�Ĵ���
    if (eMode == MB_REG_WRITE)
    {
        // ѭ����ÿ���Ĵ��������ݴӻ�����д�뵽�Ĵ����С�
        while (usNRegs > 0)
        {
            REG_HOLD_BUF[usRegIndex] = (pucRegBuffer[0] << 8) | pucRegBuffer[1];
            pucRegBuffer += 2;
            usRegIndex++;
            usNRegs--;
        }
    }
    // ���Ĵ���
    else
    {
        // ѭ����ÿ���Ĵ��������ݴӼĴ����ж�ȡ����������
        while (usNRegs > 0)
        {
            *pucRegBuffer++ = (unsigned char)(REG_HOLD_BUF[usRegIndex] >> 8);
            *pucRegBuffer++ = (unsigned char)(REG_HOLD_BUF[usRegIndex] & 0xFF);
            usRegIndex++;
            usNRegs--;
        }
    }

    return MB_ENOERR;
}

/**
 * @brief CMD1��5��15������ص�����
 *
 * �ú������ڴ���ModbusЭ���е�CMD1��CMD5��CMD15���
 * ����Ҫ�����ȡ��д��Ĵ����е�λ���ݡ�
 *
 * @param pucRegBuffer ָ��Ĵ�����������ָ�룬���ڶ�ȡ��д�����ݡ�
 * @param usAddress Ҫ�����ļĴ�����ʼ��ַ��
 * @param usNCoils Ҫ������λ����
 * @param eMode ����ģʽ�������Ƕ���д��
 *
 * @return ���ز������������ɹ��򷵻�MB_ENOERR�����򷵻���Ӧ�Ĵ�����롣
 */
eMBErrorCode eMBRegCoilsCB(UCHAR* pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode)
{
    // ����Ĵ�������
    USHORT usRegIndex = usAddress - 1;
    // ����λ�����ı���
    UCHAR ucBits = 0;
    // ���ڴ洢λ״̬�ı���
    UCHAR ucState = 0;
    // ����ѭ�������ı���
    UCHAR ucLoops = 0;

    // �Ƿ���⣺�����ʷ�Χ�Ƿ񳬳��Ĵ�����������С��
    if ((usRegIndex + usNCoils) > REG_COILS_SIZE)
    {
        return MB_ENOREG;
    }

    // ���ݲ���ģʽִ����Ӧ�Ĳ���
    if (eMode == MB_REG_WRITE)
    {
        // ������Ҫѭ���Ĵ���
        ucLoops = (usNCoils - 1) / 8 + 1;
        // д����
        while (ucLoops != 0)
        {
            // ��ȡ��ǰ�Ĵ�����״̬
            ucState = *pucRegBuffer++;
            // λ����
            ucBits = 0;
            // ����ÿ��λ
            while (usNCoils != 0 && ucBits < 8)
            {
                // ��״̬д��Ĵ���������
                REG_COILS_BUF[usRegIndex++] = (ucState >> ucBits) & 0X01;
                // ����ʣ��λ��
                usNCoils--;
                // ����λ����
                ucBits++;
            }
            // ����ѭ������
            ucLoops--;
        }
    }
    else
    {
        // ������Ҫѭ���Ĵ���
        ucLoops = (usNCoils - 1) / 8 + 1;
        // ������
        while (ucLoops != 0)
        {
            // ��ʼ��״̬����
            ucState = 0;
            // λ����
            ucBits = 0;
            // ����ÿ��λ
            while (usNCoils != 0 && ucBits < 8)
            {
                // ���ݼĴ�����������״̬����״̬����
                if (REG_COILS_BUF[usRegIndex])
                {
                    ucState |= (1 << ucBits);
                }
                // ����ʣ��λ��
                usNCoils--;
                // ���¼Ĵ�������
                usRegIndex++;
                // ����λ����
                ucBits++;
            }
            // ��״̬д��Ĵ���������
            *pucRegBuffer++ = ucState;
            // ����ѭ������
            ucLoops--;
        }
    }

    return MB_ENOERR;
}

/**
 * @brief CMD2������ص�����
 *
 * �ú������ڴ���MODBUSЭ���е�CMD2�����Ҫ�����ȡ��ɢ����Ĵ�����ֵ��
 *
 * @param pucRegBuffer ָ���żĴ������ݵĻ�������
 * @param usAddress �Ĵ�������ʼ��ַ��
 * @param usNDiscrete Ҫ��ȡ����ɢ����Ĵ�����������
 *
 * @return ����ִ�н��������ɹ��򷵻�MB_ENOERR�����򷵻���Ӧ�Ĵ�����롣
 */
eMBErrorCode eMBRegDiscreteCB(UCHAR* pucRegBuffer, USHORT usAddress, USHORT usNDiscrete)
{
    // ����Ĵ�����������0��ʼ
    USHORT usRegIndex = usAddress - 1;
    // ���ڴ���λ�����ı���
    UCHAR ucBits = 0;
    // ���ڴ洢��ǰ�Ĵ���״̬�ı���
    UCHAR ucState = 0;
    // ���ڿ���ѭ�������ı���
    UCHAR ucLoops = 0;

    // �Ƿ���⣺�����ʷ�Χ�Ƿ񳬳��Ĵ�����������С
    if ((usRegIndex + usNDiscrete) > REG_DISC_SIZE)
    {
        return MB_ENOREG;
    }

    // ������Ҫѭ���Ĵ�����ÿ��ѭ���������8����ɢ����
    ucLoops = (usNDiscrete - 1) / 8 + 1;
    // ѭ����ȡ��ɢ����Ĵ�����ֵ
    while (ucLoops != 0)
    {
        ucState = 0;
        ucBits = 0;
        // ��ȡÿ���Ĵ�����ֵ��������״̬���µ�ucState������
        while (usNDiscrete != 0 && ucBits < 8)
        {
            if (REG_DISC_BUF[usRegIndex])
            {
                ucState |= (1 << ucBits);
            }
            usNDiscrete--;
            usRegIndex++;
            ucBits++;
        }
        // ����ȡ����״ֵ̬���뻺������
        *pucRegBuffer++ = ucState;
        ucLoops--;
    }

    // ģ����ɢ�����뱻�ı䣬����򵥵ؽ�ÿ���Ĵ�����״̬ȡ��
    for (usRegIndex = 0; usRegIndex < REG_DISC_SIZE; usRegIndex++)
    {
        REG_DISC_BUF[usRegIndex] = !REG_DISC_BUF[usRegIndex];
    }

    return MB_ENOERR;
}

/**
 * @brief �� Modbus �������ӳ��Ϊ Modbus �쳣��
 *
 * @param error Modbus ������루eMBErrorCode��
 * @return ��Ӧ�� Modbus �쳣��
 */
uint8_t mapErrorToException(eMBErrorCode error)
{
    switch (error)
    {
    case MB_ENOREG:     // �Ƿ��Ĵ�����ַ
        return 0x02;    // �Ƿ����ݵ�ַ
    case MB_EINVAL:     // �Ƿ�����
        return 0x03;    // �Ƿ�����ֵ
    case MB_ENORES:     // ��Դ����
        return 0x04;    // ��վ�豸����
    case MB_ETIMEDOUT:  // ��ʱ
        return 0x06;    // ��վ�豸æ
    case MB_EPORTERR:   // �˿ڴ���
        return 0x04;    // ��վ�豸����
    case MB_ENOERR:     // �޴���
        return 0x00;    // ���쳣
    default:            // δ֪����
        return 0x04;    // ��վ�豸����
    }
}

