#include "DHT.h"
#include <SoftwareSerial.h>

#define DHTPIN 4       // SDA 핀의 설정
#define light_sensor 5 // 조도센서 핀 설정
#define YL69 4         // 토양습도센서 핀 설정
#define LED_RELAY1 8    // LED 릴레이 핀 설정
#define LED_RELAY2 9
#define HIT_RELAY 12  // 히터 릴레이 핀 설정 D49
#define FAN_RELAY1 11    // 공기순황용 팬 릴레이 핀 설정
#define FAN_RELAY2 10
#define FAN_RELAY3 13   // 펠티어 팬 릴레이 test
#define WPUMP_RELAY 7
#define tx 6
#define rx 5

#define DHTTYPE DHT22   // DHT22 (AM2302) 센서종류 설정

SoftwareSerial bluetooth(tx, rx);

extern volatile unsigned long timer0_millis;
unsigned long hit_timeVal, pump_timeVal, mainpump_timeVal;
unsigned long hit_times, pump_times, mainpump_times;
unsigned long readTime;

int stop_code;
int hour, minute;
int day_count;

float h; // 습도
float t; // 온도
int   gndH; // 토양습도
int   light; // 조도

int   select;   // 파종 모종 선택
int   set_type;  //  설정된 작물 번호
float set_h;     // 설정된 습도
float set_t;     // 설정된 온도
int   set_light;    // 설정된 조도
int   set_fan;

int hit_rest;
int mainpump_rest;
int pump_rest;
boolean mainpump_state;
boolean mainpump_time;
boolean pump_state;
boolean pump_time;
boolean hit_state;
boolean hit_time;

typedef struct s_relay
{
  boolean hit_relay;
  boolean led_relay_main;
  boolean led_relay_sub;
  boolean fan_relay_st;
  boolean fan_relay_lst;
  boolean cold_fan;
  boolean wpump_relay;
  boolean main_wpump_relay;
  int     start;
} t_relay;

t_relay g_data;

int  finish;

DHT dht(DHTPIN, DHTTYPE);

void pinModeSet()
{
  Serial.begin(9600);
  bluetooth.begin(9600);
  pinMode(LED_RELAY1, OUTPUT);
  pinMode(LED_RELAY2, OUTPUT);
  pinMode(HIT_RELAY, OUTPUT);
  pinMode(FAN_RELAY1, OUTPUT);
  pinMode(FAN_RELAY2, OUTPUT);
  pinMode(FAN_RELAY3, OUTPUT);
  pinMode(WPUMP_RELAY, OUTPUT);
}

void start_Set()
{
  g_data.hit_relay = 0;
  g_data.led_relay_main = 1;
  g_data.led_relay_sub = 0;
  g_data.fan_relay_st = 1;
  g_data.fan_relay_lst = 0;
  g_data.cold_fan = 0;
  g_data.wpump_relay = 0;
  g_data.main_wpump_relay = 0;

  day_count = 1;
  set_type = 0;
  set_t = 0;
  set_h = 0;
  set_light = 0;
  set_fan = 0;
  timer0_millis = 0;
  
  digitalWrite(FAN_RELAY1, HIGH);
  digitalWrite(FAN_RELAY2, HIGH);
  digitalWrite(FAN_RELAY3, LOW);
  digitalWrite(LED_RELAY1, HIGH);
  digitalWrite(WPUMP_RELAY, LOW);
  digitalWrite(LED_RELAY2, HIGH);
  digitalWrite(HIT_RELAY, LOW);
}

//int makeLight(int light)
//{
//  int ret;
//  
//  ret = (abs(light - 1200)) / 10;
//  return (ret);
//}

int makeGndHumidity(int gndH)
{
  int ret;

  Serial.print("gndH ::: ");
  Serial.println(gndH);
  ret = (abs(gndH - 1200)) / 10;
  return (ret);
}

void send_sensor_data()
{
  Serial.println("data send *******");

  bluetooth.print(finish);
  bluetooth.print(", ");
  bluetooth.print(t);
  bluetooth.print(", ");
  bluetooth.print(h);
  bluetooth.print(", ");
  bluetooth.print(light);
  bluetooth.print(", ");
  bluetooth.print(gndH);
  bluetooth.print(", ");
  bluetooth.print(day_count);
//  bluetooth.print(",");
//  bluetooth.print(set_type);
//  bluetooth.print(",");
//  bluetooth.print(select);
  
  Serial.println(t);
  Serial.println(h);
  Serial.println(light);
  Serial.println(gndH);
  Serial.println(set_type);
  Serial.println(select);
  Serial.println("data send finish *******");
}

void sleep_all()
{
  g_data.hit_relay = 0;
  g_data.led_relay_main = 0;
  g_data.led_relay_sub = 0;
  g_data.fan_relay_st = 0;
  g_data.fan_relay_lst = 0;
  g_data.cold_fan = 0;
  g_data.main_wpump_relay = 0;
  g_data.wpump_relay = 0;
}

void recieve_setting_data()
{
  int save_type, save_t, save_h, save_light, save_fan, save_check, save_select;
  int end_set, start_set;
  long check_sum;
  unsigned long long int num;

  end_set = 0;
  if (bluetooth.available())
  {
    start_set = bluetooth.parseInt();
    save_select = bluetooth.parseInt(); // 1: 모종, 2: 파종
    save_type = bluetooth.parseInt(); // 작물 종류 1:상추 2:청경채 3: 들깨 4: 엔디브
    save_t = bluetooth.parseInt();
    save_h = bluetooth.parseInt();
    save_light = bluetooth.parseInt();
    save_fan = bluetooth.parseInt();
    end_set = bluetooth.parseInt();
    check_sum = bluetooth.parseInt();
    Serial.print("dd : ");
    Serial.println(start_set);
    Serial.println(end_set);
    Serial.println(check_sum);
    save_check = start_set + save_select + save_type + save_t + save_h + save_light + save_fan + end_set;
    Serial.println(save_check);
    if (start_set == 96) // 작동 중지
      stop_code = 1;
    else if (start_set ==  95 && end_set == 95 && save_check == check_sum)
    {
      stop_code = 0;
      if (((select != save_select) || (set_type != save_type)) && save_type != 0)
      {
        day_count = 1;
        finish = 0;
        timer0_millis = 0;
      }
      if (save_type > 0)
      {
        set_type = save_type;
        select = save_select;
        set_t = save_t;
        set_h = save_h;
        set_light = save_light;
        set_fan = save_fan;
        g_data.start = 1; 
      }
    }
  }
}

void read_sensor_data()
{
  h = dht.readHumidity();
  t = dht.readTemperature();
  gndH = analogRead(YL69);
  light = analogRead(light_sensor);
  gndH = makeGndHumidity(gndH);
}

boolean hit_timer()
{
  hit_times = millis();
  if (hit_state == false)
  {
    hit_timeVal = millis();
    hit_state = true;
  }
  Serial.print("timeVal : ");
  Serial.println(hit_timeVal);
  Serial.print("millis : ");
  Serial.println(millis());
  Serial.print("hit_State : ");
  Serial.println(hit_state);
  Serial.print("hit_rest : ");
  Serial.println(hit_rest);
  if (hit_state == true && hit_rest == 1) // 5분간 false 상태 반환
  {
    if (hit_times - hit_timeVal >= 20000) // test code 20초
//    if (hit_times - hit_timeVal >= 300000)
    {
      hit_rest = 0;
      hit_state = false;
    }
    return (false);
  }
  if (hit_state == true && hit_rest == 0) // 2분간 true 상태 반환
  {
    if (hit_times - hit_timeVal >= 20000) // test code 20초 
//    if (hit_times - hit_timeVal >= 120000)
    {
      hit_rest = 1;
      hit_state = false;
    }
    return (true);
  }
  return (false);
}

boolean mainpump_timer()
{
  mainpump_times = millis();
  if (mainpump_state == false)
  {
    mainpump_timeVal = millis();
    mainpump_state = true;
  }
  if (mainpump_state == true && mainpump_rest == 1)
  {
    if (mainpump_times - mainpump_timeVal >= 60000) // test code 1분 휴식
//    if (mainpump_times - mainpump_timeVal >= 40200000)
    {
      mainpump_rest = 0;
      mainpump_state = false;
    }
  }
  if (mainpump_state == true && mainpump_rest == 0)
  {
    if (mainpump_times - mainpump_timeVal >= 10000) // testcode 15초
//    if (mainpump_times - mainpump_timeVal >= 60000)
    {
        mainpump_rest = 1;
        mainpump_state = false;
    }
    return (true);
  }
  return (false);
}

boolean pump_timer()
{
  pump_times = millis();
  if (pump_state == false && hour != -1)
  {
    pump_timeVal = millis();
    pump_state = true;
  }
  Serial.print("pump timeVal : ");
  Serial.println(pump_timeVal);
  Serial.print("pump millis : ");
  Serial.println(millis());
  Serial.print("pump_State : ");
  Serial.println(pump_state);
  Serial.print("pump_rest : ");
  Serial.println(pump_rest);
  if (pump_state == true && pump_rest == 1)
  {
    if (pump_times - pump_timeVal >= 10800000)
    {
      pump_rest = 0;
      pump_state = false;
    }
  }
  if (pump_state == true && pump_rest == 0)
  {
    if (pump_times - pump_timeVal >= 30000)
    {
        pump_rest = 1;
        pump_state = false;
    }
    return (true);
  }
  return (false);
}

void timer()
{
  Serial.print("time : ");
  Serial.println(millis());
  if (g_data.start == 0)
    timer0_millis = 0;
 if (millis() >= 90000) // test code 2분
//  if (millis() >= 86400000) // 1일 기준 시간
  {
    day_count++;
    timer0_millis = 0;
    mainpump_state = false;
    pump_state = false;
    hit_state =false;
    mainpump_rest = 0;
    pump_rest = 0;
    hit_rest = 0;
  }
}

void check_finish()
{
  if (day_count == 28 && set_type == 1)
  {
    finish = 1;
  }
  else if (set_type == 2 && day_count == 4 && select == 2)
    finish = 1;
  else if (set_type == 2 && day_count == 30 && select == 1)
    finish = 1;
  else if (set_type == 3 && day_count == 8 && select == 2)
    finish = 1;
  else if (set_type == 3 && day_count == 30 && select == 1)
    finish = 1;
  else if (set_type == 4 && day_count == 30)
    finish = 1;
  if (finish == 1 || stop_code == 1)
  {
    g_data.start = 0;
    sleep_all();
  }
  Serial.print("day_count :: ");
  Serial.print(day_count);
}

void control_flag()
{
  if (set_type == 0)
    sleep_all();
  if (g_data.main_wpump_relay)
  {
    g_data.fan_relay_st = 0;
    g_data.fan_relay_lst = 0;
    g_data.cold_fan = 0;
    g_data.hit_relay = 0;
  }
  else if (g_data.hit_relay)
  {
    g_data.fan_relay_st = 0;
    g_data.fan_relay_lst = 0;
    g_data.cold_fan = 0;
    g_data.wpump_relay = 0;
  }
  else if (g_data.cold_fan)
  {
    g_data.hit_relay = 0;
    g_data.wpump_relay = 0;
  }
  else if (g_data.wpump_relay)
  {
    g_data.cold_fan = 0;
    g_data.hit_relay = 0;
    g_data.fan_relay_st = 0;
    g_data.fan_relay_lst = 0;
  }
}

void setup() {
  
  pinModeSet();
  start_Set();

  dht.begin();  
  Serial.println("reset on");
}
 
void loop() {

  read_sensor_data();

  send_sensor_data(); // 현재 측정 센서 값 보내기

  recieve_setting_data(); // 설정된 데이터 받기
  
  timer();

// LED 제어
  if (set_light == 2 || light <= 750)
  {
    g_data.led_relay_main = 1;
    g_data.led_relay_sub = 1;
  }
  else if (set_light == 1)
  {
    g_data.led_relay_main = 1;
    g_data.led_relay_sub = 0;
  }

// 공기 습도에 따른 제어
  if (h > set_h)
  {
    g_data.fan_relay_st = 1;
  }
  else if (h < set_h - 5)
    g_data.fan_relay_st = 0;
    
// 온도에 따른 히터 제어
Serial.println(t);
Serial.println(set_t);
  if (t < set_t - 3)
  {
    hit_time = hit_timer();
    if (hit_time)
    {
      g_data.hit_relay = 1;
      g_data.wpump_relay = 0;
    }
    else if (!hit_time)
    {
      g_data.hit_relay = 0;
      g_data.fan_relay_st = 1;
      g_data.fan_relay_lst = 1;
    }
  }
  else if (t > set_t + 3)
  {
    g_data.cold_fan = 1;
    g_data.fan_relay_st = 1;
    g_data.fan_relay_lst = 1;
    g_data.hit_relay = 0;
  }
  else if (t <= set_t && g_data.cold_fan == 1)
  {
    g_data.cold_fan = 0;
    g_data.fan_relay_st = 0;
    g_data.fan_relay_lst = 0;
  }
  else if (t >= set_t && g_data.hit_relay == 1)
  {
    g_data.hit_relay = 0;
    g_data.fan_relay_st = 0;
    g_data.fan_relay_lst = 0;
  }

 // 공기 순환 팬제어 
  if (set_fan == 1)
  {
    g_data.fan_relay_st = 1;
    g_data.fan_relay_lst = 0;
  }
  else if (set_fan == 2)
  {
    g_data.fan_relay_st = 1;
    g_data.fan_relay_lst = 1;
  }
  
// 시간에 따른 pump 제어
Serial.print("start");
Serial.println(day_count);
  if (day_count % 2 == 0 && select == 1)
  {
    mainpump_time = mainpump_timer();
    if (mainpump_time)
    {
      g_data.main_wpump_relay = 1;
    }
    else if (!mainpump_time)
      g_data.main_wpump_relay = 0;
  }
  else if (gndH < 70 && select == 2)
  {
    pump_time = pump_timer();
    if (pump_time)
    {
      g_data.wpump_relay = 1;
    }
    else if (!pump_time)
    {
      g_data.wpump_relay = 0;
    }
  }
  else if (gndH >= 70 || day_count % 3 != 0)
  {
    g_data.wpump_relay = 0;
    g_data.main_wpump_relay = 0;
  }
  
  control_flag();

  check_finish();

  // 릴레이 제어 구문
  if (g_data.fan_relay_st == 0)
    digitalWrite(FAN_RELAY1, HIGH);
  if (g_data.fan_relay_lst == 0)
    digitalWrite(FAN_RELAY2, HIGH);
  if (g_data.cold_fan == 0)
    digitalWrite(FAN_RELAY3, LOW);
  if (g_data.led_relay_main == 0)
    digitalWrite(LED_RELAY1, HIGH);
  if (g_data.main_wpump_relay == 0 && g_data.wpump_relay == 0)
    digitalWrite(WPUMP_RELAY, LOW);
  if (g_data.led_relay_sub == 0)
    digitalWrite(LED_RELAY2, HIGH);
  if (g_data.hit_relay == 0)
    digitalWrite(HIT_RELAY, LOW);

  delay(100);
  Serial.println("start test set");
  Serial.println(g_data.led_relay_main);
  Serial.println(g_data.main_wpump_relay);
  Serial.println(g_data.wpump_relay);
  Serial.println(g_data.hit_relay);
  Serial.println(g_data.led_relay_sub);
  Serial.println(g_data.fan_relay_st);
  Serial.println(g_data.fan_relay_lst);
  Serial.println(g_data.cold_fan);
  Serial.println("finish test set");

  if (g_data.led_relay_main == 1)
    digitalWrite(LED_RELAY1, LOW);
  if (g_data.hit_relay == 1)
    digitalWrite(HIT_RELAY, HIGH);
  if (g_data.main_wpump_relay == 1 || g_data.wpump_relay == 1)
    digitalWrite(WPUMP_RELAY, HIGH);
  if (g_data.led_relay_sub == 1)
    digitalWrite(LED_RELAY2, LOW);
  if (g_data.fan_relay_st == 1)
    digitalWrite(FAN_RELAY1, LOW);
  if (g_data.fan_relay_lst == 1)
    digitalWrite(FAN_RELAY2, LOW);
  if (g_data.cold_fan == 1)
    digitalWrite(FAN_RELAY3, HIGH);
}
