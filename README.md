# Mini Thermometer Project - README
***
![](https://csharpcorner-mindcrackerinc.netdna-ssl.com/article/how-to-create-digital-room-thermometer-using-arduino-uno-r3/Images/01.jpg)  

### **Instructions on how to compile/execute program(s):**
*`project.c`, `lcd.h`, `lcd.c` , `encoder.h`, `encoder.c`, `ds18b20.h`, `ds18b20.c` , `adc.h` , `adc.c` should all be in the same directory*  

1. Set up Arduino Uno  
    a. Connect digital temperature sensor (DS18B20) to PC3  
    b. Connect LEDs (red LED to PD2 and green LED to PD3)   
    c. Connect buzzer to PC5    
    d. Connect buttons to PC1 and PC2   
    e. Connect rotary encoder to PB3 and PB4    
2. CD to **project_files**  
3. Run **project.c**    

*Green light indicates cold temperature (below DS18B20 sensor temperature)*  
*Red light indicates hot temperature (higher than DS18B20 sensor temperature)*  
*Button on PC1 indicates rotary encoder controls threshold temperature* 
*Button on PC2 indicates rotary encoder controls notes for buzzer*  
*The buzzer only rings (plays the respective note) when temperature is 2 degrees over the threshold*    