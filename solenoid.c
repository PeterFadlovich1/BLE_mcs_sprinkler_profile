#include "lp.h"
#include "nvic_table.h"
#include "pb.h"
#include "board.h"
#include "gpio.h"


/***Solenoid Functions**************/
#define MXC_GPIO_PORT_INTERRUPT_STATUS1 MXC_GPIO1
#define MXC_GPIO_PIN_INTERRUPT_STATUS1 MXC_GPIO_PIN_9//31
#define MXC_GPIO_PORT_INTERRUPT_STATUS2 MXC_GPIO1
#define MXC_GPIO_PIN_INTERRUPT_STATUS2 MXC_GPIO_PIN_8//30

#define SOLENOID_TIMER MXC_TMR4
  // 0 = Solenoid closed, 1 = Solenoid open

uint16_t pin = 0;

void oneshotTimerInit8kTimer5(mxc_tmr_regs_t *timer, uint32_t ticks, mxc_tmr_pres_t prescalar)
{
    // Declare variables
    mxc_tmr_cfg_t tmr;
    /*
    Steps for configuring a timer for PWM mode:
    1. Disable the timer
    2. Set the prescale value
    3  Configure the timer for continuous mode
    4. Set polarity, timer parameters
    5. Enable Timer
    */

    MXC_TMR_Shutdown(timer);

    tmr.pres = prescalar;
    tmr.mode = TMR_MODE_ONESHOT;
    tmr.bitMode = TMR_BIT_MODE_32;
    tmr.clock = MXC_TMR_32K_CLK; //was 32k switched to 8k for timer 5 usage as a one shot
    tmr.cmp_cnt = ticks; //SystemCoreClock*(1/interval_time);
    tmr.pol = 0;

    if (MXC_TMR_Init(timer, &tmr, true) != E_NO_ERROR) {
        printf("Failed Continuous timer Initialization.\n");
        return;
    }

    MXC_TMR_EnableInt(timer);

    // Clear Wakeup status
    MXC_LP_ClearWakeStatus();
    // Enable wkup source in Poower seq register
    MXC_LP_EnableTimerWakeup(timer);
    // Enable Timer wake-up source
    MXC_TMR_EnableWakeup(timer, &tmr);

    //MXC_TMR_SetCount(timer,0);

    //MXC_TMR_ClearFlags(timer);
    printf("solenoid oneshot initialized\n");
    fflush(stdout);
    
}

void openSolenoid()
{
    MXC_GPIO_OutToggle(MXC_GPIO_PORT_INTERRUPT_STATUS2, MXC_GPIO_PIN_INTERRUPT_STATUS2);
    MXC_TMR_Start(SOLENOID_TIMER);
    //oneshotTimerInit8kTimer5(SOLENOID_TIMER, 15,TMR_PRES_64);
    printf("turned back on1!\n");
    pin = 1;
}

void closeSolenoid()
{
    MXC_GPIO_OutToggle(MXC_GPIO_PORT_INTERRUPT_STATUS1, MXC_GPIO_PIN_INTERRUPT_STATUS1);
    MXC_TMR_Start(SOLENOID_TIMER);
    //oneshotTimerInit8kTimer5(SOLENOID_TIMER, 15,TMR_PRES_64);
    printf("turned back on2!\n");
    pin = 2;
}

void SolenoidOSTHandler(void)
{
    // Clear 
    printf("solenoid interrupt");
    fflush(stdout);
    MXC_TMR_ClearFlags(SOLENOID_TIMER);
    //MXC_TMR_Shutdown(SOLENOID_TIMER);

    // Clear interrupt
    if (SOLENOID_TIMER->wkfl & MXC_F_TMR_WKFL_A) {
        SOLENOID_TIMER->wkfl = MXC_F_TMR_WKFL_A;
        MXC_GPIO_OutToggle( MXC_GPIO_PORT_INTERRUPT_STATUS1,  pin == 1 ? MXC_GPIO_PIN_INTERRUPT_STATUS2 : MXC_GPIO_PIN_INTERRUPT_STATUS1);
    }
}



void solenoidInit(){
    //MXC_TMR_ClearFlags(SOLENOID_TIMER);
    mxc_gpio_cfg_t gpio_interrupt_status1;
    mxc_gpio_cfg_t gpio_interrupt_status2;

    MXC_NVIC_SetVector(TMR4_IRQn, SolenoidOSTHandler);
    NVIC_EnableIRQ(TMR4_IRQn);
    oneshotTimerInit8kTimer5(SOLENOID_TIMER, 15,TMR_PRES_64); //30ms on 32k clock 15
    //MXC_TMR_Shutdown(SOLENOID_TIMER);

    gpio_interrupt_status1.port = MXC_GPIO_PORT_INTERRUPT_STATUS1;
    gpio_interrupt_status1.mask = MXC_GPIO_PIN_INTERRUPT_STATUS1;
    gpio_interrupt_status1.pad = MXC_GPIO_PAD_NONE;
    gpio_interrupt_status1.func = MXC_GPIO_FUNC_OUT;
    gpio_interrupt_status1.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&gpio_interrupt_status1);
    MXC_GPIO_OutSet(MXC_GPIO_PORT_INTERRUPT_STATUS1,  MXC_GPIO_PIN_INTERRUPT_STATUS1);

    gpio_interrupt_status2.port = MXC_GPIO_PORT_INTERRUPT_STATUS2;
    gpio_interrupt_status2.mask = MXC_GPIO_PIN_INTERRUPT_STATUS2;
    gpio_interrupt_status2.pad = MXC_GPIO_PAD_NONE;
    gpio_interrupt_status2.func = MXC_GPIO_FUNC_OUT;
    gpio_interrupt_status2.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&gpio_interrupt_status2);

    MXC_GPIO_OutClr(MXC_GPIO_PORT_INTERRUPT_STATUS2,  MXC_GPIO_PIN_INTERRUPT_STATUS2);

}

void GPIOINIT(){
    mxc_gpio_cfg_t gpio_moisture_enable1;
    mxc_gpio_cfg_t gpio_moisture_enable2;

    gpio_moisture_enable1.port = MXC_GPIO0;
    gpio_moisture_enable1.mask = MXC_GPIO_PIN_21;
    gpio_moisture_enable1.pad = MXC_GPIO_PAD_NONE;
    gpio_moisture_enable1.func = MXC_GPIO_FUNC_OUT;
    gpio_moisture_enable1.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&gpio_moisture_enable1);

    gpio_moisture_enable2.port = MXC_GPIO0;
    gpio_moisture_enable2.mask = MXC_GPIO_PIN_22;
    gpio_moisture_enable2.pad = MXC_GPIO_PAD_NONE;
    gpio_moisture_enable2.func = MXC_GPIO_FUNC_OUT;
    gpio_moisture_enable2.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&gpio_moisture_enable2);
}