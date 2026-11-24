#define F_CPU 8000000UL // 8 MHz clock frequency
#include <avr/io.h>
#include <util/delay.h>

// Temperature threshold in Celsius
#define FIRE_THRESHOLD 50 

// Bitwise Pin Definitions
#define SENSOR_PIN PA0
#define ALARM_PIN  PB0
#define MOTOR_PIN  PB1
#define SAFE_PIN   PB2

// Initialize ADC on Port A using bitwise operators
void ADC_Init(void) {
    // 1. Set PA0 as input using bitwise AND with inverted bitmask
    DDRA &= ~(1 << SENSOR_PIN);

    // 2. Select AVcc as reference voltage (REFS0 = 1, REFS1 = 0)
    ADMUX |= (1 << REFS0);
    ADMUX &= ~(1 << REFS1);

    // 3. Enable ADC (ADEN = 1) and set Prescaler to 128 (ADPS2, ADPS1, ADPS0 = 1)
    // 8MHz / 128 = 62.5kHz (Ideal ADC clock range: 50kHz - 200kHz)
    ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

// Read 10-bit analog value from a specific Port A channel
uint16_t ADC_Read(uint8_t channel) {
    // Clear lower 5 bits (MUX4:0) using bitwise AND, then set target channel using bitwise OR
    ADMUX = (ADMUX & 0xE0) | (channel & 0x07);

    // Start single conversion by setting ADSC bit to 1
    ADCSRA |= (1 << ADSC);

    // Poll ADSC bit using bitwise AND until hardware clears it to 0 (conversion done)
    while (ADCSRA & (1 << ADSC));

    return ADC;
}

int main(void) {
    uint16_t adc_result;
    float temperature;

    // Set PB0, PB1, and PB2 as OUTPUT pins using bitwise OR
    DDRB |= (1 << ALARM_PIN) | (1 << MOTOR_PIN) | (1 << SAFE_PIN);

    // Ensure all outputs start in the OFF state using bitwise AND with NOT
    PORTB &= ~((1 << ALARM_PIN) | (1 << MOTOR_PIN) | (1 << SAFE_PIN));

    // Initialize ADC
    ADC_Init();

    while (1) {
        // 1. Automatically read sensor from Port A (PA0)
        adc_result = ADC_Read(SENSOR_PIN);

        // 2. Convert 10-bit ADC value to Celsius (LM35 = 10mV/°C, Vref = 5V)
        temperature = (adc_result * 500.0) / 1024.0;

        // 3. Automatic Switching & Extinguishing Logic
        if (temperature >= FIRE_THRESHOLD) {
            // FIRE DETECTED:
            // Turn ON Alarm (PB0) and Motor (PB1) using bitwise OR
            PORTB |= (1 << ALARM_PIN) | (1 << MOTOR_PIN);
            
            // Turn OFF Safe LED (PB2) using bitwise AND + NOT
            PORTB &= ~(1 << SAFE_PIN);
        } 
        else {
            // FIRE EXTINGUISHED / SAFE STATE:
            // Automatically turn OFF Alarm (PB0) and Motor (PB1) using bitwise AND + NOT
            PORTB &= ~((1 << ALARM_PIN) | (1 << MOTOR_PIN));
            
            // Turn ON Safe LED (PB2) using bitwise OR
            PORTB |= (1 << SAFE_PIN);
        }

        _delay_ms(500);
    }

    return 0;
}