/*
Main for Testing and Changing PID Control during Runtime
*/

/*==================================================================*/
  //Extern libraries
  #include "Arduino.h"
  #include <Timer.h>
  #include "../lib/sensors/sensors.h"
  #include "../lib/control/RotorControl.h"
  #include "../lib/params.h"
  #include "../lib/ble_com/ble_com.h"
  #include "SoftwareSerial.h"

/*==================================================================*/
  //Declare needed objects
  Timer t;
  SENSORS sensors;
  ROTORCONTROL rotors;
  BLE_COM ble;
  SoftwareSerial skm53Serial(RX_PIN, TX_PIN); //RX 4 geht zu TX im GPS Modul; TX 3 geht zu RX im GPS Modul

  //Declare necessary variables
  float t1, t2, t3, t4;

/*==================================================================*/
  //Functions
void timerUpdate(){
  t1 = micros();
  sensors.update();
  t2 = micros();
  rotors.update(sensors.imu.rot, sensors.imu.rot_vel);
  t3 = micros();

  Serial.print("SensorsUpdate: ");
  Serial.print(t2-t1);
  Serial.print(" RotorControlUpdate: ");
  Serial.print(t3-t2);
  Serial.print(" TotalFastUpdate: ");
  Serial.println(t3-t1);
}

void slowTimerUpdate(){
  t1 = micros();
  sensors.updateSlow();
  t2 = micros();
  rotors.updateSlow(sensors.imu.rot, sensors.imu.rot_vel, sensors.usr.height);
  t3 = micros();
  #if BLE_TELEMETRICS_ON
    ble.update();
  #endif
  t4 = micros();

  Serial.print("SensorsUpdate: ");
  Serial.print(t2-t1);
  Serial.print(" RotorControlUpdate: ");
  Serial.print(t3-t2);
  Serial.print(" BleUpdate: ");
  Serial.print(t4-t3);
  Serial.print(" TotalSlowUpdate: ");
  Serial.println(t4-t1);
}

void setup(){
  //Start Serial and wait for connection
  Serial.begin(38400);
  if(FORCE_SERIAL) while(!Serial);

  //start and calibrate sensors
  sensors.begin(&skm53Serial);

  //set Rotors/ESCs to PINs and initialize
  rotors.begin();
  rotors.start(BEFORE_TAKE_OFF_SIGNAL);

  //Serial Communication
  Serial.println("What would you like to update?");
  Serial.println("controller: 'c'");
  Serial.println("height: 'h'");
  Serial.println("angle: 'a'");

  //Activate untethered communication
  #if BLE_TELEMETRICS_ON
    ble.begin(rotors);
  #endif

  //Set timer event, that calls timerUpdate every SAMPLE_RATE milliseconds
  t.every(SAMPLE_RATE,timerUpdate);
  t.every(SLOW_SAMPLE_RATE,slowTimerUpdate);
}

void loop(){
  t.update();
}

void serialEvent(){
  if (Serial.available() > 0){
    char firstInput = (char)Serial.read();
    //while(Serial.available()) Serial.read();
    switch (firstInput) {

      case 99: // compares firstInput to 'c'
        rotors.stop();
        rotors.positionController.setConstantsViaSerial();
        rotors.start(BEFORE_TAKE_OFF_SIGNAL);
        break;

      case 67: // compares firstInput to 'C'
          rotors.stop();
          rotors.heightController.setConstantsViaSerial();
          rotors.start(BEFORE_TAKE_OFF_SIGNAL);
          break;

      case 104: //compares firstInput to 'h'
        rotors.stop();
        rotors.heightController.setTargetHeight();
        rotors.start(BEFORE_TAKE_OFF_SIGNAL);
        break;

      case 97: //compares firstInput to 'a'
        if (rotors.positionController.targetPosition[0] == 0){
          rotors.positionController.targetPosition[0] = 20;
          Serial.println("angle of 20 degree has been set!");
        }
        else {
          rotors.positionController.targetPosition[0] = 0;
          Serial.println("angle of 0 degree has been set!");
        }
        /*Serial.println("Enter a 2-digit number! Maximum angle is 45 degree. ");
        Serial.println("What angle would you like to see? ");
        while (Serial.available()<2) {} //wating for Serial to have two digits
        int angleInput;
        angleInput = Serial.parseInt();
        if(angleInput <= 40){
          targetPosition[0] = angleInput;
        }*/
        break;

      default:
        Serial.println("Your first Input could not be recognized. Try again");
        //while(Serial.available()) Serial.read();
        break;
    }

    Serial.println("What would you like to update?");
    Serial.println("controller: 'c'");
    Serial.println("height: 'h'");
    Serial.println("angle: 'a'");
  }
}