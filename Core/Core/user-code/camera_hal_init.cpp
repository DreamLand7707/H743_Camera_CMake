
#include "camera_declare.hpp"
//
void dcmi_data_structure_init() {
    RGB_hdcmi.data.instance.Instance                = DCMI;
    RGB_hdcmi.data.instance.Init.SynchroMode        = DCMI_SYNCHRO_HARDWARE;
    RGB_hdcmi.data.instance.Init.PCKPolarity        = DCMI_PCKPOLARITY_RISING;
    RGB_hdcmi.data.instance.Init.VSPolarity         = DCMI_VSPOLARITY_HIGH;
    RGB_hdcmi.data.instance.Init.HSPolarity         = DCMI_HSPOLARITY_HIGH;
    RGB_hdcmi.data.instance.Init.CaptureRate        = DCMI_CR_ALL_FRAME;
    RGB_hdcmi.data.instance.Init.ExtendedDataMode   = DCMI_EXTEND_DATA_8B;
    RGB_hdcmi.data.instance.Init.JPEGMode           = DCMI_JPEG_DISABLE;
    RGB_hdcmi.data.instance.Init.ByteSelectMode     = DCMI_BSM_ALL;
    RGB_hdcmi.data.instance.Init.ByteSelectStart    = DCMI_OEBS_ODD;
    RGB_hdcmi.data.instance.Init.LineSelectMode     = DCMI_LSM_ALL;
    RGB_hdcmi.data.instance.Init.LineSelectStart    = DCMI_OELS_ODD;
    RGB_hdcmi.data.instance.MspInitCallback         = dcmi_rgb_mspinit;
    RGB_hdcmi.data.instance.MspDeInitCallback       = dcmi_msp_deinit;
    RGB_hdcmi.data.eg                               = xEventGroupCreate();

    YCbCr_hdcmi.data.instance.Instance              = DCMI;
    YCbCr_hdcmi.data.instance.Init.SynchroMode      = DCMI_SYNCHRO_HARDWARE;
    YCbCr_hdcmi.data.instance.Init.PCKPolarity      = DCMI_PCKPOLARITY_RISING;
    YCbCr_hdcmi.data.instance.Init.VSPolarity       = DCMI_VSPOLARITY_HIGH;
    YCbCr_hdcmi.data.instance.Init.HSPolarity       = DCMI_HSPOLARITY_HIGH;
    YCbCr_hdcmi.data.instance.Init.CaptureRate      = DCMI_CR_ALL_FRAME;
    YCbCr_hdcmi.data.instance.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
    YCbCr_hdcmi.data.instance.Init.JPEGMode         = DCMI_JPEG_DISABLE;
    YCbCr_hdcmi.data.instance.Init.ByteSelectMode   = DCMI_BSM_ALL;
    YCbCr_hdcmi.data.instance.Init.ByteSelectStart  = DCMI_OEBS_ODD;
    YCbCr_hdcmi.data.instance.Init.LineSelectMode   = DCMI_LSM_ALL;
    YCbCr_hdcmi.data.instance.Init.LineSelectStart  = DCMI_OELS_ODD;
    YCbCr_hdcmi.data.instance.MspInitCallback       = dcmi_ycbcr_mspinit;
    YCbCr_hdcmi.data.instance.MspDeInitCallback     = dcmi_msp_deinit;
    YCbCr_hdcmi.data.eg                             = xEventGroupCreate();

    JPEG_hdcmi.data.instance.Instance               = DCMI;
    JPEG_hdcmi.data.instance.Init.SynchroMode       = DCMI_SYNCHRO_HARDWARE;
    JPEG_hdcmi.data.instance.Init.PCKPolarity       = DCMI_PCKPOLARITY_RISING;
    JPEG_hdcmi.data.instance.Init.VSPolarity        = DCMI_VSPOLARITY_HIGH;
    JPEG_hdcmi.data.instance.Init.HSPolarity        = DCMI_HSPOLARITY_HIGH;
    JPEG_hdcmi.data.instance.Init.CaptureRate       = DCMI_CR_ALL_FRAME;
    JPEG_hdcmi.data.instance.Init.ExtendedDataMode  = DCMI_EXTEND_DATA_8B;
    JPEG_hdcmi.data.instance.Init.JPEGMode          = DCMI_JPEG_ENABLE;
    JPEG_hdcmi.data.instance.Init.ByteSelectMode    = DCMI_BSM_ALL;
    JPEG_hdcmi.data.instance.Init.ByteSelectStart   = DCMI_OEBS_ODD;
    JPEG_hdcmi.data.instance.Init.LineSelectMode    = DCMI_LSM_ALL;
    JPEG_hdcmi.data.instance.Init.LineSelectStart   = DCMI_OELS_ODD;
    JPEG_hdcmi.data.instance.MspInitCallback        = dcmi_jpeg_mspinit;
    JPEG_hdcmi.data.instance.MspDeInitCallback      = dcmi_msp_deinit;
    JPEG_hdcmi.data.eg                              = xEventGroupCreate();
    // Delay
    static auto delay_handle = [](uint32_t delay)
    {
        uint32_t i;
        while (delay--) {
            i = 200;
            while (i--)
                ;
        }
    };
    my_sccb.address_withwr          = OV5640_ADDR;
    my_sccb.parameters.delay_handle = delay_handle;
    my_sccb.parameters.frequency    = 200000;
    my_sccb.port_setting.SCL_port   = DCMI_SCL_GPIO_Port;
    my_sccb.port_setting.SCL_pin    = DCMI_SCL_Pin;
    my_sccb.port_setting.SDA_port   = DCMI_SDA_GPIO_Port;
    my_sccb.port_setting.SDA_pin    = DCMI_SDA_Pin;

    static auto get_tick            = []()
    {
        return (int32_t)(HAL_GetTick());
    };
    ov5640_io.Address  = OV5640_ADDR;
    ov5640_io.GetTick  = get_tick;
    ov5640_io.Init     = ov5640_init;
    ov5640_io.DeInit   = ov5640_deinit;
    ov5640_io.ReadReg  = ov5640_read_series_reg;
    ov5640_io.WriteReg = ov5640_write_series_reg;

    current_resolution = camera_resolution::reso_sreen;
    current_format     = camera_format::format_RGB;
}

static void dcmi_clk_pin_init() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_DCMI_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin       = GPIO_PIN_6;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin       = GPIO_PIN_8;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

    GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_11;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin       = GPIO_PIN_3;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin       = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void dcmi_rgb_mspinit(DCMI_HandleTypeDef *dcmiHandle) {
    dcmi_clk_pin_init();

    Camera_DCMI_Data   *p                       = container_of(dcmiHandle, Camera_DCMI_Data, instance);
    DMA_HandleTypeDef  &camera_first_stage_dma  = p->first_stage_dma;
    MDMA_HandleTypeDef &camera_second_stage_dma = p->second_stage_dma;
    //
    camera_first_stage_dma.Instance                 = DMA1_Stream0;
    camera_first_stage_dma.Init.Request             = DMA_REQUEST_DCMI;
    camera_first_stage_dma.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    camera_first_stage_dma.Init.PeriphInc           = DMA_PINC_DISABLE;
    camera_first_stage_dma.Init.MemInc              = DMA_MINC_ENABLE;
    camera_first_stage_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    camera_first_stage_dma.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    camera_first_stage_dma.Init.Mode                = DMA_CIRCULAR;
    camera_first_stage_dma.Init.Priority            = DMA_PRIORITY_HIGH;
    camera_first_stage_dma.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    camera_first_stage_dma.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    camera_first_stage_dma.Init.MemBurst            = DMA_MBURST_SINGLE;
    camera_first_stage_dma.Init.PeriphBurst         = DMA_PBURST_SINGLE;
    if (HAL_DMA_Init(&camera_first_stage_dma) != HAL_OK) {
        Error_Handler();
    }

    __HAL_LINKDMA(dcmiHandle, DMA_Handle, camera_first_stage_dma);

    HAL_NVIC_SetPriority(DCMI_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DCMI_IRQn);

    // MDMA
    camera_second_stage_dma.Instance                      = MDMA_Channel2;
    camera_second_stage_dma.Init.Request                  = MDMA_REQUEST_DMA1_Stream0_TC;
    camera_second_stage_dma.Init.TransferTriggerMode      = MDMA_REPEAT_BLOCK_TRANSFER;
    camera_second_stage_dma.Init.Priority                 = MDMA_PRIORITY_VERY_HIGH;
    camera_second_stage_dma.Init.Endianness               = MDMA_LITTLE_ENDIANNESS_PRESERVE;
    camera_second_stage_dma.Init.SourceInc                = MDMA_SRC_INC_WORD;
    camera_second_stage_dma.Init.DestinationInc           = MDMA_DEST_INC_WORD;
    camera_second_stage_dma.Init.SourceDataSize           = MDMA_SRC_DATASIZE_WORD;
    camera_second_stage_dma.Init.DestDataSize             = MDMA_DEST_DATASIZE_WORD;
    camera_second_stage_dma.Init.DataAlignment            = MDMA_DATAALIGN_PACKENABLE;
    camera_second_stage_dma.Init.BufferTransferLength     = 32;
    camera_second_stage_dma.Init.SourceBurst              = MDMA_SOURCE_BURST_8BEATS;
    camera_second_stage_dma.Init.DestBurst                = MDMA_DEST_BURST_8BEATS;
    camera_second_stage_dma.Init.SourceBlockAddressOffset = 0;
    camera_second_stage_dma.Init.DestBlockAddressOffset   = 0;
}

void dcmi_ycbcr_mspinit(DCMI_HandleTypeDef *dcmiHandle) {
    dcmi_clk_pin_init();

    Camera_DCMI_Data *p                       = container_of(dcmiHandle, Camera_DCMI_Data, instance);
    DMA_HandleTypeDef      &camera_first_stage_dma  = p->first_stage_dma;
    MDMA_HandleTypeDef     &camera_second_stage_dma = p->second_stage_dma;
    //
    camera_first_stage_dma.Instance                 = DMA1_Stream0;
    camera_first_stage_dma.Init.Request             = DMA_REQUEST_DCMI;
    camera_first_stage_dma.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    camera_first_stage_dma.Init.PeriphInc           = DMA_PINC_DISABLE;
    camera_first_stage_dma.Init.MemInc              = DMA_MINC_ENABLE;
    camera_first_stage_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    camera_first_stage_dma.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    camera_first_stage_dma.Init.Mode                = DMA_CIRCULAR;
    camera_first_stage_dma.Init.Priority            = DMA_PRIORITY_HIGH;
    camera_first_stage_dma.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    camera_first_stage_dma.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    camera_first_stage_dma.Init.MemBurst            = DMA_MBURST_INC4;
    camera_first_stage_dma.Init.PeriphBurst         = DMA_PBURST_SINGLE;
    if (HAL_DMA_Init(&camera_first_stage_dma) != HAL_OK) {
        Error_Handler();
    }

    __HAL_LINKDMA(dcmiHandle, DMA_Handle, camera_first_stage_dma);

    HAL_NVIC_SetPriority(DCMI_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DCMI_IRQn);

    // MDMA
    camera_second_stage_dma.Instance                      = MDMA_Channel2;
    camera_second_stage_dma.Init.Request                  = MDMA_REQUEST_DMA1_Stream0_TC;
    camera_second_stage_dma.Init.TransferTriggerMode      = MDMA_REPEAT_BLOCK_TRANSFER;
    camera_second_stage_dma.Init.Priority                 = MDMA_PRIORITY_VERY_HIGH;
    camera_second_stage_dma.Init.Endianness               = MDMA_LITTLE_ENDIANNESS_PRESERVE;
    camera_second_stage_dma.Init.SourceInc                = MDMA_SRC_INC_WORD;
    camera_second_stage_dma.Init.DestinationInc           = MDMA_DEST_INC_WORD;
    camera_second_stage_dma.Init.SourceDataSize           = MDMA_SRC_DATASIZE_WORD;
    camera_second_stage_dma.Init.DestDataSize             = MDMA_DEST_DATASIZE_WORD;
    camera_second_stage_dma.Init.DataAlignment            = MDMA_DATAALIGN_PACKENABLE;
    camera_second_stage_dma.Init.BufferTransferLength     = 32;
    camera_second_stage_dma.Init.SourceBurst              = MDMA_SOURCE_BURST_8BEATS;
    camera_second_stage_dma.Init.DestBurst                = MDMA_DEST_BURST_8BEATS;
    camera_second_stage_dma.Init.SourceBlockAddressOffset = 0;
    camera_second_stage_dma.Init.DestBlockAddressOffset   = 0;
}

void dcmi_jpeg_mspinit(DCMI_HandleTypeDef *dcmiHandle) {
    dcmi_clk_pin_init();

    Camera_DCMI_Data *p                       = container_of(dcmiHandle, Camera_DCMI_Data, instance);
    DMA_HandleTypeDef      &camera_first_stage_dma  = p->first_stage_dma;
    MDMA_HandleTypeDef     &camera_second_stage_dma = p->second_stage_dma;
    //
    camera_first_stage_dma.Instance                 = DMA1_Stream0;
    camera_first_stage_dma.Init.Request             = DMA_REQUEST_DCMI;
    camera_first_stage_dma.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    camera_first_stage_dma.Init.PeriphInc           = DMA_PINC_DISABLE;
    camera_first_stage_dma.Init.MemInc              = DMA_MINC_ENABLE;
    camera_first_stage_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    camera_first_stage_dma.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    camera_first_stage_dma.Init.Mode                = DMA_CIRCULAR;
    camera_first_stage_dma.Init.Priority            = DMA_PRIORITY_HIGH;
    camera_first_stage_dma.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    camera_first_stage_dma.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    camera_first_stage_dma.Init.MemBurst            = DMA_MBURST_INC4;
    camera_first_stage_dma.Init.PeriphBurst         = DMA_PBURST_SINGLE;
    if (HAL_DMA_Init(&camera_first_stage_dma) != HAL_OK) {
        Error_Handler();
    }

    __HAL_LINKDMA(dcmiHandle, DMA_Handle, camera_first_stage_dma);

    HAL_NVIC_SetPriority(DCMI_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DCMI_IRQn);

    // MDMA
    camera_second_stage_dma.Instance                      = MDMA_Channel2;
    camera_second_stage_dma.Init.Request                  = MDMA_REQUEST_DMA1_Stream0_TC;
    camera_second_stage_dma.Init.TransferTriggerMode      = MDMA_REPEAT_BLOCK_TRANSFER;
    camera_second_stage_dma.Init.Priority                 = MDMA_PRIORITY_VERY_HIGH;
    camera_second_stage_dma.Init.Endianness               = MDMA_LITTLE_ENDIANNESS_PRESERVE;
    camera_second_stage_dma.Init.SourceInc                = MDMA_SRC_INC_WORD;
    camera_second_stage_dma.Init.DestinationInc           = MDMA_DEST_INC_WORD;
    camera_second_stage_dma.Init.SourceDataSize           = MDMA_SRC_DATASIZE_WORD;
    camera_second_stage_dma.Init.DestDataSize             = MDMA_DEST_DATASIZE_WORD;
    camera_second_stage_dma.Init.DataAlignment            = MDMA_DATAALIGN_PACKENABLE;
    camera_second_stage_dma.Init.BufferTransferLength     = 32;
    camera_second_stage_dma.Init.SourceBurst              = MDMA_SOURCE_BURST_8BEATS;
    camera_second_stage_dma.Init.DestBurst                = MDMA_DEST_BURST_8BEATS;
    camera_second_stage_dma.Init.SourceBlockAddressOffset = 0;
    camera_second_stage_dma.Init.DestBlockAddressOffset   = 0;
}



void dcmi_msp_deinit(DCMI_HandleTypeDef *dcmiHandle) {
    __HAL_RCC_DCMI_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_6);
    HAL_GPIO_DeInit(GPIOH, GPIO_PIN_8);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_11);
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_3);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9);

    HAL_DMA_DeInit(dcmiHandle->DMA_Handle);
    HAL_NVIC_DisableIRQ(DCMI_IRQn);
}



int32_t ov5640_init() {
    uint8_t  reg1, reg2;
    uint16_t reg;

#ifdef ALINTEK_BOARD
    if (!PCF8574_init) {
        PCF8574_Init();
        PCF8574_init = true;
    }
#endif

    OV5640_RST(0);
    timer_delay_ms(20);
    OV5640_PWDN_Set(0);
    timer_delay_ms(5);
    OV5640_RST(1);
    timer_delay_ms(20);

    timer_delay_ms(5);
    if (OV5640_RD_Reg(&my_sccb, OV5640_CHIP_ID_HIGH_BYTE, &reg1))
        return OV5640_ERROR;
    if (OV5640_RD_Reg(&my_sccb, OV5640_CHIP_ID_LOW_BYTE, &reg2))
        return OV5640_ERROR;
    reg = reg1;
    reg <<= 8u;
    reg |= reg2;
    if (reg != OV5640_ID) {
        return OV5640_ERROR;
    }

    return OV5640_OK;
}

int32_t ov5640_deinit() { return OV5640_OK; }



int32_t ov5640_read_series_reg(uint16_t address, uint16_t reg, uint8_t *pdata, uint16_t length) {
    (void)address;
    for (uint16_t idx = 0; idx < length; idx++) {
        if (OV5640_RD_Reg(&my_sccb, reg + idx, pdata + idx))
            return OV5640_ERROR;
    }
    return OV5640_OK;
}

int32_t ov5640_write_series_reg(uint16_t address, uint16_t reg, uint8_t *data, uint16_t length) {
    (void)address;
    for (uint16_t idx = 0; idx < length; idx++) {
        if (OV5640_WR_Reg(&my_sccb, reg + idx, data[idx]))
            return OV5640_ERROR;
    }
    return OV5640_OK;
}

void OV5640_PWDN_Set(uint8_t sta) {
#ifdef ALINTEK_BOARD
    PCF8574_WriteBit(DCMI_PWDN_IO, sta);
#else
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, sta ? GPIO_PIN_SET : GPIO_PIN_RESET);
#endif
}

void dcmi_io_deinit_ov5640() {
#ifdef ALINTEK_BOARD
    if (!PCF8574_init) {
        PCF8574_Init();
        PCF8574_init = true;
    }
#endif
    OV5640_PWDN_Set(1);
    OV5640_RST(0);
}
