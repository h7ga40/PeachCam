/////////////////////////////////////////////////////////////////////////////////
// bh1792.c
// 
// Copyright (c) 2016-17 ROHM Co.,Ltd.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
/////////////////////////////////////////////////////////////////////////////////

#include "bh1792.h"

/////////////////////////////////////////////////////////////////////////////////
//  Public Functions
/////////////////////////////////////////////////////////////////////////////////

BH1792::BH1792(PinName sda_pin, PinName scl_pin) :
	wire(sda_pin, scl_pin),
	i2c_err(0),
	is_measuring(0),
	sync_seq(0)
{
	wire.frequency(400000L);
}

//===============================================================================
// @brief  BH1792 Initialization
//
// @param[in]    : None
// @param[out]   : None
// @param[inout] : BH1792::t *pBH1792
// @retval       : BH1792_SUCCESS
//                 BH1792_I2C_ERR
//                 BH1792_NOT_EXIST
//===============================================================================
int32_t BH1792::Init()
{
	int32_t ret = BH1792_SUCCESS;
	int32_t ret_i2c = 0;
	uint8_t reg[1 + 1];

	// Set inital register parameters
	this->prm.sel_adc = BH1792_PRM_SEL_ADC_GREEN;
	this->prm.msr = BH1792_PRM_MSR_32HZ;
	this->prm.led_en = BH1792_PRM_LED_EN1_0;
	this->prm.led_cur1 = BH1792_PRM_LED_CUR1_MIN;
	this->prm.led_cur2 = BH1792_PRM_LED_CUR2_MIN;
	this->prm.ir_th = 0xFFFC;
	this->prm.int_sel = BH1792_PRM_INT_SEL_DISABLE;

	// Clear flags
	this->i2c_err = 0;
	this->is_measuring = 0;
	this->sync_seq = 0;

	ret_i2c = i2c_read(BH1792_SLAVE_ADDR, BH1792_ADDR_PARTID, reg, 1U);
	if (ret_i2c == 0) {
		if (reg[0] != BH1792_PRM_PARTID) {
			ret = BH1792_NOT_EXIST;
		}
		else {
			reg[0] = BH1792_ADDR_RESET;
			reg[1] = BH1792_PRM_SWRESET << 7;
			ret_i2c = wire.write(BH1792_SLAVE_ADDR, (char *)reg, 2U);
		}
	}

	if (ret_i2c != 0) {
		this->i2c_err = ret_i2c;
		ret = BH1792_I2C_ERR;
	}

	return ret;
}

//===============================================================================
// @brief  BH1792 Parameter Setting
//
// @param[in]    : None
// @param[out]   : None
// @param[inout] : None
// @retval       : BH1792_SUCCESS
//                 BH1792_I2C_ERR
//===============================================================================
int32_t BH1792::SetParams(bh1792_prm_t &prm)
{
	int32_t ret = BH1792_SUCCESS;
	int32_t ret_i2c = 0;
	uint8_t reg_size = 7U;
	uint8_t reg[1 + 7];

	this->prm = prm;

	// MEAS_CTRLx register
	reg[0] = BH1792_ADDR_MEAS_CTRL1;
	reg[1] = (BH1792_PRM_RDY << 7) | (BH1792_PRM_SEL_LED << 6) | (this->prm.sel_adc << 4) | this->prm.msr;
	reg[2] = ((this->prm.led_en >> 1) << 6) | this->prm.led_cur1;
	reg[3] = ((this->prm.led_en & 0x01U) << 7) | this->prm.led_cur2;
	reg[4] = (uint8_t)this->prm.ir_th;
	reg[5] = (uint8_t)(this->prm.ir_th >> 8);
	reg[6] = this->prm.int_sel;
	// MEAS_START register
	if (this->is_measuring > 0) {
		reg[7] = BH1792_PRM_MEAS_ST;
		reg_size = 8U;
	}

	ret_i2c = wire.write(BH1792_SLAVE_ADDR, (char *)reg, reg_size);
	if (ret_i2c != 0) {
		this->i2c_err = ret_i2c;
		ret = BH1792_I2C_ERR;
	}

	return ret;
}

//===============================================================================
// @brief  BH1792 Measurement Start
//
// @param[in]    : None
// @param[out]   : None
// @param[inout] : None
// @retval       : BH1792_SUCCESS
//                 BH1792_I2C_ERR
//===============================================================================
int32_t BH1792::StartMeasure(void)
{
	int32_t ret = BH1792_SUCCESS;
	int32_t ret_i2c = 0;
	uint8_t reg[1 + 1];

	if (this->prm.msr < BH1792_PRM_MSR_SINGLE) {
		this->is_measuring = 1;

		// Initialize moving average parameters
		this->ma_prm.sum.off = 0U;
		this->ma_prm.sum.on = 0U;
		this->ma_prm.pos = 0;
		this->ma_prm.num = 0;

		switch (this->prm.msr) {
		case BH1792_PRM_MSR_32HZ:
		case BH1792_PRM_MSR_128HZ:
		case BH1792_PRM_MSR_64HZ:
			this->ma_prm.len = 1;
			break;
		case BH1792_PRM_MSR_256HZ:
			this->ma_prm.len = 2;
			break;
		case BH1792_PRM_MSR_1024HZ:
			this->ma_prm.len = 8;
			break;
		default:
			this->ma_prm.len = 0;
			break;
		}
	}

	reg[0] = BH1792_ADDR_MEAS_START;
	reg[1] = BH1792_PRM_MEAS_ST;
	ret_i2c = wire.write(BH1792_SLAVE_ADDR, (char *)reg, 2U);
	if (ret_i2c != 0) {
		this->i2c_err = ret_i2c;
		ret = BH1792_I2C_ERR;
	}

	return ret;
}

//===============================================================================
// @brief  BH1792 Measurement Stop
//
// @param[in]    : None
// @param[out]   : None
// @param[inout] : None
// @retval       : BH1792_SUCCESS
//                 BH1792_I2C_ERR
//===============================================================================
int32_t BH1792::StopMeasure(void)
{
	int32_t ret = BH1792_SUCCESS;
	int32_t ret_i2c = 0;
	uint8_t reg[1 + 1];

	this->is_measuring = 0;
	this->sync_seq = 0;

	reg[0] = BH1792_ADDR_RESET;
	reg[1] = BH1792_PRM_SWRESET << 7;
	ret_i2c = wire.write(BH1792_SLAVE_ADDR, (char *)reg, 2U);
	if (ret_i2c != 0) {
		this->i2c_err = ret_i2c;
		ret = BH1792_I2C_ERR;
	}

	return ret;
}

//===============================================================================
// @brief  BH1792 Measurement Data
//
// @param[in]    : None
// @param[out]   : bh1792_data_t *dat
// @param[inout] : None
// @retval       : BH1792_SUCCESS
//                 BH1792_I2C_ERR
//                 BH1792_FIFO_FULL
//===============================================================================
int32_t BH1792::GetMeasData(bh1792_data_t *dat)
{
	int32_t ret = BH1792_SUCCESS;
	int32_t ret_i2c = 0;
	uint8_t int_clear = 0U;
	uint8_t fifo_level = 0U;

	if (this->prm.msr <= BH1792_PRM_MSR_1024HZ) {
		ret_i2c = i2c_read(BH1792_SLAVE_ADDR, BH1792_ADDR_FIFO_LEV, &dat->fifo_lev, 1U);
		if (ret_i2c == 0) {
			if (dat->fifo_lev == BH1792_PRM_FIFO_LEV_FULL) {
				ret = BH1792_FIFO_FULL;
			}

			if (this->prm.int_sel == BH1792_PRM_INT_SEL_WTM) {
				dat->fifo_lev = BH1792_PRM_FIFO_LEV_WTM;
			}

			ret_i2c = getFifoData(dat);
			if (ret_i2c == 0) {
				ret_i2c = i2c_read(BH1792_SLAVE_ADDR, BH1792_ADDR_FIFO_LEV, &fifo_level, 1U);
			}
		}
	}
	else {
		dat->fifo_lev = 0U;
		ret_i2c = getDataOut(dat);

		if (ret_i2c == 0) {
			if (this->prm.int_sel >= BH1792_PRM_INT_SEL_IR) {
				ret_i2c = i2c_read(BH1792_SLAVE_ADDR, BH1792_ADDR_INT_CLEAR, &int_clear, 1U);
			}
		}
	}

	if (ret_i2c != 0) {
		this->i2c_err = ret_i2c;
		ret = BH1792_I2C_ERR;
	}

	return ret;
}

//===============================================================================
// @brief  BH1792 Synchronization
//
// @param[in]    : None
// @param[out]   : None
// @param[inout] : None
// @retval       : BH1792_SUCCESS
//                 BH1792_I2C_ERR
//                 BH1792_NOT_EXIST
//                 BH1792_FIFO_FULL
//===============================================================================
int32_t BH1792::SetSync(void)
{
	int32_t ret = BH1792_SUCCESS;
	int32_t ret_i2c = 0;
	uint8_t reg[1 + 1];

	reg[0] = BH1792_ADDR_MEAS_SYNC;
	reg[1] = BH1792_PRM_MEAS_SYNC;
	ret_i2c = wire.write(BH1792_SLAVE_ADDR, (char *)reg, 2U);
	if (ret_i2c != 0) {
		this->i2c_err = ret_i2c;
		ret = BH1792_I2C_ERR;
	}
	else {
		if (this->sync_seq < 3) {
			this->sync_seq++;
		}
	}

	return ret;
}

//===============================================================================
// @brief  BH1792 FIFO DATA Clear
//
// @param[in]    : None
// @param[out]   : None
// @param[inout] : None
// @retval       : BH1792_SUCCESS
//                 BH1792_I2C_ERR
//===============================================================================
int32_t BH1792::ClearFifoData(void)
{
	int32_t ret = BH1792_SUCCESS;
	int32_t ret_i2c = 0;
	uint8_t i = 0U;
	uint8_t r_cnt = 35U;
	uint8_t fifo_level = 0U;
	uint8_t reg[4];

	if (this->prm.msr == BH1792_PRM_MSR_32HZ) {
		r_cnt = 32U;
	}

	for (i = 0; i < r_cnt; i++) {
		ret_i2c = i2c_read(BH1792_SLAVE_ADDR, BH1792_ADDR_FIFO_DATA0_LSBS, reg, 4U);
		if (ret_i2c != 0) {
			break;
		}
	}

	if (ret_i2c == 0) {
		ret_i2c = i2c_read(BH1792_SLAVE_ADDR, BH1792_ADDR_FIFO_LEV, &fifo_level, 1U);
	}

	if (ret_i2c != 0) {
		this->i2c_err = ret_i2c;
		ret = BH1792_I2C_ERR;
	}

	return ret;
}


/////////////////////////////////////////////////////////////////////////////////
//  Local Functions
/////////////////////////////////////////////////////////////////////////////////

//===============================================================================
// @brief  BH1792 IR/GREEN DATA
//
// @param[in]    : None
// @param[out]   : bh1792_data_t *dat
// @param[inout] : None
// @retval       : I2C Error Code
//===============================================================================
int32_t BH1792::getDataOut(bh1792_data_t *dat)
{
	int32_t ret_i2c = 0;
	uint8_t idx = 0U;
	uint8_t reg[8] = { 0 };

	if (this->prm.sel_adc == BH1792_PRM_SEL_ADC_GREEN) {
		idx = 4U;
	}

	ret_i2c = i2c_read(BH1792_SLAVE_ADDR, (BH1792_ADDR_IRDATA_LEDOFF_LSBS + idx), &reg[idx], 4U);
	if (ret_i2c == 0) {
		dat->ir.off = ((uint16_t)reg[1] << 8) | (uint16_t)reg[0];
		dat->ir.on = ((uint16_t)reg[3] << 8) | (uint16_t)reg[2];
		dat->green.off = ((uint16_t)reg[5] << 8) | (uint16_t)reg[4];
		dat->green.on = ((uint16_t)reg[7] << 8) | (uint16_t)reg[6];
	}

	return ret_i2c;
}

//===============================================================================
// @brief  BH1792 FIFO DATA
//
// @param[in]    : None
// @param[out]   : bh1792_data_t *dat
// @param[inout] : None
// @retval       : I2C Error Code
//===============================================================================
int32_t BH1792::getFifoData(bh1792_data_t *dat)
{
	int32_t ret_i2c = 0;
	uint8_t i = 0U;
	uint8_t reg[4];
	bh1792_maPrm_t *ma_prm;

	ma_prm = &this->ma_prm;

	for (i = 0U; i < dat->fifo_lev; i++) {
		ret_i2c = i2c_read(BH1792_SLAVE_ADDR, BH1792_ADDR_FIFO_DATA0_LSBS, reg, 4U);
		if (ret_i2c != 0) {
			break;
		}
		dat->fifo[i].off = ((uint16_t)reg[1] << 8) | (uint16_t)reg[0];
		dat->fifo[i].on = ((uint16_t)reg[3] << 8) | (uint16_t)reg[2];

		// Moving Average
		if (ma_prm->num == ma_prm->len) {
			ma_prm->sum.off -= ma_prm->buf[ma_prm->pos].off;
			ma_prm->sum.on -= ma_prm->buf[ma_prm->pos].on;
		}
		ma_prm->sum.off += dat->fifo[i].off;
		ma_prm->sum.on += dat->fifo[i].on;
		ma_prm->buf[ma_prm->pos].off = dat->fifo[i].off;
		ma_prm->buf[ma_prm->pos].on = dat->fifo[i].on;

		ma_prm->pos++;
		if (ma_prm->pos >= ma_prm->len) {
			ma_prm->pos = 0;
		}
		if (ma_prm->num < ma_prm->len) {
			ma_prm->num++;
		}

		if (ma_prm->num > 0) {
			dat->fifo_lpf[i].off = ma_prm->sum.off / ma_prm->num;
			dat->fifo_lpf[i].on = ma_prm->sum.on / ma_prm->num;
		}
	}

	return ret_i2c;
}

// Note:  I2C access should be completed within 0.5ms
int32_t BH1792::i2c_read(uint8_t slv_adr, uint8_t reg_adr, uint8_t *reg, uint8_t reg_size)
{
	uint8_t rc;

	rc = wire.write(slv_adr, (char *)&reg_adr, 1, true);
	if (rc != 0)
		return rc;

	rc = wire.read((int32_t)slv_adr, (char *)reg, (int32_t)reg_size);
	if (rc != 0)
		return rc;

	return rc;
}
