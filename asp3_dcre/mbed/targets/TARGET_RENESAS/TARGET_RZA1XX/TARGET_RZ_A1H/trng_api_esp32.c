/* mbed Microcontroller Library
 * Copyright (c) 2006-2017 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(DEVICE_TRNG)
#include "mbed.h"

#define ESP32_I2C_ADDR    (0x28<<1)

void trng_init_esp32(void) {
    /* P3_10(EN), P3_9(IO0) */
    if (((GPIOP3   & 0x0400) == 0)
     || ((GPIOPMC3 & 0x0400) != 0)
     || ((GPIOPM3  & 0x0400) != 0)

     || ((GPIOP3   & 0x0200) == 0)
     || ((GPIOPMC3 & 0x0200) != 0)
     || ((GPIOPM3  & 0x0200) != 0)) {

        /* P3_10(EN) */
        GPIOP3   &= ~0x0400;         /* Outputs low level */
        GPIOPMC3 &= ~0x0400;         /* Port mode */
        GPIOPM3  &= ~0x0400;         /* Output mode */

        /* P3_9(IO0) */
        GPIOP3   &= ~0x0200;         /* Outputs low level */
        GPIOPMC3 &= ~0x0200;         /* Port mode */
        GPIOPM3  &= ~0x0200;         /* Output mode */

        GPIOP3   |=  0x2000;         /* Outputs hi level */
        wait_ms(5);
        GPIOP3   |=  0x0400;         /* Outputs hi level */
    }
}

void trng_free_esp32(void) {
    // do nothing
}

int trng_get_bytes_esp32(uint8_t *output, size_t length, size_t *output_length) {
    I2C mI2c(I2C_SDA, I2C_SCL);
    int ret;
    char send_data[1];
    char recv_data[4];
    size_t idx = 0;
    int i;
    int err_cnt = 0;

    while (idx < length) {
        send_data[0] = 0;
        ret = mI2c.write(ESP32_I2C_ADDR, send_data, 1);
        if (ret == 0) {
            mI2c.read(ESP32_I2C_ADDR, recv_data, sizeof(recv_data));
            for (i = 0; (i < 4) && (idx < length); i++) {
                output[idx++] = recv_data[i];
            }
        } else {
            err_cnt++;
            if (err_cnt >= 20) {
                break;
            }
            wait_ms(100);
        }
    }
    if (output_length != NULL) {
        *output_length = idx;
    }

    return (idx != 0 ? 0 : -1);
}
#endif
