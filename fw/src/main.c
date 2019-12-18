#include "stm32f030x6.h"
#include "user_i2c.h"
#include "tb6612.h"
#include "boot.h"
#include "gpio.h"
#include "pins.h"

#define I2C_BASE_ADDR           0x2d

#define MAX_PKT_LEN             32

#define ERR_TIMEOUT             (-1)
#define ERR_ABORTED             (-2)
#define ERR_NOTIMPL             (-3)


__IO uint32_t VectorTable[48] __attribute__ ((section (".ram_vector")));


static uint8_t buf[MAX_PKT_LEN];
volatile uint32_t timeout = 0;


void SysTick_Handler(void)
{
    if (timeout)
        timeout--;
}

static int receive_cmd(uint8_t *buf, uint8_t size)
{
    uint8_t *pbuf = buf;
    uint8_t len = 0;

    /* Reenable interface to reset it`s state */
    I2C1->CR1 = 0;
    while (I2C1->CR1 & I2C_CR1_PE);
    I2C1->CR1 = I2C_CR1_PE;
    while ((I2C1->CR1 & I2C_CR1_PE) == 0);

    /* Clear all interrupt flags */
    I2C1->ICR = 0xffffffff;

    /* Wait own addr match */
    while ((I2C1->ISR & I2C_ISR_ADDR) == 0);
    I2C1->ICR = I2C_ICR_ADDRCF;

    /* Send responce for a general read command */
    if (I2C1->ISR & I2C_ISR_DIR) {
        /* Just reply own addr back */
        I2C1->TXDR = (I2C1->OAR1 & 0xFF) >> 1;
        while(!(I2C1->ISR & I2C_ISR_TXE));

        timeout = 2;
        while (((I2C1->ISR & I2C_ISR_STOPF) == 0) && (timeout));
        if (!timeout)
            return ERR_TIMEOUT;

        return 0;
    }

    /* Receive 1st byte of data */
    timeout = 4;
    while ((I2C1->ISR & I2C_ISR_RXNE) == 0 && 
        (I2C1->ISR & I2C_ISR_STOPF) == 0 && 
        timeout);
    if (!timeout)
        return ERR_TIMEOUT;
    if (I2C1->ISR & I2C_ISR_STOPF)
        return ERR_ABORTED;
    *buf++ = I2C1->RXDR;

    /* Wait the rest of data bytes or 2nd start bit */
    while ((I2C1->ISR & I2C_ISR_RXNE) == 0 && (I2C1->ISR & I2C_ISR_ADDR) == 0);

    /* 2nd start bit received */
    if (I2C1->ISR & I2C_ISR_ADDR) {
        I2C1->ICR = I2C_ICR_ADDRCF;

        /* Read direction */
        if (I2C1->ISR & I2C_ISR_DIR) {
            uint8_t len = handle_cmd(CMD_READ, pbuf, size);
            /* Send a reply */
            for (uint8_t i = 0; i < len; i++) {
                I2C1->TXDR = pbuf[i];
                timeout = 2;
                while(!(I2C1->ISR & I2C_ISR_TXE) && !(I2C1->ISR & I2C_ISR_NACKF) && (timeout));
                /* Check if master has interrupted the transmission */
                if ((I2C1->ISR & I2C_ISR_NACKF)) {
                    break;
                }
                if (!timeout)
                    return ERR_TIMEOUT;
            }

            timeout = 2;
            while (((I2C1->ISR & I2C_ISR_STOPF) == 0) && (timeout));
            if (!timeout)
                return ERR_TIMEOUT;

            return 0;
        } else {
            /* 2nd start for write direction is not supported */
            return ERR_NOTIMPL;
        }
    }

    /* Receive the rest of data bytes */
    timeout = 2;
    for (len = 1; len < MAX_PKT_LEN; len++) {
        while ((I2C1->ISR & I2C_ISR_RXNE) == 0 && 
            (I2C1->ISR & I2C_ISR_STOPF) == 0 && 
            timeout);
        if (!timeout)
            return ERR_TIMEOUT;
        if (I2C1->ISR & I2C_ISR_STOPF)
            return len;
        *buf++ = I2C1->RXDR;
    }

    while (((I2C1->ISR & I2C_ISR_STOPF) == 0) && (timeout));
    if (!timeout)
        return ERR_TIMEOUT;

    I2C1->ICR = I2C_ICR_STOPCF;

    return len;
}

static void init_gpio(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOFEN;

    /* TB6612 pins */
    GPIOA->MODER |= MODER(MODE_OUT, PIN_AIN1) | MODER(MODE_OUT, PIN_AIN2) |
        MODER(MODE_OUT, PIN_BIN1) | MODER(MODE_OUT, PIN_BIN2) |
        MODER(MODE_AF, PIN_PWMA) | MODER(MODE_AF, PIN_PWMB) |
        MODER(MODE_OUT, PIN_STBY);
    GPIOA->AFR[0] |= AFR(AF1, PIN_PWMA) | AFR(AF1, PIN_PWMB);
    // GPIOA->AFR[0] |= (1 << GPIO_AFRH_AFRH6_Pos) | (1 << GPIO_AFRH_AFRH7_Pos);

    /* EXTI pins */
    GPIOA->MODER |= MODER(MODE_IN, PIN_IRQA) | MODER(MODE_IN, PIN_IRQB);
    GPIOF->PUPDR |= PUPDR(PULL_UP, PIN_IRQA) | PUPDR(PULL_UP, PIN_IRQB);

    /* Address selection pins */
    GPIOF->MODER |= MODER(MODE_IN, PIN_AD0) | MODER(MODE_IN, PIN_AD1);
    GPIOF->PUPDR |= PUPDR(PULL_UP, PIN_AD0) | PUPDR(PULL_UP, PIN_AD1);
}

static void init_exti(void)
{
    /* External interrupt is set by default to PORTA */
    // SYSCFG->EXTICR[(PIN_IRQA / 4)] |= (1 << ((PIN_IRQA % 4) * 4));
    // SYSCFG->EXTICR[(PIN_IRQB / 4)] |= (1 << ((PIN_IRQB % 4) * 4));

    /* Enable the 'falling edge' trigger */
    EXTI->IMR |= (1 << PIN_IRQA) | (1 << PIN_IRQB);
    EXTI->FTSR |= (1 << PIN_IRQA) | (1 << PIN_IRQB);

    /* Enable the NVIC interrupt and set it low priority. */
    NVIC_SetPriority(EXTI4_15_IRQn, 0x03);
    NVIC_EnableIRQ(EXTI4_15_IRQn);
}


static void init_i2c(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    GPIOA->MODER |= MODER(MODE_AF, PIN_SCL) | MODER(MODE_AF, PIN_SDA);
    GPIOA->AFR[1] |= AFR(AF4, PIN_SCL) | AFR(AF4, PIN_SDA);
    // GPIOA->AFR[1] |= (4 << GPIO_AFRH_AFRH1_Pos) | (4 << GPIO_AFRH_AFRH2_Pos);
    GPIOA->OTYPER |= OTYPER(OTYPE_OD, PIN_SCL) | OTYPER(OTYPE_OD, PIN_SDA);
    GPIOA->PUPDR |= PUPDR(PULL_UP, PIN_SCL) | PUPDR(PULL_UP, PIN_SDA);

    I2C1->OAR1 = I2C_OAR1_OA1EN | ((I2C_BASE_ADDR + (GPIOF->IDR & 3)) << 1);
    I2C1->CR1 = I2C_CR1_PE;
}

static void init_pwm(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    TIM3->CCMR1 = TIM_CCMR1_OC1PE | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 |
        TIM_CCMR1_OC2PE | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1;
    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E;
    TIM3->BDTR = TIM_BDTR_MOE;
    TIM3->ARR = PWM_STEPS - 1;
    TIM3->PSC = 8000000 / (PWM_STEPS * PWM_MAX_FREQ) - 1;
    TIM3->EGR = TIM_EGR_UG;
    TIM3->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;
}

int main()
{
    /* Relocate the vector table to the internal SRAM at 0x20000000 */
    __disable_irq();
    for (uint8_t i = 0; i < 48; i++) {
        VectorTable[i] = *(__IO uint32_t *)(MAIN_PROGRAM_START_ADDRESS + (i << 2));
    }
    /* Remap SRAM at 0x00000000 */
    SYSCFG->CFGR1 |= SYSCFG_CFGR1_MEM_MODE;
    /* Enable the SYSCFG peripheral clock */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;
    __enable_irq();

    init_gpio();
    init_exti();
    init_i2c();
    init_pwm();

    SysTick_Config(8000);

    while (1) {
        int8_t len = receive_cmd(buf, sizeof(buf));
        if (len > 0) {
            handle_cmd(CMD_WRITE, buf, len);
        }
    }

    return 0;
}