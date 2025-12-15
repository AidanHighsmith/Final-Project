/*****************************************************************//**
 * @file main.cpp
 *
 * @brief Basic test of nexys4 ddr mmio cores
 *
 * @author aidan highsmith
 * @version v1.0: initial release
 *********************************************************************/

// #define _DEBUG
#include <cmath>
#include "chu_init.h"
#include "gpio_cores.h"
#include "sseg_core.h"
#include "spi_core.h"

/**
 * check individual led
 * @param led_p pointer to led instance
 * @param n number of led
 */
void led_check(GpoCore *led_p, int n) {
   int i;

   for (i = 0; i < n; i++) {
      led_p->write(1, i);
      sleep_ms(100);
      led_p->write(0, i);
      sleep_ms(100);
   }
}

/**
 * Test adxl362 accelerometer using SPI
 */

void gsensor_check(SpiCore *spi_p, GpoCore *led_p) {
   const uint8_t RD_CMD = 0x0b;
   const uint8_t PART_ID_REG = 0x02;
   const uint8_t DATA_REG = 0x08;
   const float raw_max = 127.0 / 2.0;  //128 max 8-bit reading for +/-2g

   int8_t xraw, yraw, zraw;
   float x, y, z;
   int id;

   spi_p->set_freq(400000);
   spi_p->set_mode(0, 0);
   // check part id
   spi_p->assert_ss(0);    // activate
   spi_p->transfer(RD_CMD);  // for read operation
   spi_p->transfer(PART_ID_REG);  // part id address
   id = (int) spi_p->transfer(0x00);
   spi_p->deassert_ss(0);
   uart.disp("read ADXL362 id (should be 0xf2): ");
   uart.disp(id, 16);
   uart.disp("\n\r");
   // read 8-bit x/y/z g values once
   spi_p->assert_ss(0);    // activate
   spi_p->transfer(RD_CMD);  // for read operation
   spi_p->transfer(DATA_REG);  //
   xraw = spi_p->transfer(0x00);
   yraw = spi_p->transfer(0x00);
   zraw = spi_p->transfer(0x00);
   spi_p->deassert_ss(0);
   x = (float) xraw / raw_max;
   y = (float) yraw / raw_max;
   z = (float) zraw / raw_max;
   uart.disp("x/y/z axis g values: ");
   uart.disp(x, 3);
   uart.disp(" / ");
   uart.disp(y, 3);
   uart.disp(" / ");
   uart.disp(z, 3);
   uart.disp("\n\r");
}


float get_zmotion(SpiCore *spi_p, GpoCore *led_p) {
   const uint8_t RD_CMD = 0x0b;
   const uint8_t PART_ID_REG = 0x02;
   const uint8_t DATA_REG = 0x08;
   const float raw_max = 127.0 / 2.0;  //128 max 8-bit reading for +/-2g

   int8_t xraw, yraw, zraw;
   float x, y, z;
   int id;

   //spi_p->set_freq(400000);
   //spi_p->set_mode(0, 0);
   /* check part id
   spi_p->assert_ss(0);    // activate
   spi_p->transfer(RD_CMD);  // for read operation
   spi_p->transfer(PART_ID_REG);  // part id address
   id = (int) spi_p->transfer(0x00);
   spi_p->deassert_ss(0); */
   /* Display for testing purposes
   uart.disp("read ADXL362 id (should be 0xf2): ");
   uart.disp(id, 16);
   uart.disp("\n\r");
   */
   // read 8-bit x/y/z g values once
   spi_p->assert_ss(0);    // activate
   spi_p->transfer(RD_CMD);  // for read operation
   spi_p->transfer(DATA_REG);  //
   xraw = spi_p->transfer(0x00);
   yraw = spi_p->transfer(0x00);
   zraw = spi_p->transfer(0x00);
   spi_p->deassert_ss(0);
   //x = (float) xraw / raw_max;
   //y = (float) yraw / raw_max;
   z = (float) zraw / raw_max;
   uart.disp(z, 3);
   uart.disp("\n\r");

   return z;
}

void fall_panic(GpoCore *led_p){
   int i;

   for (i = 0; i < 20; i++) {
       led_p->write(0xffff);
       sleep_ms(100);
       led_p->write(0x0000);
       sleep_ms(100);
       //debug("timer check - (loop #)/now: ", i, now_ms());
   }
}

void monitor_fall(SpiCore *spi_p, GpoCore *led_p){
   const float FREE_FALL_THRESHOLD=0.15, COUNT_THRESHOLD=30, IMPACT_THRESHOLD=1.8;
   int freeFall=0, fall=0, count=0;
   float z;

   //GpoCore led(get_slot_addr(BRIDGE_BASE, S2_LED));
   //SpiCore spi(get_slot_addr(BRIDGE_BASE, S9_SPI));
   

   while(!fall){
       sleep_ms(10);
       z = get_zmotion(spi_p, led_p);
       if (!freeFall && ((std::abs(z))) <= FREE_FALL_THRESHOLD) {
           //possible crash
           fall = 0;
           freeFall = 1;
           sleep_ms(300);
       }

       if (freeFall && (!fall || count < COUNT_THRESHOLD)) {
           count++;
           if(std::abs(z) >= IMPACT_THRESHOLD) {
               fall=1;
               freeFall=0;
               count=0;
           }
       }

       if (count > COUNT_THRESHOLD) {
           freeFall=0;
           count=0;
       }

       if (fall) {
           fall_panic(led_p);
           fall=0;
       }
   }

}

GpoCore led(get_slot_addr(BRIDGE_BASE, S2_LED));
SpiCore spi(get_slot_addr(BRIDGE_BASE, S9_SPI));


int main() {
   spi.set_freq(400000);
   spi.set_mode(0, 0);
   while (1) {
      //led_check(&led, 16);
      //gsensor_check(&spi, &led);
      monitor_fall(&spi, &led);
   }
}