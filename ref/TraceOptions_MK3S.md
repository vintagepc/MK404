## Avaliable telemetry streams:
- [By name](#by-name)
- [By category](#by-category)
### By Name:
Name | Categories
-----|-----------
Beeper_<digital.in| `OutputPin` `Misc`
E_8<tmc2130.byte_in| `SPI` `Stepper`
E_8>tmc2130.byte_out| `SPI` `Stepper`
E_<tmc2130.cs_in| `SPI` `Stepper` `OutputPin`
E_<tmc2130.dir_in| `OutputPin` `Stepper`
E_<tmc2130.en_in| `OutputPin` `Stepper`
E_<tmc2130.step_in| `OutputPin` `Stepper`
E_>tmc2130.diag_out| `InputPin` `Stepper`
Encoder_>encoder.a| `InputPin` `Display`
Encoder_>encoder.b| `InputPin` `Display`
Encoder_>encoder.button| `InputPin` `Display`
Fan1_<Fan.digital_in>| `Fan` `OutputPin`
Fan1_<Fan.pwm_in| `PWM` `Fan`
Fan1_>Fan.speed_out| `Fan` `Misc`
Fan1_>Fan.tach_out| `Fan` `InputPin`
Fan_<Fan.digital_in>| `Fan` `OutputPin`
Fan_<Fan.pwm_in| `PWM` `Fan`
Fan_>Fan.speed_out| `Fan` `Misc`
Fan_>Fan.tach_out| `Fan` `InputPin`
Heater_B_<heater.digital_in| `Heater`
Heater_B_<heater.pwm_in| `Heater` `PWM`
Heater_B_>heater.on| `Heater` `Misc`
Heater_H_<heater.digital_in| `Heater`
Heater_H_<heater.pwm_in| `Heater` `PWM`
Heater_H_>heater.on| `Heater` `Misc`
LCD_8<hd44780.BRIGHTNESS_PWM_IN| `Display` `PWM`
LCD_8>hd44780.DATA_IN| `Display`
LCD_<hd44780.E| `Display` `OutputPin`
LCD_<hd44780.RS| `Display` `OutputPin`
LCD_<hd44780.RW| `Display` `OutputPin`
LCD_=hd44780.BRIGHTNESS_IN| `Display` `OutputPin`
PINDA_>pinda.out| `InputPin` `Misc`
PINDA_>sheet.out| `InputPin` `Misc`
Power Panic_Power Panic| `InputPin` `Misc`
SDCard_8<SD.byte_in| `SPI` `Storage`
SDCard_8>SD.byte_out| `SPI` `Storage`
SDCard_<SD.cs_in| `SPI` `Storage` `OutputPin`
SDCard_>SD.card_present| `InputPin` `Storage`
SPIFlash_1<w25x20cl.cs_in| `SPI` `Storage` `OutputPin`
SPIFlash_8<w25x20cl.byte_in| `SPI` `Storage`
SPIFlash_8>w25x20cl.byte_out| `SPI` `Storage`
Thermistor1_>adc.out| `ADC` `Thermistor`
Thermistor1_>temp.out| `Thermistor` `Misc`
Thermistor2_>adc.out| `ADC` `Thermistor`
Thermistor2_>temp.out| `Thermistor` `Misc`
Thermistor3_>adc.out| `ADC` `Thermistor`
Thermistor3_>temp.out| `Thermistor` `Misc`
Thermistor_>adc.out| `ADC` `Thermistor`
Thermistor_>temp.out| `Thermistor` `Misc`
UART0_8<avr.uart0.in| `Serial`
UART0_8>avr.uart0.out| `Serial`
UART0_>avr.uart0.xoff| `Serial`
UART0_>avr.uart0.xon| `Serial`
UART2_8<avr.uart2.in| `Serial`
UART2_8>avr.uart2.out| `Serial`
UART2_>avr.uart2.xoff| `Serial`
UART2_>avr.uart2.xon| `Serial`
VSrc4_16>voltage.value_out| `ADC` `Power`
VSrc4_>voltage.digital_out| `Power` `InputPin`
VSrc8_16>voltage.value_out| `ADC` `Power`
VSrc8_>voltage.digital_out| `Power` `InputPin`
VSrc9_16>voltage.value_out| `ADC` `Power`
VSrc9_>voltage.digital_out| `Power` `InputPin`
X_8<tmc2130.byte_in| `SPI` `Stepper`
X_8>tmc2130.byte_out| `SPI` `Stepper`
X_<tmc2130.cs_in| `SPI` `Stepper` `OutputPin`
X_<tmc2130.dir_in| `OutputPin` `Stepper`
X_<tmc2130.en_in| `OutputPin` `Stepper`
X_<tmc2130.step_in| `OutputPin` `Stepper`
X_>tmc2130.diag_out| `InputPin` `Stepper`
Y_8<tmc2130.byte_in| `SPI` `Stepper`
Y_8>tmc2130.byte_out| `SPI` `Stepper`
Y_<tmc2130.cs_in| `SPI` `Stepper` `OutputPin`
Y_<tmc2130.dir_in| `OutputPin` `Stepper`
Y_<tmc2130.en_in| `OutputPin` `Stepper`
Y_<tmc2130.step_in| `OutputPin` `Stepper`
Y_>tmc2130.diag_out| `InputPin` `Stepper`
Z_8<tmc2130.byte_in| `SPI` `Stepper`
Z_8>tmc2130.byte_out| `SPI` `Stepper`
Z_<tmc2130.cs_in| `SPI` `Stepper` `OutputPin`
Z_<tmc2130.dir_in| `OutputPin` `Stepper`
Z_<tmc2130.en_in| `OutputPin` `Stepper`
Z_<tmc2130.step_in| `OutputPin` `Stepper`
Z_>tmc2130.diag_out| `InputPin` `Stepper`
### By category
#### Display
 - LCD_<hd44780.E
 - LCD_<hd44780.RS
 - LCD_<hd44780.RW
 - LCD_8>hd44780.DATA_IN
 - LCD_=hd44780.BRIGHTNESS_IN
 - LCD_8<hd44780.BRIGHTNESS_PWM_IN
 - Encoder_>encoder.button
 - Encoder_>encoder.a
 - Encoder_>encoder.b
#### Fan
 - Fan_<Fan.pwm_in
 - Fan_<Fan.digital_in>
 - Fan_>Fan.tach_out
 - Fan_>Fan.speed_out
 - Fan1_<Fan.pwm_in
 - Fan1_<Fan.digital_in>
 - Fan1_>Fan.tach_out
 - Fan1_>Fan.speed_out
#### Heater
 - Heater_B_<heater.pwm_in
 - Heater_B_<heater.digital_in
 - Heater_B_>heater.on
 - Heater_H_<heater.pwm_in
 - Heater_H_<heater.digital_in
 - Heater_H_>heater.on
#### InputPin
 - SDCard_>SD.card_present
 - Fan_>Fan.tach_out
 - Fan1_>Fan.tach_out
 - Encoder_>encoder.button
 - Encoder_>encoder.a
 - Encoder_>encoder.b
 - X_>tmc2130.diag_out
 - Z_>tmc2130.diag_out
 - Y_>tmc2130.diag_out
 - E_>tmc2130.diag_out
 - PINDA_>pinda.out
 - PINDA_>sheet.out
 - VSrc9_>voltage.digital_out
 - VSrc4_>voltage.digital_out
 - Power Panic_Power Panic
 - VSrc8_>voltage.digital_out
#### OutputPin
 - SDCard_<SD.cs_in
 - Fan_<Fan.digital_in>
 - Fan1_<Fan.digital_in>
 - Beeper_<digital.in
 - LCD_<hd44780.E
 - LCD_<hd44780.RS
 - LCD_<hd44780.RW
 - LCD_=hd44780.BRIGHTNESS_IN
 - X_<tmc2130.cs_in
 - X_<tmc2130.step_in
 - X_<tmc2130.dir_in
 - X_<tmc2130.en_in
 - Z_<tmc2130.cs_in
 - Z_<tmc2130.step_in
 - Z_<tmc2130.dir_in
 - Z_<tmc2130.en_in
 - Y_<tmc2130.cs_in
 - Y_<tmc2130.step_in
 - Y_<tmc2130.dir_in
 - Y_<tmc2130.en_in
 - E_<tmc2130.cs_in
 - E_<tmc2130.step_in
 - E_<tmc2130.dir_in
 - E_<tmc2130.en_in
 - SPIFlash_1<w25x20cl.cs_in
#### Power
 - VSrc9_16>voltage.value_out
 - VSrc9_>voltage.digital_out
 - VSrc4_16>voltage.value_out
 - VSrc4_>voltage.digital_out
 - VSrc8_16>voltage.value_out
 - VSrc8_>voltage.digital_out
#### Stepper
 - X_8<tmc2130.byte_in
 - X_8>tmc2130.byte_out
 - X_<tmc2130.cs_in
 - X_<tmc2130.step_in
 - X_<tmc2130.dir_in
 - X_<tmc2130.en_in
 - X_>tmc2130.diag_out
 - Z_8<tmc2130.byte_in
 - Z_8>tmc2130.byte_out
 - Z_<tmc2130.cs_in
 - Z_<tmc2130.step_in
 - Z_<tmc2130.dir_in
 - Z_<tmc2130.en_in
 - Z_>tmc2130.diag_out
 - Y_8<tmc2130.byte_in
 - Y_8>tmc2130.byte_out
 - Y_<tmc2130.cs_in
 - Y_<tmc2130.step_in
 - Y_<tmc2130.dir_in
 - Y_<tmc2130.en_in
 - Y_>tmc2130.diag_out
 - E_8<tmc2130.byte_in
 - E_8>tmc2130.byte_out
 - E_<tmc2130.cs_in
 - E_<tmc2130.step_in
 - E_<tmc2130.dir_in
 - E_<tmc2130.en_in
 - E_>tmc2130.diag_out
#### Storage
 - SDCard_8<SD.byte_in
 - SDCard_8>SD.byte_out
 - SDCard_<SD.cs_in
 - SDCard_>SD.card_present
 - SPIFlash_8<w25x20cl.byte_in
 - SPIFlash_8>w25x20cl.byte_out
 - SPIFlash_1<w25x20cl.cs_in
#### Thermistor
 - Thermistor_>adc.out
 - Thermistor_>temp.out
 - Thermistor1_>adc.out
 - Thermistor1_>temp.out
 - Thermistor2_>adc.out
 - Thermistor2_>temp.out
 - Thermistor3_>adc.out
 - Thermistor3_>temp.out
#### Serial
 - UART0_8>avr.uart0.out
 - UART0_8<avr.uart0.in
 - UART0_>avr.uart0.xon
 - UART0_>avr.uart0.xoff
 - UART2_8>avr.uart2.out
 - UART2_8<avr.uart2.in
 - UART2_>avr.uart2.xon
 - UART2_>avr.uart2.xoff
#### SPI
 - SDCard_8<SD.byte_in
 - SDCard_8>SD.byte_out
 - SDCard_<SD.cs_in
 - X_8<tmc2130.byte_in
 - X_8>tmc2130.byte_out
 - X_<tmc2130.cs_in
 - Z_8<tmc2130.byte_in
 - Z_8>tmc2130.byte_out
 - Z_<tmc2130.cs_in
 - Y_8<tmc2130.byte_in
 - Y_8>tmc2130.byte_out
 - Y_<tmc2130.cs_in
 - E_8<tmc2130.byte_in
 - E_8>tmc2130.byte_out
 - E_<tmc2130.cs_in
 - SPIFlash_8<w25x20cl.byte_in
 - SPIFlash_8>w25x20cl.byte_out
 - SPIFlash_1<w25x20cl.cs_in
#### ADC
 - Thermistor_>adc.out
 - Thermistor1_>adc.out
 - Thermistor2_>adc.out
 - Thermistor3_>adc.out
 - VSrc9_16>voltage.value_out
 - VSrc4_16>voltage.value_out
 - VSrc8_16>voltage.value_out
#### PWM
 - Fan_<Fan.pwm_in
 - Fan1_<Fan.pwm_in
 - Heater_B_<heater.pwm_in
 - Heater_H_<heater.pwm_in
 - LCD_8<hd44780.BRIGHTNESS_PWM_IN
#### Misc
 - Thermistor_>temp.out
 - Thermistor1_>temp.out
 - Thermistor2_>temp.out
 - Thermistor3_>temp.out
 - Fan_>Fan.speed_out
 - Fan1_>Fan.speed_out
 - Heater_B_>heater.on
 - Heater_H_>heater.on
 - Beeper_<digital.in
 - PINDA_>pinda.out
 - PINDA_>sheet.out
 - Power Panic_Power Panic
