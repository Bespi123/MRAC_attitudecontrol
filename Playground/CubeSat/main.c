/***********************
 * IAAPP - UNSA
 * Autor: Brayan Espinoza (Bespi123)
 * Description:
 * This program send MPU6050 conected to I2C1 througth Serial 5 port.
 * IMPORTANT: Change stack size into 2k and compiled into c99 dialect.
 ***********************/

//--------------------------Program Lybraries-------------------------
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "inc/tm4c123gh6pm.h"

//-------------------------Program Definitions------------------------
//  MPU 6050 DEFINITIONS
#define MPU_6050_ADDR           0x68    //MPU 6050 ADDRESS

// MPU 6050 REGISTERS
#define  Reg_PWR_MGMT_1         0x6B    // Power management 1
#define  Reg_INT_PIN_CFG        0x37    // Interrupt pin configuration
#define  Reg_CONFIG             0x1A    // Configuration
#define  Reg_ACC_CONFIG         0x1C    // Accel configuration
#define  Reg_GYRO_CONFIG        0x1B    // Gyro configurations
#define  Reg_INT_PIN_CFG        0x37    // Int pin enable configuration
#define  Reg_INT_ENABLE         0x38    // Enables interrupt generation
#define  Reg_INT_STATUS         0x3A    // Shows interrupt status
#define  Reg_ACC_XOUT_H         0x3B    // AccX data MSB
#define  Reg_ACC_YOUT_H         0x3D    // AccY data MSB
#define  Reg_ACC_ZOUT_H         0x3F    // AccZ data MSB
#define  Reg_TEMP_OUT           0x41    // Temp data MSB
#define  Reg_GYRO_XOUT_H        0x43    // GyroX data MSB
#define  Reg_GYRO_YOUT_H        0x45    // GyroY data MSB
#define  Reg_GYRO_ZOUT_H        0x47    // GyroZ data MSB
#define  Reg_WHO_I_AM           0x75    // Read Address

int32_t Count1,Count2 = 0;

//------------------PROGRAM FUNCTIONS-------------------
//  I2C Functions
void I2c1_begin(void);
char I2C1_writeByte(int slaveAddr, char memAddr, char data);
char I2C1_readBytes(int slaveAddr, char memAddr, int byteCount, char* data);
static int I2C_wait_till_done(void);

// Interrupt functions (to erase)
void GPIO(void);
void PortF1_IntEnable(void);

//  UART Functions
void Uart5_begin(void);
void UART5_Transmitter(unsigned char data);
void UART5_printString(char *str);

//  MPU6050 Functions
bool MPU6050_Init(void);
void MPU6050_getData(int16_t *MPURawData);
void Delay(unsigned long counter);

//----------------PROGRAM GLOBAL VARIABLES--------------
int16_t MPURawData[7];
char m_sMsg[20];
float AX, AY, AZ, t, GX, GY, GZ;      // MPU6050 Data

//---------------------MAIN PROGRAM---------------------
int main(void){

  //    Initialize I2C1 and UART5
  I2c1_begin();
  Delay(1000);
  Uart5_begin();
  MPU6050_Init();
  Delay(1000);
  GPIO();
  //PortF1_IntEnable();
  while(1){
      /*
      // Read MPU6050 in a burst of data
      MPU6050_getData(MPURawData);

      // Convert The Readings
      AX = (float)MPURawData[0]/16384.0;
      AY = (float)MPURawData[1]/16384.0;
      AZ = (float)MPURawData[2]/16384.0;
      GX = (float)MPURawData[4]/131.0;
      GY = (float)MPURawData[5]/131.0;
      GZ = (float)MPURawData[6]/131.0;
      t = ((float)MPURawData[3]/340.00)+36.53;

      //Turn into string and send trough Serial5
      sprintf(m_sMsg,"%.2f,%.2f,%.2f\n",AX,AY,AZ);
      UART5_printString(m_sMsg);
      */
    }
}

bool MPU6050_Init(void){
    //  Local variables
    char Ack;
    //  Check connection
    I2C1_readBytes(MPU_6050_ADDR, Reg_WHO_I_AM, 1, &Ack);
    if(MPU_6050_ADDR==Ack){
        //Configure MPU6050
        I2C1_writeByte(MPU_6050_ADDR,Reg_PWR_MGMT_1, 0x00);      // Wake-up, 8MHz Oscillator
        I2C1_writeByte(MPU_6050_ADDR,Reg_ACC_CONFIG, 0x00);      // Accel range +/- 2G
        I2C1_writeByte(MPU_6050_ADDR,Reg_GYRO_CONFIG,0x00);      // Gyro range +/- 250 Deg/s
        I2C1_writeByte(MPU_6050_ADDR,Reg_CONFIG, 0x00);          // Set DLPF 256Hz
        //Enable dataReady Interrupt
        I2C1_writeByte(MPU_6050_ADDR,Reg_INT_PIN_CFG,0x70);      // Low active/Open drain/Latch until read/Clear on any read
        I2C1_writeByte(MPU_6050_ADDR,Reg_INT_ENABLE, 0x01);      // Raw_ready_enable int active
        return true;
    }
    return false;
}

void MPU6050_getData(int16_t *MPURawData){
    //Local variables
    char sensordata[14];                  // Char array

    //Read 14 Registers in a single burst
    I2C1_readBytes(MPU_6050_ADDR,Reg_ACC_XOUT_H, 14, sensordata);
    //Turn into 16bits data
    for(char i = 0; i < 7; i++){
        MPURawData[i] = ((int16_t)sensordata[2*i] << 8) | sensordata[2*i+1];
    }
}

void PortF1_IntEnable(void){
    //Enable GPIOF clock
    SYSCTL_RCGCGPIO_R |= (1<<5);
    while((SYSCTL_PRGPIO_R & (1<<5))==0);

    //Initialize PORTF1
    GPIO_PORTF_DIR_R &= ~(1<<1);        // Set as digital input
    GPIO_PORTF_DEN_R |= (1<<1);         // Set as digital pin
    GPIO_PORTF_PUR_R |= (1<<1);         // Enable PORTF0 pull up resistor

    //Configure PORTF1 for falling edge trigger interrupt
    GPIO_PORTF_IS_R &=~(1<<1);          // make bit 1 edge sensitive
    GPIO_PORTF_IBE_R &=~(1<<1);         // trigger controlled by IEV
    GPIO_PORTF_IEV_R &=~(1<<1);         // falling edge trigger
    GPIO_PORTF_ICR_R |= (1<<1);         // clear any prior interrupt
    GPIO_PORTF_IM_R |= (1<<1);          // unmask interrupt

    // enable interrupt in NVIC and set priority to 3 */
    NVIC_PRI7_R = (NVIC_PRI7_R & 0xFF00FFFF)|0x00A00000;  //Priority 5
    NVIC_EN0_R |= (1<<30);  // enable IRQ30 (D30 of ISER[0])
}

/* void Uart5_begin(void)
 * Description:
 * This function initialize the UART5 Serial Port in 9600 bps
 * as data length 8-bit, not parity bit, FIFO enable.
 *
 */
void Uart5_begin(void){
    // Enable clock to UART5 and wait for the clock
    SYSCTL_RCGCUART_R |= 0x20;
    while((SYSCTL_PRUART_R & 0x20)==0);

    // Enable clock to PORTE for PE4/Rx and RE5/Tx and wait
    SYSCTL_RCGCGPIO_R |= 0x10;
    while((SYSCTL_PRGPIO_R & 0x10)==0);

    // UART5 initialization
    UART5_CTL_R &= ~0x01;                           // Disable UART5
    UART5_IBRD_R = (UART5_IBRD_R & ~0XFFFF)+104;    // for 9600 baud rate, integer = 104
    UART5_FBRD_R = (UART5_FBRD_R & ~0X3F)+11;       // for 9600 baud rate, fractional = 11

    UART5_CC_R = 0x00;                              // Select system clock (based on clock source and divisor factor)*/
    //UART5_LCRH_R = (UART5_LCRH_R & ~0XFF)| 0x70;    // Data length 8-bit, not parity bit, FIFO enable
    UART5_LCRH_R = (UART5_LCRH_R & ~0XFF)| 0x60;    // Data length 8-bit, not parity bit, FIFO disable
    UART5_CTL_R &= ~0X20;                           // HSE = 0
    UART5_CTL_R |= 0X301;                           // UARTEN = 1, RXE = 1, TXE =1

    // UART5 TX5 and RX5 use PE4 and PE5.
    // Configure them as digital and enable alternate function.
    GPIO_PORTE_DEN_R = 0x30;             // Set PE4 and PE5 as digital.
    GPIO_PORTE_AFSEL_R = 0x30;           // Use PE4,PE5 alternate function.
    GPIO_PORTE_AMSEL_R = 0X00;           // Turn off analog function.
    GPIO_PORTE_PCTL_R = 0x00110000;      // Configure PE4 and PE5 for UART.

    // Enable UART5 interrupt
    UART5_ICR_R &= ~(0x0780);            // Clear receive interrupt
    UART5_IM_R  = 0x0010;                // Enable UART5 Receive interrupt
    NVIC_EN1_R  |= (1<<29);               // Enable IRQ61 for UART5
    //NVIC->ISER[1] |= 0x20000000;
}

void UART5_Transmitter(unsigned char data){
    while((UART5_FR_R & (1<<5)) != 0);      // Wait until Tx buffer is not full
    UART5_DR_R = data;                      // Before giving it another byte
}

void UART5_printString(char *str){
  while(*str){
        UART5_Transmitter(*(str++));
    }
}

void UART5_Handler(void){
    //Local variables
    unsigned char rx_data = 0;
    UART5_ICR_R &= ~(0x010);            // Clear receive interrupt
    rx_data = UART5_DR_R ;              // Get the received data byte
    if(rx_data == 'A')
        GPIO_PORTF_DATA_R |= (1<<3);
    else if(rx_data == 'B')
        GPIO_PORTF_DATA_R &= ~0X08;
    UART5_Transmitter(rx_data); // send data that is received
}

/* void I2c1_begin(void)
 *
 * Description:
 * This function initialize the I2C1 Port in 100kHz.
 *
 */
void I2c1_begin(void){
    SYSCTL_RCGCGPIO_R |= 0x01;              // Enable the clock for port A
    while((SYSCTL_PRGPIO_R & 0x01)==0);     // Wait until clock is initialized
    SYSCTL_RCGCI2C_R   |= 0X02;             // Enable the clock for I2C 1
    while((SYSCTL_PRI2C_R & 0x02)==0);      // Wait until clock is initialized
    GPIO_PORTA_DEN_R |= 0xC0;               // PA6 and PA7 as digital

    // Configure Port A pins 6 and 7 as I2C 1
    GPIO_PORTA_AFSEL_R |= 0xC0;             // Use PA6, PA7 alternate function
    GPIO_PORTA_PCTL_R  |= 0X33000000;       // Configure PA6 and PA7 as I2C
    GPIO_PORTA_ODR_R   |= 0x80;             // SDA (PA7) pin as open Drain
    I2C1_MCR_R = 0x0010;                    // Enable I2C 1 master function

    /* Configure I2C 1 clock frequency
    (1 + TIME_PERIOD ) = SYS_CLK /(2*
    ( SCL_LP + SCL_HP ) * I2C_CLK_Freq )
    TIME_PERIOD = 16 ,000 ,000/(2(6+4) *100000) - 1 = 7 */
    I2C1_MTPR_R = 0x07;
}

/* char I2C1_writeByte( @slaveAddr, @memAddr, @data)
 * int slaveAddr: I2C Slave Address
 * char memAddr:  Register number
 * char data:   Data to send
 *
 * Returns:
 *  0 if all it's Ok and 1 if there is an error.
 *
 * Description:
 * This function write only one byte with the following frame.
 * byte write: S-(saddr+w)-ACK-maddr-ACK-data-ACK-P
 *
 */
char I2C1_writeByte(int slaveAddr, char memAddr, char data){
    char error;
    // Send slave address and starting address
    I2C1_MSA_R = (slaveAddr << 1);      // Assign Master as writing mode
    I2C1_MDR_R = memAddr;               // Assign Starting address
    error = I2C_wait_till_done();       // Wait until write is complete
    if (error) return error;

    I2C1_MCS_R = 0x03;                  // S-(saddr+w)-ACK-maddr-ACK (Start/ Transmit)
    error = I2C_wait_till_done();       // Wait until write is complete
    if (error) return error;

    // Send data to write
    I2C1_MDR_R = data;                   // Assign data to send
    I2C1_MCS_R = 5;                      // -data-ACK-P (Transmit/ Stop)
    error = I2C_wait_till_done();        // Wait until write is complete
    while(I2C1_MCS_R & 0x40);            // Wait until bus is not busy
    error = I2C1_MCS_R & 0xE;            // Check error
    if (error) return error;
    return 0;
}


char I2C1_readBytes(int slaveAddr, char memAddr, int byteCount, char* data){
    // Local variables
    char error;

    // Check for incorrect length
    if (byteCount <= 0)
        return 1;

    // Send slave address and starting address
    I2C1_MSA_R = slaveAddr << 1;            // Assign Slave Address as writing mode
    I2C1_MDR_R = memAddr;                   // Send memory Address
    I2C1_MCS_R = 3;                         // S-(saddr+w)-ACK-maddr-ACK (Start/ Run)
    error = I2C_wait_till_done();           // Wait until write is complete
    if (error) return error;                // Check for error

    // Change bus to read mode
    I2C1_MSA_R = (slaveAddr << 1) + 1;      // Assign Slave Address as reading mode
                                            // Restart: -R-(saddr+r)-ACK */
    // If it's the last Byte NonACK
    if (byteCount == 1)
        I2C1_MCS_R = 7;                     // -data-NACK-P (Start/Transmit/Stop)
    else
        I2C1_MCS_R = 0x0B;                  // -data-ACK- (Ack/Start/Transmit)
    error = I2C_wait_till_done();           // Wait until write is complete
    if (error) return error;                // Check for error

    // Store the received data
    *data++ = I2C1_MDR_R;                   // Coming data from Slave

    // Single byte read
    if (--byteCount == 0){
        while(I2C1_MCS_R & 0x40);           // Wait until bus is not busy
        return 0;                           // No error
    }

    // Read the rest of the bytes
    while (byteCount > 1){
        I2C1_MCS_R = 9;                     // -data-ACK-
        error = I2C_wait_till_done();
        if (error) return error;
        byteCount--;
        *data++ = I2C1_MDR_R;               // store data received
    }

    I2C1_MCS_R = 5;                         // -data-NACK-P
    error = I2C_wait_till_done();
    *data = I2C1_MDR_R;                     // Store data received
    while(I2C1_MCS_R & 0x40);               // Wait until bus is not busy

    return 0;                               // No error
}

/* static int I2C_wait_till_done(void)
 * Returns:
 *  If there is no error, return 0.
 *
 * Description:
 * Wait until I2C master is not busy and return error code
 */
static int I2C_wait_till_done(void){
    while(I2C1_MCS_R & (1<<0) != 0); // Wait until I2C master is not busy
    return I2C1_MCS_R & 0xE;         // Check for errors
}

void Delay(unsigned long counter){
    unsigned long i = 0;
    for(i=0; i< counter*10000; i++);
}

void GPIO(void){
    SYSCTL_RCGCGPIO_R |= (1<<5); //Set bit5 to enable

    //PORTF0 has special function, need to unlock to modify
    GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;   //Unlock commit register
    GPIO_PORTF_CR_R = 0X1F;              //Make PORTF0 configurable
    //GPIO_PORTF_LOCK_R = 0;               //Lock commit register

    //Initialize PF3 as a digital output, PF0 and PF4 as digital input pins
    GPIO_PORTF_DIR_R  &= ~(1<<4)|~(1<<0);               //Set PF4 and PF0 as a digital input pins
    GPIO_PORTF_DIR_R |= (1<<3);                         // Set PF3 as digital output to control green LED
    GPIO_PORTF_DEN_R |= (1<<4)|(1<<3)|(1<<0);           // make PORTF4-0 digital pins
    GPIO_PORTF_PUR_R |= (1<<4)|(1<<0);                  // enable pull up for PORTF4, 0

    // configure PORTF4, 0 for falling edge trigger interrupt
    GPIO_PORTF_IS_R  &= ~(1<<4)|~(1<<0);        // make bit 4, 0 edge sensitive
    GPIO_PORTF_IBE_R &=~(1<<4)|~(1<<0);         // trigger is controlled by IEV
    GPIO_PORTF_IEV_R &= ~(1<<4)|~(1<<0);        // falling edge trigger
    GPIO_PORTF_ICR_R |= (1<<4)|(1<<0);          // clear any prior interrupt
    GPIO_PORTF_IM_R  |= (1<<4)|(1<<0);          // unmask interrupt

    // enable interrupt in NVIC and set priority to 5
    NVIC_PRI7_R = (NVIC_PRI7_R & 0xFF00FFFF)|0x00A00000;  //Priority 5
    NVIC_EN0_R |= (1<<30);  // enable IRQ30 (D30 of ISER[0])
}

void GPIOPortF_Handler(void){

    if(GPIO_PORTF_MIS_R & 0x10){        //check if interrupt causes by PF4/SW1
        GPIO_PORTF_DATA_R |= (1<<3);
        GPIO_PORTF_ICR_R |= 0x10;       //clear the interrupt flag
    }
    else if (GPIO_PORTF_MIS_R & 0x01){  //check if interrupt causes by PF0/SW2
        GPIO_PORTF_DATA_R &= ~0X08;
        GPIO_PORTF_ICR_R |= 0x01;       //clear the interrupt flag
    }
    else if (GPIO_PORTF_MIS_R & (1<<1)){ //check if interrupt causes by PF0/SW2
        //UART5_printString("A");
        /*
        GPIO_PORTF_ICR_R |= (1<<1);       //clear the interrupt flag
        // Read MPU6050 in a burst of data
        MPU6050_getData(MPURawData);

        // Convert The Readings
        AX = (float)MPURawData[0]/16384.0;
        AY = (float)MPURawData[1]/16384.0;
        AZ = (float)MPURawData[2]/16384.0;
        GX = (float)MPURawData[4]/131.0;
        GY = (float)MPURawData[5]/131.0;
        GZ = (float)MPURawData[6]/131.0;
        t = ((float)MPURawData[3]/340.00)+36.53;

        //Turn into string and send trough Serial5
        sprintf(m_sMsg,"%.2f,%.2f,%.2f\n",AX,AY,AZ);
        UART5_printString(m_sMsg);
        */
    }
}

void enable_PWM(void){
    // Clock setting for PWM and GPIO PORT
    SYSCTL_RCGCPWM_R |= (1<<1);         // Enable clock to PWM1 module
    SYSCTL_RCGCGPIO_R |= (1<<5);        // Enable system clock to PORTF
    SYSCTL_RCC_R |= (1<<20);            // Enable System Clock Divisor function
    SYSCTL_RCC_R |= (7<<17);            // Use pre-divider value of 64 and after that feed clock to PWM1 module

    // Setting of PF2 pin for M1PWM6 channel output pin
    GPIO_PORTF_AFSEL_R |= (1<<2);       // PF2 sets a alternate function
    GPIO_PORTF_PCTL_R &= ~0x00000F00;   // Set PF2 as output pin
    GPIO_PORTF_PCTL_R |= 0x00000500;    // Make PF2 PWM output pin
    GPIO_PORTF_DEN_R |= (1<<2);         // set PF2 as a digital pin

    PWM1_3_CTL_R

    PWM1->_3_CTL &= ~(1<<0);   /* Disable Generator 3 counter */
    PWM1->_3_CTL &= ~(1<<1);   /* select down count mode of counter 3*/
    PWM1->_3_GENA = 0x0000008C;  /* Set PWM output when counter reloaded and clear when matches PWMCMPA */
    PWM1->_3_LOAD = 5000;     /* set load value for 50Hz 16MHz/65 = 250kHz and (250KHz/5000) */
    PWM1->_3_CMPA = 4999;     /* set duty cyle to to minumum value*/
    PWM1->_3_CTL = 1;           /* Enable Generator 3 counter */
    PWM1->ENABLE = 0x40;      /* Enable PWM1 channel 6 output */

}
