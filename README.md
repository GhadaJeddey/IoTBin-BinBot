# IoTBin-BinBot

# Hardware used for this project 
- Esp32 SoC
- 2 Ultrasonic Sensors
- 1 Servo Motor
- F/M and M/M Cables

# Features and Functionalities
The BinBot is equipped with multiple features that simplify and optimize waste management. It provides the ability to check whether the bin is full or not while enabling hygienic disposal of waste through automated lid opening and closing. These features, detailed below, showcase how the BinBot combines convenience, modernity, and advanced technology to address waste management needs in various environments.
– Presence Detection: An external ultrasonic sensor detects the presence of an object within 5 cm.
– Automatic Opening and Closing: If the bin is not full and an object is detected, the lid opens to allow waste disposal and automatically closes after 5 seconds.
– Fill Level Monitoring: An internal ultrasonic sensor measures the fill level and publishes the data on the Blynk application.
– Opening Restriction When Full: If the bin is full, the lid does not open unless manually commanded via Alexa or the Blynk application.
– MQTT Publishing: The bin’s fill level and status are published on the smartbin/status topic.
– Voice Control via Alexa: Allows users to check the bin’s status and open the lid using the ”Open” intent.


https://github.com/user-attachments/assets/b3748adf-4de3-4eaa-9f17-cc03bca907fb

