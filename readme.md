# WaterSystem: Automated Water Pump Control

```
         _|=|__________
        /  /            \                                       [~~~~]
       /  /              \                                      [____]
      /__/________________\                                     //
       ||  || /--\ ||  ||                                      //
       ||[]|| | .| ||[]||                                     //
    () ||__||_|__|_||__||     ()         ___   	 __ [~~~~~~] //   ___
   ( )|-|-|-|====|-|-|-|||-|  ( ) 	(_(O)---^|| [______]//`--(O)_)
  ^^^^^^^^^^^====^^^^^^^^^^^^^^^^^^^YYYY^^^^^||^^^^^^^^^^^^^^YYYY^^^^^^/
                                             ||
                                             ||
                                             ||
                                            {~~}
                                            {__}
```

The WaterSystem is an intelligent and automated solution designed for the precise control of water pumps in a fluid management 
system. This project is specifically tailored for scenarios where multiple containers with varying water levels need to be 
efficiently managed to ensure a continuous and optimal water supply.

## Key Features

1. **Pump Control:** The system automates the operation of water pumps based on real-time water levels in different containers. 
Pumps are activated or deactivated as needed to maintain optimal water levels at [specific periods](#specific-periods).

2. **Container Monitoring:** Water levels in multiple containers are constantly monitored using sensors. This information is crucial 
for making informed decisions about when and how long to activate the pumps.

3. **User-Defined Settings:** Users can customize the system's behavior by setting thresholds for water levels and specifying pump 
activation and deactivation criteria. This flexibility allows the system to adapt to different environments and requirements.

4. **Data Logging:** The system logs water level data over time, providing users with historical insights into water usage patterns. 
This data can be valuable for analyzing trends and optimizing the overall efficiency of water distribution.

5. **User Interface:** A user-friendly interface, potentially implemented using an LCD display or other visualization tools, allows 
users to interact with and monitor the system. Users can view current water levels, system status, and configure settings.

6. **Fault Detection:** The system incorporates fault detection mechanisms to identify and alert users about any anomalies or 
malfunctions, ensuring a reliable and resilient operation.

<a name="specific-periods"></a> <ins>Specific Periods</ins>

The system utilizes a Real-Time Clock (RTC) module to intelligently schedule pump operations during specific periods of the day, 
days of the week, and even across different seasons. By integrating time-based control, the WaterSystem ensures efficient water 
management aligned with user-defined schedules. This feature enhances the system's adaptability to varying water needs and 
contributes to overall resource optimization.


## Potential Components

* Arduino board (e.g., Arduino Mega2560) - I'm using Mega as primary device and AtMega8 as Slave device.
* Liquid Crystal Display (LCD) - For diferent modes and level reads.
* Push buttons for user input
* LEDs for status indication 
* Buzzer for sound output

* Two water containers with ultrasonic sensors for water level monitoring.
* Arduino board controlling the system, including pumps and sensor readings.
* Pump 1 for drawing water from the well.
* Pump 2 for raising water to a higher place.
* An optional Atmega8 microcontroller for data processing in the raised tank.


## License
This project is licensed under the MIT License - see the LICENSE file for details.

