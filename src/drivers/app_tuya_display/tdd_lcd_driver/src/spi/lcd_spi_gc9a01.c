#include "tuya_app_config.h"

#ifdef TUYA_MULTI_TYPES_LCD
#include "tal_display_service.h"

static const DISPLAY_INIT_SEQ_T gc9a01_init_seq[] = {
    {.type = TY_INIT_RST,   .reset = {{120}, {120}, {120}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xEF, .len = 0}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xEB, .len = 1,  .v = {0x14}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xFE, .len = 0}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xEF, .len = 0}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xEB, .len = 1,  .v = {0x14}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x84, .len = 1,  .v = {0x40}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x85, .len = 1,  .v = {0xFF}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x86, .len = 1,  .v = {0xFF}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x87, .len = 1,  .v = {0xFF}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x88, .len = 1,  .v = {0x0A}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x89, .len = 1,  .v = {0x21}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x8A, .len = 1,  .v = {0x00}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x8B, .len = 1,  .v = {0x80}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x8C, .len = 1,  .v = {0x01}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x8D, .len = 1,  .v = {0x01}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x8E, .len = 1,  .v = {0xFF}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x8F, .len = 1,  .v = {0xFF}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xB6, .len = 2,  .v = {0x00, 0x00}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x36, .len = 1,  .v = {0x08}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x3A, .len = 1,  .v = {0x55}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x90, .len = 4,  .v = {0x08, 0x08, 0x08, 0x08}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xBD, .len = 1,  .v = {0x06}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xBC, .len = 1,  .v = {0x00}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xFF, .len = 3,  .v = {0x60, 0x01, 0x04}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xC3, .len = 1,  .v = {0x13}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xC4, .len = 1,  .v = {0x13}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xC9, .len = 1,  .v = {0x22}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xBE, .len = 1,  .v = {0x11}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xE1, .len = 2,  .v = {0x10, 0x0E}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xDF, .len = 3,  .v = {0x21, 0x0C, 0x02}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xF0, .len = 6,  .v = {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xF1, .len = 6,  .v = {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xF2, .len = 6,  .v = {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xF3, .len = 6,  .v = {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xED, .len = 2,  .v = {0x1B, 0x0B}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xAE, .len = 1,  .v = {0x77}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xCD, .len = 1,  .v = {0x63}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x70, .len = 9,  .v = {0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xE8, .len = 1,  .v = {0x34}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x62, .len = 12, .v = {0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x63, .len = 12, .v = {0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x64, .len = 7,  .v = {0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x66, .len = 10, .v = {0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x67, .len = 10, .v = {0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x74, .len = 7,  .v = {0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x98, .len = 2,  .v = {0x3E, 0x07}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x21, .len = 0}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x11, .len = 0}},
    {.type = TY_INIT_DELAY, .delay_time = 120},
    {.type = TY_INIT_REG,   .reg = {.r = 0x29, .len = 0}},
    {.type = TY_INIT_CONF_END}
};

const ty_display_device_s lcd_spi_gc9a01_device = {
    .type = DISPLAY_SPI,
    .name = "spi_gc9a01",
    .spi = {
        .width = 240,
        .height = 240,
        .pixel_fmt = TY_PIXEL_FMT_RGB565,
        .cfg = {
            .role = TUYA_SPI_ROLE_MASTER,
            .mode = TUYA_SPI_MODE0,
            .type = TUYA_SPI_AUTO_TYPE,
            .databits = TUYA_SPI_DATA_BIT8,
            .bitorder = TUYA_SPI_ORDER_MSB2LSB,
            .freq_hz = 40000000,
            .spi_dma_flags = 1
        },
        .init_seq = gc9a01_init_seq,
        .display_cfg = NULL
    }
};
#endif
