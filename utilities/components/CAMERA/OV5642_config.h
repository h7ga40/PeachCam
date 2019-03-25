
#ifndef OV5642_CONFIG_H
#define OV5642_CONFIG_H

#include "mbed.h"
#include "camera_config.h"

#define SIZE_640x480   1
#define SIZE_800x600   2
#define SIZE_1280x720  3
#define SIZE_1280x960  4

#define OV5642_SIZE SIZE_640x480

class OV5642_config : public camera_config {

public:

    /** Initialise
     *
     * @return true = success, false = failure
     */
    virtual bool Initialise() {
        /* OV5642 camera input config */
        const char OV5642_InitRegTable[][3] = {
            {0x31, 0x03, 0x93}, {0x30, 0x08, 0x82}, {0x30, 0x17, 0x7f}, {0x30, 0x18, 0xfc},
            {0x38, 0x10, 0xc2}, {0x36, 0x15, 0xf0}, {0x30, 0x00, 0x00}, {0x30, 0x01, 0x00},
            {0x30, 0x02, 0x00}, {0x30, 0x03, 0x00}, {0x30, 0x00, 0xf8}, {0x30, 0x01, 0x48},
            {0x30, 0x02, 0x5c}, {0x30, 0x03, 0x02}, {0x30, 0x04, 0x07}, {0x30, 0x05, 0xb7},
            {0x30, 0x06, 0x43}, {0x30, 0x07, 0x37}, {0x30, 0x10, 0x10}, {0x46, 0x0c, 0x22},
        #if ( OV5642_SIZE == SIZE_640x480 )
            /* PLL   0x10 : 30fps */
            {0x30, 0x11, 0x10}, {0x38, 0x15, 0x04},
        #elif ( OV5642_SIZE == SIZE_1280x720 || OV5642_SIZE == SIZE_1280x960 || OV5642_SIZE == SIZE_800x600 )
            /* PLL   0x08 : 15fps */
            {0x30, 0x11, 0x08}, {0x38, 0x15, 0x02},
        #endif
            {0x37, 0x0d, 0x06}, {0x37, 0x0c, 0xa0}, {0x36, 0x02, 0xfc}, {0x36, 0x12, 0xff},
            {0x36, 0x34, 0xc0}, {0x36, 0x13, 0x00}, {0x36, 0x05, 0x7c}, {0x36, 0x21, 0x09},
            {0x36, 0x22, 0x00}, {0x36, 0x04, 0x40}, {0x36, 0x03, 0xa7}, {0x36, 0x03, 0x27},
            {0x40, 0x00, 0x21}, {0x40, 0x1d, 0x02}, {0x36, 0x00, 0x54}, {0x36, 0x05, 0x04},
            {0x36, 0x06, 0x3f}, {0x3c, 0x01, 0x00}, {0x50, 0x00, 0x4f}, {0x50, 0x20, 0x04},
            {0x51, 0x81, 0x79}, {0x51, 0x82, 0x00}, {0x51, 0x85, 0x22}, {0x51, 0x97, 0x01},
            {0x50, 0x01, 0xff}, {0x55, 0x00, 0x0a}, {0x55, 0x04, 0x00}, {0x55, 0x05, 0x7f},
            {0x50, 0x80, 0x08}, {0x30, 0x0e, 0x18}, {0x46, 0x10, 0x00}, {0x47, 0x1d, 0x05},
            {0x47, 0x08, 0x06}, {0x37, 0x10, 0x10}, {0x36, 0x32, 0x41}, {0x37, 0x02, 0x40},
            {0x36, 0x20, 0x37}, {0x36, 0x31, 0x01},
        /* Output size(scaling) */
        #if ( OV5642_SIZE == SIZE_640x480 )
            /* DVPHO 640, DVPVO 480 */
            {0x38, 0x08, 0x02}, {0x38, 0x09, 0x80}, {0x38, 0x0a, 0x01}, {0x38, 0x0b, 0xe0},
        #elif ( OV5642_SIZE == SIZE_800x600 )
            /* DVPHO 800, DVPVO 600 */
            {0x38, 0x08, 0x03}, {0x38, 0x09, 0x20}, {0x38, 0x0a, 0x02}, {0x38, 0x0b, 0x58},
        #elif ( OV5642_SIZE == SIZE_1280x720 )
            /* DVPHO 1280, DVPVO 720 */
            {0x38, 0x08, 0x05}, {0x38, 0x09, 0x00}, {0x38, 0x0a, 0x02}, {0x38, 0x0b, 0xd0},
        #elif ( OV5642_SIZE == SIZE_1280x960 )
            /* DVPHO 1280, DVPVO 960 */
            {0x38, 0x08, 0x05}, {0x38, 0x09, 0x00}, {0x38, 0x0a, 0x03}, {0x38, 0x0b, 0xc0},
        #endif
            /* H total */
            {0x38, 0x0c, 0x0c}, {0x38, 0x0d, 0x80}, {0x38, 0x0e, 0x07}, {0x38, 0x0f, 0xd0},
            {0x50, 0x1f, 0x00}, {0x50, 0x00, 0x4f}, {0x43, 0x00, 0x30}, {0x35, 0x03, 0x07},
            {0x35, 0x01, 0x73}, {0x35, 0x02, 0x80}, {0x35, 0x0b, 0x00}, {0x35, 0x03, 0x07},
            {0x38, 0x24, 0x11}, {0x35, 0x01, 0x1e}, {0x35, 0x02, 0x80}, {0x35, 0x0b, 0x7f},
            {0x38, 0x0c, 0x0c}, {0x38, 0x0d, 0x80}, {0x38, 0x0e, 0x03}, {0x38, 0x0f, 0xe8},
            {0x3a, 0x0d, 0x04}, {0x3a, 0x0e, 0x03}, {0x38, 0x18, 0xa1}, {0x37, 0x05, 0xdb},
            {0x37, 0x0a, 0x81}, {0x38, 0x01, 0x80}, {0x36, 0x21, 0xe7}, {0x38, 0x01, 0x50},
            {0x38, 0x03, 0x08}, {0x38, 0x27, 0x08}, {0x38, 0x10, 0xc0}, {0x47, 0x11, 0x20},
        #if ( OV5642_SIZE == SIZE_1280x720 )
            {0x38, 0x00, 0x00}, {0x38, 0x01, 0x50}, {0x38, 0x02, 0x00}, {0x38, 0x03, 0xf8},
            {0x38, 0x04, 0x05}, {0x38, 0x05, 0x00}, {0x56, 0x82, 0x05}, {0x56, 0x83, 0x00},
            {0x38, 0x06, 0x02}, {0x38, 0x07, 0xd0}, {0x56, 0x86, 0x02}, {0x56, 0x87, 0xd0},
        #elif ( OV5642_SIZE == SIZE_1280x960 || OV5642_SIZE == SIZE_800x600 || OV5642_SIZE == SIZE_640x480 )
            {0x38, 0x00, 0x00}, {0x38, 0x01, 0x50}, {0x38, 0x02, 0x00}, {0x38, 0x03, 0x08},
            {0x38, 0x04, 0x05}, {0x38, 0x05, 0x00}, {0x56, 0x82, 0x05}, {0x56, 0x83, 0x00},
            {0x38, 0x06, 0x03}, {0x38, 0x07, 0xc0}, {0x56, 0x86, 0x03}, {0x56, 0x87, 0xc0},
        #endif
            {0x3a, 0x00, 0x78}, {0x3a, 0x1a, 0x04}, {0x3a, 0x13, 0x30}, {0x3a, 0x18, 0x00},
            {0x3a, 0x19, 0x7c}, {0x3a, 0x08, 0x12}, {0x3a, 0x09, 0xc0}, {0x3a, 0x0a, 0x0f},
            {0x3a, 0x0b, 0xa0}, {0x30, 0x04, 0xff}, {0x35, 0x0c, 0x07}, {0x35, 0x0d, 0xd0},
            {0x35, 0x00, 0x00}, {0x35, 0x01, 0x00}, {0x35, 0x02, 0x00}, {0x35, 0x0a, 0x00},
            {0x35, 0x0b, 0x00}, {0x35, 0x03, 0x00}, {0x52, 0x8a, 0x02}, {0x52, 0x8b, 0x04},
            {0x52, 0x8c, 0x08}, {0x52, 0x8d, 0x08}, {0x52, 0x8e, 0x08}, {0x52, 0x8f, 0x10},
            {0x52, 0x90, 0x10}, {0x52, 0x92, 0x00}, {0x52, 0x93, 0x02}, {0x52, 0x94, 0x00},
            {0x52, 0x95, 0x02}, {0x52, 0x96, 0x00}, {0x52, 0x97, 0x02}, {0x52, 0x98, 0x00},
            {0x52, 0x99, 0x02}, {0x52, 0x9a, 0x00}, {0x52, 0x9b, 0x02}, {0x52, 0x9c, 0x00},
            {0x52, 0x9d, 0x02}, {0x52, 0x9e, 0x00}, {0x52, 0x9f, 0x02}, {0x3a, 0x0f, 0x3c},
            {0x3a, 0x10, 0x30}, {0x3a, 0x1b, 0x3c}, {0x3a, 0x1e, 0x30}, {0x3a, 0x11, 0x70},
            {0x3a, 0x1f, 0x10}, {0x30, 0x30, 0x0b}, {0x3a, 0x02, 0x00}, {0x3a, 0x03, 0x7d},
            {0x3a, 0x04, 0x00}, {0x3a, 0x14, 0x00}, {0x3a, 0x15, 0x7d}, {0x3a, 0x16, 0x00},
            {0x3a, 0x08, 0x09}, {0x3a, 0x09, 0x60}, {0x3a, 0x0a, 0x07}, {0x3a, 0x0b, 0xd0},
            {0x3a, 0x0d, 0x08}, {0x3a, 0x0e, 0x06}, {0x51, 0x93, 0x70}, {0x36, 0x20, 0x57},
            {0x37, 0x03, 0x98}, {0x37, 0x04, 0x1c}, {0x58, 0x9b, 0x04}, {0x58, 0x9a, 0xc5},
            {0x52, 0x8a, 0x00}, {0x52, 0x8b, 0x02}, {0x52, 0x8c, 0x08}, {0x52, 0x8d, 0x10},
            {0x52, 0x8e, 0x20}, {0x52, 0x8f, 0x28}, {0x52, 0x90, 0x30}, {0x52, 0x92, 0x00},
            {0x52, 0x93, 0x00}, {0x52, 0x94, 0x00}, {0x52, 0x95, 0x02}, {0x52, 0x96, 0x00},
            {0x52, 0x97, 0x08}, {0x52, 0x98, 0x00}, {0x52, 0x99, 0x10}, {0x52, 0x9a, 0x00},
            {0x52, 0x9b, 0x20}, {0x52, 0x9c, 0x00}, {0x52, 0x9d, 0x28}, {0x52, 0x9e, 0x00},
            {0x52, 0x9f, 0x30}, {0x52, 0x82, 0x00}, {0x53, 0x00, 0x00}, {0x53, 0x01, 0x20},
            {0x53, 0x02, 0x00}, {0x53, 0x03, 0x7c}, {0x53, 0x0c, 0x00}, {0x53, 0x0d, 0x0c},
            {0x53, 0x0e, 0x20}, {0x53, 0x0f, 0x80}, {0x53, 0x10, 0x20}, {0x53, 0x11, 0x80},
            {0x53, 0x08, 0x20}, {0x53, 0x09, 0x40}, {0x53, 0x04, 0x00}, {0x53, 0x05, 0x30},
            {0x53, 0x06, 0x00}, {0x53, 0x07, 0x80}, {0x53, 0x14, 0x08}, {0x53, 0x15, 0x20},
            {0x53, 0x19, 0x30}, {0x53, 0x16, 0x10}, {0x53, 0x17, 0x08}, {0x53, 0x18, 0x02},
            {0x53, 0x80, 0x01}, {0x53, 0x81, 0x00}, {0x53, 0x82, 0x00}, {0x53, 0x83, 0x4e},
            {0x53, 0x84, 0x00}, {0x53, 0x85, 0x0f}, {0x53, 0x86, 0x00}, {0x53, 0x87, 0x00},
            {0x53, 0x88, 0x01}, {0x53, 0x89, 0x15}, {0x53, 0x8a, 0x00}, {0x53, 0x8b, 0x31},
            {0x53, 0x8c, 0x00}, {0x53, 0x8d, 0x00}, {0x53, 0x8e, 0x00}, {0x53, 0x8f, 0x0f},
            {0x53, 0x90, 0x00}, {0x53, 0x91, 0xab}, {0x53, 0x92, 0x00}, {0x53, 0x93, 0xa2},
            {0x53, 0x94, 0x08}, {0x54, 0x80, 0x14}, {0x54, 0x81, 0x21}, {0x54, 0x82, 0x36},
            {0x54, 0x83, 0x57}, {0x54, 0x84, 0x65}, {0x54, 0x85, 0x71}, {0x54, 0x86, 0x7d},
            {0x54, 0x87, 0x87}, {0x54, 0x88, 0x91}, {0x54, 0x89, 0x9a}, {0x54, 0x8a, 0xaa},
            {0x54, 0x8b, 0xb8}, {0x54, 0x8c, 0xcd}, {0x54, 0x8d, 0xdd}, {0x54, 0x8e, 0xea},
            {0x54, 0x8f, 0x10}, {0x54, 0x90, 0x05}, {0x54, 0x91, 0x00}, {0x54, 0x92, 0x04},
            {0x54, 0x93, 0x20}, {0x54, 0x94, 0x03}, {0x54, 0x95, 0x60}, {0x54, 0x96, 0x02},
            {0x54, 0x97, 0xb8}, {0x54, 0x98, 0x02}, {0x54, 0x99, 0x86}, {0x54, 0x9a, 0x02},
            {0x54, 0x9b, 0x5b}, {0x54, 0x9c, 0x02}, {0x54, 0x9d, 0x3b}, {0x54, 0x9e, 0x02},
            {0x54, 0x9f, 0x1c}, {0x54, 0xa0, 0x02}, {0x54, 0xa1, 0x04}, {0x54, 0xa2, 0x01},
            {0x54, 0xa3, 0xed}, {0x54, 0xa4, 0x01}, {0x54, 0xa5, 0xc5}, {0x54, 0xa6, 0x01},
            {0x54, 0xa7, 0xa5}, {0x54, 0xa8, 0x01}, {0x54, 0xa9, 0x6c}, {0x54, 0xaa, 0x01},
            {0x54, 0xab, 0x41}, {0x54, 0xac, 0x01}, {0x54, 0xad, 0x20}, {0x54, 0xae, 0x00},
            {0x54, 0xaf, 0x16}, {0x34, 0x06, 0x00}, {0x51, 0x92, 0x04}, {0x51, 0x91, 0xf8},
            {0x51, 0x93, 0x70}, {0x51, 0x94, 0xf0}, {0x51, 0x95, 0xf0}, {0x51, 0x8d, 0x3d},
            {0x51, 0x8f, 0x54}, {0x51, 0x8e, 0x3d}, {0x51, 0x90, 0x54}, {0x51, 0x8b, 0xc0},
            {0x51, 0x8c, 0xbd}, {0x51, 0x87, 0x18}, {0x51, 0x88, 0x18}, {0x51, 0x89, 0x6e},
            {0x51, 0x8a, 0x68}, {0x51, 0x86, 0x1c}, {0x51, 0x81, 0x50}, {0x51, 0x84, 0x25},
            {0x51, 0x82, 0x11}, {0x51, 0x83, 0x14}, {0x51, 0x84, 0x25}, {0x51, 0x85, 0x24},
            {0x50, 0x25, 0x82}, {0x3a, 0x0f, 0x7e}, {0x3a, 0x10, 0x72}, {0x3a, 0x1b, 0x80},
            {0x3a, 0x1e, 0x70}, {0x3a, 0x11, 0xd0}, {0x3a, 0x1f, 0x40}, {0x55, 0x83, 0x40},
            {0x55, 0x84, 0x40}, {0x55, 0x80, 0x02}, {0x36, 0x33, 0x07}, {0x37, 0x02, 0x10},
            {0x37, 0x03, 0xb2}, {0x37, 0x04, 0x18}, {0x37, 0x0b, 0x40}, {0x37, 0x0d, 0x02},
            {0x36, 0x20, 0x52}, {0x50, 0x3d, 0x00}, {0x50, 0x3e, 0x00}
        };
        const char sw_reset_cmd[3] = {0x30, 0x00, 0xf8};
        int ret;
        I2C mI2c_(I2C_SDA, I2C_SCL);
        mI2c_.frequency(150000);

        if (mI2c_.write(0x78, sw_reset_cmd, 3) != 0) {
            return false;
        }
        ThisThread::sleep_for(1);

        for (uint32_t i = 0; i < (sizeof(OV5642_InitRegTable) / 3) ; i++) {
            ret = mI2c_.write(0x78, OV5642_InitRegTable[i], 3);
            if (ret != 0) {
                return false;
            }
        }

        return true;
    }

    virtual void SetExtInConfig(DisplayBase::video_ext_in_config_t * p_cfg) {
        p_cfg->inp_format     = DisplayBase::VIDEO_EXTIN_FORMAT_BT601; /* BT601 8bit YCbCr format */
        p_cfg->inp_pxd_edge   = DisplayBase::EDGE_RISING;              /* Clock edge select for capturing data          */
        p_cfg->inp_vs_edge    = DisplayBase::EDGE_RISING;              /* Clock edge select for capturing Vsync signals */
        p_cfg->inp_hs_edge    = DisplayBase::EDGE_RISING;              /* Clock edge select for capturing Hsync signals */
        p_cfg->inp_endian_on  = DisplayBase::OFF;                      /* External input bit endian change on/off       */
        p_cfg->inp_swap_on    = DisplayBase::OFF;                      /* External input B/R signal swap on/off         */
        p_cfg->inp_vs_inv     = DisplayBase::SIG_POL_NOT_INVERTED;     /* External input DV_VSYNC inversion control     */
        p_cfg->inp_hs_inv     = DisplayBase::SIG_POL_NOT_INVERTED;     /* External input DV_HSYNC inversion control     */
        p_cfg->inp_f525_625   = DisplayBase::EXTIN_LINE_525;           /* Number of lines for BT.656 external input */
        p_cfg->inp_h_pos      = DisplayBase::EXTIN_H_POS_YCBYCR;       /* Y/Cb/Y/Cr data string start timing to Hsync reference */
        p_cfg->cap_vs_pos     = 8;                                     /* Capture start position from Vsync */
        p_cfg->cap_hs_pos     = 8;                                     /* Capture start position form Hsync */
        p_cfg->cap_width      = 640;                                   /* Capture width  */
        p_cfg->cap_height     = 480u;                                  /* Capture height Max 480[line] */
    }

};

#endif

