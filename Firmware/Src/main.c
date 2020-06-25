/* 
 * File:   main.c
 * Author: pschilk
 *
 * Created on June 24, 2020, 8:34 AM
 */

// CONFIG1
#pragma config FOSC = INTOSC    // (INTOSC oscillator; I/O function on CLKIN pin)
#pragma config WDTE = ON        // Watchdog Timer Enable (WDT enabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = OFF      // PLL Enable (4x PLL disabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LPBOREN = OFF    // Low Power Brown-out Reset enable bit (LPBOR is disabled)
#pragma config LVP = ON         // Low-Voltage Programming Enable (Low-voltage programming enabled)

#include <pic.h>
#include <stdint.h>

#define DEBOUNCE_TIME  10 // Time inputs are ignored after input in 10s of ms
#define PUMP_DIR_THR   33
#define PUMP_SCALE     6
#define PUMP_PERM_THR  0x3F0

// Pump modes
typedef enum{
    direct,   // Pump on if solenoid on
    timeout,  // Pump runs when solenoid off until timed-out
    permanent // Pump always runs
} PUMP_MODE_T;

uint32_t debounce_count;
uint8_t debounce_prevstate;
uint32_t pump_timeout_coumt; 
uint32_t pump_timeout;
PUMP_MODE_T pump_mode;
uint8_t solenoid_on;

int main(int argc, char** argv) {
    CLRWDT();
    
    // ===== WDT & CLK =====
    // Using internal MF OSC at 500khz
    OSCCONbits.SPLLEN = 0;
    OSCCONbits.IRCF = 0b0111;
    OSCCONbits.SCS = 0b10;
    
    // Have WDT reset every 512ms
    WDTCONbits.WDTPS = 0b01001;
    
    // ===== Config GPIO =====
    TRISAbits.TRISA0 = 1; // Pedal Input
    TRISAbits.TRISA2 = 0; // Valve Ouput
    TRISAbits.TRISA4 = 1; // Poti Input
    TRISAbits.TRISA5 = 0; // Motor Output
    
    // Clear all outputs
    LATA = 0x0; 
    
    // Set RA4 as only analog input
    ANSELA = 0x0;
    ANSELAbits.ANSA4 = 1;
    
    // Disable all pullups, all pins push-pull,
    // no slewrate limiting
    WPUA = 0x0;
    ODCONA = 0x0;
    SLRCONA = 0x0;
    
    // ===== Config ADC ======
    ADCON0bits.CHS = 0b00011; // Select CH3 (RA4)
    
    ADCON1bits.ADFM = 1; // Right-justified result
    ADCON1bits.ADCS = 0b000; // Conversion Clk Fosc/2
    ADCON1bits.ADPREF = 0b00; // VDD is positive voltage refernce
    
    ADCON2bits.TRIGSEL = 0; // No Automatic triggering
    
    ADCON0bits.ADON = 1; // Turn on ADC
    
    ADCON0bits.GO = 1; // Start conversion
    
    // ===== Setup Variables =====
    debounce_count = 0;
    debounce_prevstate = PORTAbits.RA0;
    pump_timeout_coumt = 0;
    solenoid_on = 0;
    
    // ===== Config Regular interrupt =====
    // Setup Timer 0 to overflow at aprox. 120Hz and trigger interrupt
    OPTION_REGbits.TMR0CS = 0; // Timer 0 clocked from Fosc/4
    OPTION_REGbits.PSA = 0;    // Timer 0 has prescalar
    OPTION_REGbits.PS = 0b01;  // Timer 0 prescalar is /4
    
    INTCONbits.GIE = 1;    // Interupts enabled.
    INTCONbits.TMR0IE = 1; // Timer 0 interrupt enabled
    
    
    while(1){
        NOP();
    }
    return (EXIT_SUCCESS);
}

void __interrupt() ISR(void){
    CLRWDT(); // pet the watchdog..
    
    // Handle ADC
    if(!ADCON0bits.GO_nDONE){
        // ADC Conversion finished
        uint16_t result = (ADRESH << 8) | ADRESL;
        
        if(result >= PUMP_PERM_THR){
            pump_mode = permanent;  
            pump_timeout = 0;
        } else if(result <= PUMP_DIR_THR){
            pump_mode = direct;
            pump_timeout = 0;
        } else {
            pump_mode = timeout;
            pump_timeout = result * PUMP_SCALE;
        }
        
        // Start next conversion
        ADCON0bits.GO_nDONE = 1;
    }
    
	// Handle De-bounce/Solenoid
	if(debounce_count == 0){ // Ready for next event
        // Determine if input changed and we should time out again
        if(debounce_prevstate != PORTAbits.RA0){
            debounce_count = DEBOUNCE_TIME;
        }
        debounce_prevstate = PORTAbits.RA0;
        
        // If the pedal is pressed, open the solenoid 
        // and keep reseting the pump timeout
        if(PORTAbits.RA0){
            LATAbits.LATA2 = 1; // Open solenoid
            pump_timeout_coumt = pump_timeout;
            solenoid_on = 1;
        } else {
            LATAbits.LATA2 = 0; // Close solenoid
            solenoid_on = 0;
        }
        
    } else { // Still timed out
        debounce_count--;
    }
        
    // Turn on/off pump
    if(solenoid_on){
        LATAbits.LATA5 = 1; // start pump
    } else if(pump_mode == permanent){
        LATAbits.LATA5 = 1; // start pump
    } else if (pump_mode == timeout && pump_timeout_coumt != 0){
        LATAbits.LATA5 = 1; // start pump
    } else {
        LATAbits.LATA5 = 0; // stop pump
    }
    
    // Handle pump timeout
    // If potentiometer was decreased during timeout, shorten. 
    if(pump_timeout_coumt > pump_timeout){
        pump_timeout_coumt = pump_timeout;
    }
    if(pump_timeout_coumt != 0){
        pump_timeout_coumt--;
    }
    
    
    // Clear flags and reset GIE
    INTCONbits.TMR0IF = 0;
    INTCONbits.GIE = 1;
}
