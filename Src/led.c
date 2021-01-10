
#include<stdint.h>
#include "led.h"



void delay(uint32_t count)
{
  for(uint32_t i = 0 ; i < count ; i++);
}

void led_init_all(void)
{

	uint32_t *pRccAhb1enr = (uint32_t*)0x40023830;
	uint32_t *pGpioBModeReg = (uint32_t*)0x40020400;

	// Enable GPIOB
	*pRccAhb1enr |= ( 1 << 1);
	//configure LEDs
	*pGpioBModeReg |= ( 1 << (2 * LED_1));
	*pGpioBModeReg |= ( 1 << (2 * LED_2));
	*pGpioBModeReg |= ( 1 << (2 * LED_3));
	*pGpioBModeReg |= ( 1 << (2 * LED_4));

#if 0
	//configure the outputtype
	*pGpioOpTypeReg |= ( 1 << (2 * LED_GREEN));
	*pGpioOpTypeReg |= ( 1 << (2 * LED_ORANGE));
	*pGpioOpTypeReg |= ( 1 << (2 * LED_RED));
	*pGpioOpTypeReg |= ( 1 << (2 * LED_BLUE));
#endif

    led_off(LED_1);
    led_off(LED_2);
    led_off(LED_3);
    led_off(LED_4);



}

void led_on(uint8_t led_no)
{
  uint32_t *pGpioBDataReg = (uint32_t*)0x40020414;
  *pGpioBDataReg |= ( 1 << led_no);

}

void led_off(uint8_t led_no)
{
	  uint32_t *pGpioBDataReg = (uint32_t*)0x40020414;
	  *pGpioBDataReg &= ~( 1 << led_no);

}


