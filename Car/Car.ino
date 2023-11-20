#include "Motor.h"
#include <PS2X_lib.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#define PS2_DAT 13
#define PS2_CMD 11
#define PS2_SEL 10
#define PS2_CLK 12
#define pressures true
#define rumble true
#define track1 35  //循迹引脚
#define track2 37
#define track3 33
#define track4 31

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
Motor motor(22, 24, 28, 26, 30, 32, 36, 34, 2, 3, 4, 5);

int speedPinA = 8;
int speedPinB = 9;

PS2X ps2x;

int line = 0;//黑线1 白线0
int underground = 1;//黑1 白0
int error = 0;
byte type = 0;
byte vibrate = 0;
int servoMin = 10;
int servoMax = 500;
int begin1 = 340;  //爪子 320-450（后续只设置到400是因为已经够大了）
int begin2 = 250;  //几把 250-480
int begin3 = 80;   //begin3和4不懂有什么用 先留着吧
int begin4 = 280;
int Sensor[4] = { 0, 0, 0, 0 };  //初始化循迹的值
static int lastTurnDirection = 0;
static unsigned long lastLineTime = millis(); // 上一次看到线的时间

void setup() {
  motor.speed(100);
  Serial.begin(9600);
  error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);
  pwm.begin();
  pwm.setPWMFreq(50);
  servos_begin();
  Track_Init();  //循迹模块初始化
}

void loop() {
  //aprint();  //打印循迹的高低电平
  if (error == 1)
    return;

  if (type == 2) {
    return;
  } else {
    ps2x.read_gamepad(false, vibrate);

    byte LY = ps2x.Analog(PSS_LY);
    byte LX = ps2x.Analog(PSS_LX);
    byte RY = ps2x.Analog(PSS_RY);
    byte RX = ps2x.Analog(PSS_RX);

    if (LY < 110) {
      motor.speed(250 - LY);
      motor.forward();
      delay(50);
    }

    if (LY > 140) {
      motor.speed(LY - 10);
      motor.backward();
      delay(50);
    }

    if (LX < 110) {
      motor.speed(150);
      motor.left();
      delay(50);
    }

    if (LX > 140) {
      motor.speed(150);
      motor.right();
      delay(50);
    }

    if (RY > 132) {
      begin1 = begin1 + 5;
      pwm.setPWM(1, 0, begin1);
      if (begin2 > 471) {
        begin2 = 470;
      }
    }

    if (RY < 124) {
      begin1 = begin1 - 5;
      pwm.setPWM(1, 0, begin1);
      if (begin2 < 230) {
        begin2 = 229;
      }
    }

    if (LY >= 110 && LY <= 140 && LX >= 110 && LX <= 140) {
      motor.stop();
      delay(50);
    }

    if (ps2x.Button(PSB_L1)) {
      begin1 = begin1 + 5;
      pwm.setPWM(0, 0, begin1);
      if (begin1 > 400) {
        begin1 = 399;
      }
    }

    if (ps2x.Button(PSB_L2)) {
      while (1) {
        ps2x.read_gamepad(false, vibrate);
        Sensor_Read();  //不断地读取循迹模块的高低电平
        delay(50);
        follow_line();
        if (ps2x.Button(PSB_R2)) {
          break;
        }
      }
    }

    if (ps2x.Button(PSB_R1)) {
      begin1 = begin1 - 5;
      pwm.setPWM(0, 0, begin1);
      Serial.print(begin1);
      if (begin1 < 330) {
        begin1 = 329;
      }
    }
  }
}

void servos_begin() {
  pwm.setPWM(0, 0, begin1);
  pwm.setPWM(1, 0, begin2);
  pwm.setPWM(2, 0, begin3);
  pwm.setPWM(3, 0, begin4);
}
void Track_Init() {
  //循迹模块D0引脚初始化，设置为输入模式
  pinMode(track1, INPUT);
  pinMode(track2, INPUT);
  pinMode(track3, INPUT);
  pinMode(track4, INPUT);
}

void Sensor_Read() {
  Sensor[0] = digitalRead(track1);  //检测到黑线为高电平（1），白线为低电平（0)
  Sensor[1] = digitalRead(track2);
  Sensor[2] = digitalRead(track3);
  Sensor[3] = digitalRead(track4);
}

void follow_line() {

  pwm.setPWM(1, 0, 460);
  if (Sensor[0] == underground && Sensor[1] == line && Sensor[2] == line && Sensor[3] == underground) {
    motor.speed(100);
    motor.backward();
    Serial.print("直行\n");
    lastTurnDirection = 0;
  } 
  // 小左转
  else if (Sensor[0] == underground && Sensor[1] == line && Sensor[2] == underground && Sensor[3] == underground) {
    motor.speed(155);
    motor.right();
    Serial.print("小左转\n");
    lastTurnDirection = -1;
  }
  // 小右转
  else if (Sensor[0] == underground && Sensor[1] == underground && Sensor[2] == line && Sensor[3] == underground) {
    motor.speed(155);
    motor.left();
    Serial.print("小右转\n");
    lastTurnDirection = 1;
  }
  // 大左转
  
  else if (Sensor[0] == line && Sensor[1] == underground && Sensor[2] == underground && Sensor[3] == underground) {
    motor.speed(165);
    motor.right();
    Serial.print("大左转\n");
    lastTurnDirection = -1;
  }
  // 大右转
   else if (Sensor[0] == underground && Sensor[1] == line && Sensor[2] == line && Sensor[3] == line) {
    motor.speed(165);
    motor.left();
    Serial.print("小右转\n");
    lastTurnDirection = 1;
  }
  // 急急急左转
  
  else if (Sensor[0] == line && Sensor[1] == line && Sensor[2] == line && Sensor[3] == underground) {
    motor.speed(175);
    motor.right();
    Serial.print("大左转\n");
    lastTurnDirection = -1;
  }
  // 急急急右转
  else if (Sensor[0] == underground && Sensor[1] == underground && Sensor[2] == underground && Sensor[3] == line) {
    motor.speed(175);
    motor.left();
    Serial.print("大右转\n");
    lastTurnDirection = 1;
  }
  // 左急转弯
  else if (Sensor[0] == line && Sensor[1] == line && Sensor[2] == underground && Sensor[3] == underground) {
    motor.speed(175);
    motor.right();
    Serial.print("左急转弯\n");
    lastTurnDirection = -1;
  }
  // 右急转弯
  else if (Sensor[0] == underground && Sensor[1] == underground && Sensor[2] == line && Sensor[3] == line) {
    motor.speed(175);
    motor.left();
    Serial.print("右急转弯\n");
    lastTurnDirection = 1;
  }
  // 直角弯（所有传感器都丢失线）
  else if (Sensor[0] == underground && Sensor[1] == underground && Sensor[2] == underground && Sensor[3] == underground) {
    if (lastTurnDirection == -1) {
      motor.speed(180);
      motor.right();
      Serial.print("继续左转\n");
    } else if (lastTurnDirection == 1) {
      motor.speed(180);
      motor.left();
      Serial.print("继续右转\n");
    } else {
      motor.speed(120);
      motor.backward();
      Serial.print("搜索线\n");
    }
  }
  else if (Sensor[0] == line && Sensor[1] == line && Sensor[2] == line && Sensor[3] == line) {
    if (lastTurnDirection == -1) {
      motor.speed(180);
      motor.right();
      Serial.print("继续左转\n");
    } else if (lastTurnDirection == 1) {
      motor.speed(180);
      motor.left();
      Serial.print("继续右转\n");
    } else {
      motor.speed(120);
      motor.backward();
      Serial.print("搜索线\n");
    }
  }
  // 如果其他所有传感器组合都未满足条件，可以选择一个默认行为，比如停止或者直行
  else {
    motor.speed(100);
    motor.backward();
    Serial.print("默认行为：直行\n");
  }
  // 这里没有使用delay，以便更快地响应传感器的变化
}


/*void aprint()	//在串口打印循迹的高低电平
{
  Serial.print(Sensor [0]);
  Serial.print("---");
  Serial.print(Sensor [1]);
  Serial.print("---");
  Serial.print(Sensor [2]);
  Serial.print("---");
  Serial.println(Sensor [3]);
}*/