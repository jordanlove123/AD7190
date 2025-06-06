#include "AD7190.h"

AD7190::AD7190(double Vref, bool cont) {
    vref = Vref;
    cont_read = cont;
    SPI.begin()
    pinMode(COPI, OUTPUT);
    pinMode(CIPO, INPUT);
    pinMode(SCLK, OUTPUT);
    pinMode(CS, OUTPUT);
}

// Reset circuitry and serial interface by writing 40 consecutive 1s. Done automatically on startup
// All registers are restored to original values
void AD7190::reset() {
    digitalWrite(CS, LOW);
    for (int i = 0; i < 5; i++) {
        spi_transfer(0xFF);
    }
    digitalWrite(CS, HIGH);
}

// Either read from or write to a register. cont means continuous read mode (see datasheet p.19)
void AD7190::set_next_op(byte read, byte reg, byte cont) {
    byte writeval = (read << 6) | (reg << 3) | (cont << 2);

    digitalWrite(CS, LOW);
    spi_transfer(writeval);
    digitalWrite(CS, HIGH);
}

// Either read from or write to a register.
void AD7190::set_next_op(byte read, byte reg) {
    set_next_op(read, reg, 0);
}

// Read out the contents of register reg
unsigned long AD7190::read_register(byte reg, int nbytes, bool cont) {
    unsigned long ret = 0;
    if (!cont) {
        set_next_op(READ, reg);
    }

    digitalWrite(CS, LOW);
    for (int i = 0; i < nbytes; i++) {
        ret |= spi_transfer(0x0) << (8*i);
    }
    digitalWrite(CS, HIGH);

    return ret;
}

// Read out the contents of register reg
unsigned long AD7190::read_register(byte reg, int nbytes) {
    return read_register(reg, nbytes, false);
}

// Check if ~RDY bit is cleared in the status register
bool AD7190::data_ready() {
    return (read_register(COMMSTAT, 1) & 0x8) == 0;
}

// Check if ERR bit is set in the status register. Can be used for debugging
bool AD7190::is_error() {
    return (read_register(COMMSTAT, 1) & 0x6) == 0;
}

// Read out 24 bits from the data register
double AD7190::read_data() {
    while (!data_ready()) {
    }

    unsigned long reg = read_register(DATA, 3, cont_read);

    if (unipolar) {
        double data = ((double) reg)/0x800000 * vref;
    }
    else {
        double data = ((double) reg)/0x800000 * 2*vref - vref;
    }

    return data;
}

// Set mode register values (see datasheet p.21)
void AD7190::set_mode(byte mode, byte clock) {
    unsigned long regval = (mode << 21) | (clock << 18);

    set_next_op(WRITE, MODE);

    digitalWrite(CS, LOW);
    for (int i = 0; i < 3; i++) {
        spi_transfer(regval >> (2-i) | 0xFF);
    }
    digitalWrite(CS, HIGH);
}

// Set config register values (see datasheet p.23). Note: multiple channels can be selected by setting each relevant
// bit instead of just one. Ex: channels = AIN1_2 | AIN3_4. Doing this means each channel will be loaded into the
// data register sequentially.
void AD7190::set_config(byte refsel, byte channels, byte polarity, byte gain) {
    unsigned long regval = (refsel << 20) | (channels << 8) | (polarity << 3) | gain;

    set_next_op(WRITE, MODE);

    digitalWrite(CS, LOW);
    for (int i = 0; i < 3; i++) {
        spi_transfer(regval >> (2-i) | 0xFF);
    }
    digitalWrite(CS, HIGH);
}

void AD7190::init(byte mode, byte clock, byte refsel, byte channels, byte polarity, byte gain) {
    unipolar = polarity;
    set_mode(mode, clock);
    set_config(refsel, channels, polarity, gain);
    if (cont_read) {
        digitalWrite(CS, LOW);
        spi_transfer(0x5C);       // Set up continuous reading
        digitalWrite(CS, HIGH);
    }
}

void AD7190::init(byte channels) {
    init(CONT, INTCLK2, REFIN1, channels, BIPOLAR, GAIN_1);
}