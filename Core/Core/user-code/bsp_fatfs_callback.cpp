// #include "prj_header.hpp"

// // BSP FATFS Callback
// uint8_t BSP_SD_ReadBlocks_DMA(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks) {
//     uint8_t  sd_state      = MSD_OK;
//     uint32_t address_start = (uint32_t)pData;
//     uint32_t address_end   = address_start + NumOfBlocks * 512;
//     MYSCB_CleanInvalidateDCache_by_AddrRange((uint32_t *)address_start, (uint32_t *)address_end);
//     if (HAL_SD_ReadBlocks_DMA(&hsd1, (uint8_t *)pData, ReadAddr, NumOfBlocks) != HAL_OK) {
//         sd_state = MSD_ERROR;
//     }

//     return sd_state;
// }

// // BSP FATFS Callback
// uint8_t BSP_SD_WriteBlocks_DMA(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks) {
//     uint8_t  sd_state      = MSD_OK;
//     uint32_t address_start = (uint32_t)pData;
//     uint32_t address_end   = address_start + NumOfBlocks * 512;
//     MYSCB_CleanInvalidateDCache_by_AddrRange((uint32_t *)address_start, (uint32_t *)address_end);
//     if (HAL_SD_WriteBlocks_DMA(&hsd1, (uint8_t *)pData, WriteAddr, NumOfBlocks) != HAL_OK) {
//         sd_state = MSD_ERROR;
//     }

//     return sd_state;
// }