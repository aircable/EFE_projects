| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

July 5th 2023

This project combines three functionalities

1. Reading temperature sensor
2. Semaphores (mutex)
3. Read and write a FAT file

There are three RTOS tasks. The first task (T1) reads the temperature sensor, it tries to get access to the semaphore S1 before acquiring the temperature value from the sensor. The second task (T2) locks the semaphore S2 when it starts and then within a loop, the task tries to get access to the semaphore S1 and reads the temperature value set by T1, in every loop the task stores the temperature value in a file kept in a FAT file system. When T2 ends the loop reading the temperature, it unlocks the semaphore S2. The third task (T3) tries to get access to S2, and the task will remain locked until T2 completes. When T3 resumes its execution, it reads the containts of the file written by T2 and prints it in the console.