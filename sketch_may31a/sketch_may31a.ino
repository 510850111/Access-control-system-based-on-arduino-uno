/*
 
============================================================================================================

Arduino UNO        Touch Shield      RFID         buzzer        SG90        MG955             HC-SR04
-------------------------------------------------------------------------------------------------------------
    5V                5V                                        7.4V        7.4V              VCC
-------------------------------------------------------------------------------------------------------------
   3.3V                             3.3V
-------------------------------------------------------------------------------------------------------------
    GND               GND            GND          GND           GND          GND               GND
-------------------------------------------------------------------------------------------------------------
    A4                SDA
-------------------------------------------------------------------------------------------------------------
    A5                SCL
-------------------------------------------------------------------------------------------------------------
    4                                             Vin
-------------------------------------------------------------------------------------------------------------
    ~5                                RST
-------------------------------------------------------------------------------------------------------------
    ~6                                                           Sign     
------------------------------------------------------------------------------------------------------------
     7                                                                                        TrigPin
-------------------------------------------------------------------------------------------------------------
     8                                                                                        EchoPin 
-------------------------------------------------------------------------------------------------------------
    ~9                                                                      Sign
-------------------------------------------------------------------------------------------------------------
    ~10                               SDA
-------------------------------------------------------------------------------------------------------------    
    ~11                               MOSI
-------------------------------------------------------------------------------------------------------------    
    12                                MISO
-------------------------------------------------------------------------------------------------------------    
    13                                SCK
-------------------------------------------------------------------------------------------------------------    
    D2                IRQ
============================================================================================================


*/
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
Servo myservo;  // create servo object to control a servo
int pos = 0;    // variable to store the servo position

#define RST_PIN         9           
#define SS_PIN          10        

MFRC522 mfrc522(SS_PIN, RST_PIN);   // 创建一个 MFRC522 实例.

MFRC522::MIFARE_Key key;

#include "mpr121.h"
#include "i2c.h"

#define DIGITS 11 //最大键盘输入位数为11位,这里我只用到了6位

//在键盘上可以看到每一个数字对应一个编码,比如数字1上写有P0,数字1上写有P4,*号键上有P3,在此一一对应.
#define ONE 0
#define TWO 4
#define THREE 8
#define FOUR 1
#define FIVE 5
#define SIX 9
#define SEVEN 2
#define EIGHT 6
#define NINE 10
#define ELE9 7    //0号键
#define ELE10 3   //*号键
#define ELE11 11  //#号键,我用它来做确认键

//中断引脚编号
int irqpin = 2;  // D2

//预置的密码
char PWD[7] = {'3','5','7','7','5','3',NULL}; 

//卡号,卡号采用byte存储,
//byte UID[] = {206,147,57,69,};
 byte UID[] = {0xCE,0x93,0x39,0x45}; 

//输入的密码
char InputPWD[6];

//当前已经输入了多少位密码.
int now = 0;

//超声波测距模块
const int TrigPin = 7; 
const int EchoPin = 8; 
float distance; 


void setup()
{
    myservo.attach(9);  // attaches the servo on pin 9 to the servo object
    
    //检测各项功能是否正常
    //检测开始----------------------------->
    
    openTheFuckingDoor();
    buzzer1(4,1500);
    
    //<------------------------------检测结束
   
    
    Serial.begin(9600); //初始化串口与PC的通信
 
    
    // 连接超声波模块SR04的引脚
    pinMode(TrigPin, OUTPUT); 
    // 要检测引脚上输入的脉冲宽度，需要先设置为输入状态
    pinMode(EchoPin, INPUT); 
    
    while (!Serial);    // 如果没有打开串口就什么都不做(基于ATMEGA32U4 arduino添加)
    SPI.begin();        //初始化SPI
    mfrc522.PCD_Init(); //初始化MFRC522卡
    
    // 使用十六进制的0xFF初始化读取的UID,换成十进制就是255
    for (byte i = 0; i < 6; i++) {
      key.keyByte[i] = 0xFF;
    }
    
    //显示已经读取出来的UID(此时是已经初始化了,显示的应该全是F)
    dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
    
    Serial.println();
    
    //确保中断的引脚是输入状态并且应该被拉高.
    pinMode(irqpin, INPUT);
    digitalWrite(irqpin, HIGH);
    
    //output on ADC4 (PC4, SDA)
    DDRC |= 0b00010011;
    //把I2C拉高
    PORTC = 0b00110000; 
    // initalize I2C bus. Wiring lib not used. 
    i2cInit();
    
    delay(100);
    // 初始化 mpr121
    mpr121QuickConfig();
    
    //把键盘输入放进中断,当按键被按下的时候,引脚IRQ会被拉低,然后执行getnumber函数.
    attachInterrupt(0,getNumber,LOW);

    //初始化完成
    Serial.println("Ready...");
    //loop();
}

void loop()
{
    //Distance();
    
    // 寻找是否有卡
    if ( ! mfrc522.PICC_IsNewCardPresent())
      return;
    
    // 选择一张卡
    if ( ! mfrc522.PICC_ReadCardSerial())
      return;
      
    Serial.print(F("Card UID:"));
    //dump_byte_array函数的作用是把卡号显示出来
    dump_byte_array(mfrc522.uid.uidByte,mfrc522.uid.size);
    Serial.println();
}

void getNumber()
{
    int i = 0;
    int touchNumber = 0;
    uint16_t touchstatus;
    char digits[DIGITS];//存放输入密码的数组
  
    touchstatus = mpr121Read(0x01) << 8;
    touchstatus |= mpr121Read(0x00);
  
    for (int j=0; j<12; j++)  // 检测输入有多少次.
    {
        if ((touchstatus & (1<<j)))  touchNumber++;
    }
  
    if (touchNumber == 1)
    {
      if (touchstatus & (1<<SEVEN))
        digits[i] = '7';
      else if (touchstatus & (1<<FOUR))
        digits[i] = '4';
      else if (touchstatus & (1<<ONE))
        digits[i] = '1';
      else if (touchstatus & (1<<EIGHT))
        digits[i] = '8';
      else if (touchstatus & (1<<FIVE))
        digits[i] = '5';
      else if (touchstatus & (1<<TWO))
        digits[i] = '2';
      else if (touchstatus & (1<<NINE))
        digits[i] = '9';
      else if (touchstatus & (1<<SIX))
        digits[i] = '6';
      else if (touchstatus & (1<<THREE))
        digits[i] = '3';
      else if (touchstatus & (1<<ELE9))
        digits[i] = '0';
      else if (touchstatus & (1<<ELE10))
        digits[i] = '*';
      else if (touchstatus & (1<<ELE11))
        digits[i] = '#';
      InputPWD[now++] = digits[i];//把输入的密码存放到InputPWD数组里面
      buzzer1(4,1000);
  
      if (InputPWD[now] == '*')
      {
         now = 0;
         memset(InputPWD, 0, sizeof(InputPWD));
         
      }
      
      //如果密码的的长度达到了6或者按下了#号键
      if (now == 6  || digits[i] == '#')
      {
          buzzer1(4,1000);
          if (euqal(InputPWD,PWD,6))
          {
            openTheFuckingDoor();
            buzzer1(4,1500);
          }
          else
          {
            Serial.println("Forbiden to access!");
          }
          now = 0;
          Serial.println(InputPWD);
          i = 0;
  
      }
    
      int key;
      key=digits[i];
      i++;
      
    }
    return;
}

//打印出(十六进制与十进制)读取的非接触式IC卡卡号
void dump_byte_array(byte *buffer, byte bufferSize) 
{
    for (byte i = 0; i < bufferSize; i++) 
    {
        Serial.print(buffer[i],HEX);//十六进制打印
        Serial.print(" ");
    }
    Serial.println();
    buzzer1(4,500);
    //十进制打印
    for (byte i = 0; i < bufferSize; i++) 
    {
      Serial.print(buffer[i]);
      Serial.print(",");
    }
     Serial.println();
     if (equalUID(buffer,bufferSize)) openTheFuckingDoor();
    return;
}


void buzzer1(int buzzer,int time)
{
      pinMode(buzzer,OUTPUT);
      digitalWrite(buzzer,HIGH);//发声音
      delay(time);//延时
      digitalWrite(buzzer,LOW);//不发声音
      return;
}

//判断输入的密码是否正确
int euqal(char A[],char B[],int Size)
{
    for(int i = 0;i < Size;i++)
    {
      if(A[i] != B[i])
      {
        Serial.println("The password is wrong!");
        return 0; 
      }
   }
  return 1;
}

//判断卡号和预置的卡号是否匹配,匹配返回1,不匹配返回0
int equalUID(byte *buffer,byte bufferSize) 
{
    Serial.print("This is equalUID!,buffersizeis :");
     Serial.println(bufferSize);
    for (byte i = 0; i < bufferSize; i++) 
    {
      if (buffer[i] != UID[i])
      return 0;
    }
    Serial.println("The UID is available!!!");
    return 1;
}

//超声波测距
void Distance()
{
      // 产生一个10us的高脉冲去触发TrigPin 
        digitalWrite(TrigPin, LOW); 
        delayMicroseconds(2); 
        digitalWrite(TrigPin, HIGH); 
        delayMicroseconds(10);
        digitalWrite(TrigPin, LOW); 
    // 检测脉冲宽度，并计算出距离
        distance = pulseIn(EchoPin, HIGH) / 58.00;
        //Serial.print(distance); 
        //Serial.println();
        if(distance <= 100 && distance >= 5)   openTheFuckingDoor();
}


void openTheFuckingDoor()
{
     buzzer1(4,500);
     buzzer1(4,500);
     Serial.println("open the door");
    for (pos = 0; pos <= 180; pos += 1) 
    { // goes from 0 degrees to 180 degrees
      // in steps of 1 degree
      myservo.write(pos);              // tell servo to go to position in variable 'pos'
      delay(15);                       // waits 15ms for the servo to reach the position
    }
    for (pos = 180; pos >= 0; pos -= 1) 
    { // goes from 180 degrees to 0 degrees
      myservo.write(pos);              // tell servo to go to position in variable 'pos'
      delay(15);                       // waits 15ms for the servo to reach the position
    }
  
}


