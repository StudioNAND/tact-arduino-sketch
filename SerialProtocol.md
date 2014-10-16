# Tact Serial Protocol

The Tact libraries make use of a dedicated serial communication protocol to make sensor and desktop application (processing sketch) talk to each other.


## Initial Handshake
They like to keep it formal. Serial communication starts with an intial handshake. 

Processing:
    
    V\n
    
Arduino response:

	2125

Tact does not just say hello, but also sends along its library version. It's `2125 - 2124 = 1` or Tact Version 1.



## Requesting data
Each command sent from Processing consists of one uppercase command key, followed by four integers for configuring the sensor, followed by a newline char.   
Tact responds with integers. Their meaning is encoded in the respective value range.

Processing example:

    P 0 48 32 1\n
    
for humans: 

     P:  Read peak
     0:  for sensor 0, 
    48:  starting at position 48 of spectrum,
    32:  read 32 measurements
     1:  with a step-size of 1.
    \n:  command finish
       
Arduino response:
	
	1025\n
	1089\n
	1099\n
	451\n
	2123\n
	
Stop babbeling, Arduino! What does this even mean?

	1025 - 1024 = Sensor ID 1
	1089 - 1088 = Data Type 1 = Peak
	1099 - 1098 = 1 value to be transmitted
	451         = value 451
	2123        = end of transmission	


### Arduino implentation
| What is being sent                   | range            | implementation         |
| :----------------------------------- | :--------------- | :--------------------- |
| Library Version (initial handshake)  | `2124 - 2199`    | `2124 + VERSION`       |
| Sensor ID                            | `1024 - 1087`    | `1024 + SENSOR_ID`     |
| Data Type                            | `1088 - 1097`    | `1088 + DATA_TYPE`     |
| Num of values to be transmitted      | `1098 - 2122`    | `1098 + DATA_COUNT`    |
| Data                                 | `0    - 1023`    | `   0 + DATA`          |
| End transmission                     | `2123`           | `2123`                 |


### Data Types
| Data Type | Processing Key | Arduino Data Type |
| :-------- | :---------     | :---------------- |
| Spectrum  | S              | 0                 |
| Peak      | P              | 1                 |
| Bias      | B              | 2                 |

