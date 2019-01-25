//Датчик влажности почвы резистивный
// Версия 3.0
// Enable debug prints
//#define MY_DEBUG
//#define DEBUG
#define MY_DISABLED_SERIAL

// Enable passive mode
#define MY_PASSIVE_NODE

#define PIN_NRF_POWER     8
// ID сервера (шлюза) умного дома
#define MY_PARENT_NODE_ID 0
#define MY_PARENT_NODE_IS_STATIC
// Этот ID уникальный для каждого сенсора
#define MY_NODE_ID        81
#define MY_RF24_CE_PIN    9
#define MY_RF24_CS_PIN    10
#define RF24_CHANNEL      76
#define PIN_1             14 //К свободному элетроду
#define PIN_2             15 //К резистору делитея 4К7
#define PIN_LED           3  //Индикаторный светодиод
#define PIN_ADC           A2 //Аналоговый вход (резстор и электрод)

#define MY_RADIO_NRF24
#define MY_RF24_PA_LEVEL (RF24_PA_MAX)

#include <MySensors.h>  

// Номера сенсоров
#define CHILD_ID_VCC     0 //Сенсор напряжения батареи
#define CHILD_ID_A0      1 //Сенсоа АЦП влажности почвы
#define TIME_SLEEP       300000 //Время сна между опросами

MyMessage msgVcc(CHILD_ID_VCC, V_VOLTAGE);
MyMessage msgA0(CHILD_ID_A0, V_STATUS);


float readVcc();
void _ReInit();
void(* resetFunc) (void) = 0; // Reset MC function 

void before(){
// Опрделяем пины управляющие делителем 
  pinMode(PIN_1,OUTPUT);
  digitalWrite(PIN_1,LOW);
  pinMode(PIN_2,OUTPUT);
  digitalWrite(PIN_2,LOW);
// Определяем пин светодиода и зажигаем его  
  pinMode(PIN_LED,OUTPUT);
  digitalWrite(PIN_LED,HIGH);
// Определяем пин питания NRF24
  pinMode(PIN_NRF_POWER,OUTPUT);
  digitalWrite(PIN_NRF_POWER,HIGH);
  delay(200);
}

void setup(){
}

void presentation()
{
//Передаем версию скетча
	sendSketchInfo("MOD1", "1.2");
//Передаем объявления сенсоров
	present(CHILD_ID_VCC, S_BINARY,"V");
	present(CHILD_ID_A0, S_HUM,"%");
}

void loop(){
// Переинициализируем NRF24 (иногда без этого не запускается после выключения питания)  
  _ReInit(); 
// Считываем сенсоры  
  float vcc = readVcc();
  uint16_t a0  = AnalogMeasure(); 
// Мигаем светодиодом и посылаем значения сенсоров
  digitalWrite(PIN_LED,HIGH); 
  send(msgVcc.set(vcc,2));
  send(msgA0.set(a0));
  digitalWrite(PIN_LED,LOW);
  delay(100);
// Уходим в режим сна  
  sleep(TIME_SLEEP ); 
}


/**
 * Считываем напряжение питания
 */
float readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = (1100L * 1023)/result;
  float ret = ((float)result)/1000;
  return ret;
}

/**
 * Реинициализация NRF-ки (выдрано из библиотеки)
 * Без него не выходит из режима низкого энергопотребления
 */
void _ReInit(){
    // Save static parent ID in eeprom (used by bootloader)
  hwWriteConfig(EEPROM_PARENT_NODE_ID_ADDRESS, MY_PARENT_NODE_ID);
  // Initialise transport layer
  transportInitialise();
  // Register transport=ready callback
  transportRegisterReadyCallback(_callbackTransportReady);
  // wait until transport is ready
  (void)transportWaitUntilReady(MY_TRANSPORT_WAIT_READY_MS);
}


uint16_t AnalogMeasure(){
   uint16_t a0 = 0;
   uint16_t n  = 0;
// Устанавливаем режим АЦП (на всякий случай)
   analogReference(DEFAULT);
// Цикл из 10 измерений
   for( int i=0; i<10; i++){
// Фаза 1
      digitalWrite(PIN_2,LOW);
      digitalWrite(PIN_1,HIGH);
// Ничего не делаем, пропускаем миллисекунду
      delay(1);
// Фаза 2 (меняем полярность на делителе)
      digitalWrite(PIN_1,LOW);
      digitalWrite(PIN_2,HIGH);
      uint16_t a = analogRead(PIN_ADC);
// После пяти итераций цикла начинаем мерить среднее значение     
      if( i>4 ){
         a0+=a;
         n++;
      }
      delay(1);    
   }
// Отключаем делитель от питания
   digitalWrite(PIN_1,LOW);
   digitalWrite(PIN_2,LOW);
// Вычисляем среднее значение АЦП 
   a0 /= n;
   return(a0);  
}
