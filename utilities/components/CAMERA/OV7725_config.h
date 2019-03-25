
#ifndef OV7725_CONFIG_H
#define OV7725_CONFIG_H

#include "mbed.h"
#include "camera_config.h"

class OV7725_config : public camera_config {

public:

    /** Initialise
     *
     * @return true = success, false = failure
     */
    virtual bool Initialise() {
        /* OV7725 camera input config */
        const char OV7725_InitRegTable[][2] = {
                {'\x0D', '\x41'}, /* COM4 */
                {'\x0F', '\xC5'}, /* COM6 */
            #if 0 /* 30fps(24MHz) */
                {'\x11', '\x01'}, /* CLKRC */
            #else /* 60fps(48MHz) */
                {'\x11', '\x00'}, /* CLKRC */
            #endif
                {'\x14', '\x1F'}, /* COM9 Drop VSYNC/HREF */
                {'\x15', '\x40'}, /* COM10 HSYNC*/
                {'\x17', '\x22'}, /* HSTART */
                {'\x18', '\xA4'}, /* HSIZE */
                {'\x19', '\x07'}, /* VSTRT */
                {'\x1A', '\xF0'}, /* VSIZE */
                {'\x22', '\x99'}, /* BDBase */
                {'\x23', '\x02'}, /* BDMStep */
                {'\x24', '\x60'}, /* AEW */
                {'\x25', '\x50'}, /* AEB */
                {'\x26', '\xA1'}, /* VPT */
                {'\x29', '\xA0'}, /* HOutSize */
                {'\x2A', '\x00'}, /* EXHCH */
                {'\x2B', '\x00'}, /* EXHCL */
                {'\x2C', '\xF0'}, /* VOutSize */
                {'\x32', '\x00'}, /* HREF */
                {'\x33', '\x01'}, /* DM_LNL */
                {'\x3D', '\x03'}, /* COM12 */
                {'\x42', '\x7F'}, /* TGT_B */
                {'\x4D', '\x09'}, /* FixGain */
                {'\x63', '\xE0'}, /* AWB_Ctrl0 */
                {'\x64', '\xFF'}, /* DSP_Ctrl1 */
                {'\x65', '\x20'}, /* DSP_Ctrl2 */
                {'\x66', '\x00'}, /* DSP_Ctrl3 */
                {'\x67', '\x48'}, /* DSP_Ctrl4 */
                {'\x6B', '\xAA'}, /* AWBCtrl3 */
                {'\x7E', '\x04'}, /* GAM1 */
                {'\x7F', '\x0E'}, /* GAM2 */
                {'\x80', '\x20'}, /* GAM3 */
                {'\x81', '\x43'}, /* GAM4 */
                {'\x82', '\x53'}, /* GAM5 */
                {'\x83', '\x61'}, /* GAM6 */
                {'\x84', '\x6D'}, /* GAM7 */
                {'\x85', '\x76'}, /* GAM8 */
                {'\x86', '\x7E'}, /* GAM9 */
                {'\x87', '\x86'}, /* GAM10 */
                {'\x88', '\x94'}, /* GAM11 */
                {'\x89', '\xA1'}, /* GAM12 */
                {'\x8A', '\xBA'}, /* GAM13 */
                {'\x8B', '\xCF'}, /* GAM14 */
                {'\x8C', '\xE3'}, /* GAM15 */
                {'\x8D', '\x26'}, /* SLOP */
                {'\x90', '\x05'}, /* EDGE1 */
                {'\x91', '\x01'}, /* DNSOff */
                {'\x92', '\x05'}, /* EDGE2 */
                {'\x93', '\x00'}, /* EDGE3 */
                {'\x94', '\x80'}, /* MTX1 */
                {'\x95', '\x7B'}, /* MTX2 */
                {'\x96', '\x06'}, /* MTX3 */
                {'\x97', '\x1E'}, /* MTX4 */
                {'\x98', '\x69'}, /* MTX5 */
                {'\x99', '\x86'}, /* MTX6 */
                {'\x9A', '\x1E'}, /* MTX_Ctrl */
                {'\x9B', '\x00'}, /* BRIGHT */
                {'\x9C', '\x20'}, /* CNST */
                {'\x9E', '\x81'}, /* UVADJ0 */
                {'\xA6', '\x04'}, /* SDE */
        };
        const char sw_reset_cmd[2] = {'\x12', '\x80'};
        int ret;
        I2C mI2c_(I2C_SDA, I2C_SCL);
        mI2c_.frequency(150000);

        if (mI2c_.write(0x42, sw_reset_cmd, 2) != 0) {
            return false;
        }
        ThisThread::sleep_for(1);

        for (uint32_t i = 0; i < (sizeof(OV7725_InitRegTable) / 2) ; i++) {
            ret = mI2c_.write(0x42, OV7725_InitRegTable[i], 2);
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
        p_cfg->cap_vs_pos     = 4+21;                                  /* Capture start position from Vsync */
        p_cfg->cap_hs_pos     = 68;                                    /* Capture start position form Hsync */
        p_cfg->cap_width      = 640;                                   /* Capture width Max */
        p_cfg->cap_height     = 480;                                   /* Capture height Max */
    }

    /** Exposure and Gain Setting
     *
     * @param[in]      bAuto             : Automatic adjustment ON/OFF(AEC/AGC)
     * @param[in]      usManualExposure  : Exposure time at automatic adjustment OFF  (number of lines)
     * @param[in]      usManualGain      : Gain at automatic adjustment OFF （0x00-0xFF)
     * @return true = success, false = failure
     */
    static bool SetExposure(bool bAuto, uint16_t usManualExposure, uint8_t usManualGain) {
        int ret;
        char cmd[2];
        I2C mI2c_(I2C_SDA, I2C_SCL);
        mI2c_.frequency(150000);

        /* COM8(AGC Enable/AEC Enable) */
        cmd[0] = 0x13;
        ret = mI2c_.write(0x42, &cmd[0], 1);
        if (ret != 0) {
            return false;
        }
        ret = mI2c_.read(0x42, &cmd[1], 1);
        if (ret != 0) {
            return false;
        }

        cmd[0] = 0x13;
        if (bAuto) {
            cmd[1] |= (uint8_t)0x05;
        } else {
            cmd[1] &= (uint8_t)~0x05;
        }
        ret = mI2c_.write(0x42, &cmd[0], 2);
        if (ret != 0) {
            return false;
        }

        if (!bAuto) {
            /* AECH/AECL(exposure) */
            cmd[0] = 0x08;
            cmd[1] = (uint8_t)((usManualExposure & 0xFF00) >> 8);
            ret = mI2c_.write(0x42, &cmd[0], 2);
            if (ret != 0) {
                return false;
            }

            cmd[0] = 0x10;
            cmd[1] = (uint8_t)(usManualExposure & 0x00FF);
            ret = mI2c_.write(0x42, &cmd[0], 2);
            if (ret != 0) {
                return false;
            }

            /* GAIN */
            cmd[0] = 0x00;
            cmd[1] = usManualGain;
            ret = mI2c_.write(0x42, &cmd[0], 2);
            if (ret != 0) {
                return false;
            }
        }

        return true;
    }
};

#endif

