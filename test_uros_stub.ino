/*
  CANWrite

  Write and send CAN Bus messages

  See the full documentation here:
  https://docs.arduino.cc/tutorials/uno-r4-wifi/can
*/

/**************************************************************************************
 * INCLUDE
 **************************************************************************************/

#include <Arduino_CAN.h>

/**************************************************************************************
 * CONSTANTS
 **************************************************************************************/

#if defined(ARDUINO_MINIMA)
static uint32_t const CAN_ID = 0x20;
#else
static uint32_t const CAN_ID = 0x21;
#endif

/**************************************************************************************
 * SETUP/LOOP
 **************************************************************************************/

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  while (!Serial) { }

  if (!CAN.begin(CanBitRate::BR_500k))
  {
    Serial.println("CAN.begin(...) failed.");
    for (;;) {}
  }
}

static uint32_t msg_cnt = 0;

void loop()
{
  /* Assemble a CAN message with the format of
   * 0xCA 0xFE 0x00 0x00 [4 byte message counter]
   */
#if defined(ARDUINO_MINIMA)
  uint8_t const msg_data[] = {0xCA,0xFE,0,0,0,0,0,0};
#else //defined(ARDUINO_UNOWIFIR4)
  uint8_t const msg_data[] = {0xBE,0xEF,0,0,0,0,0,0};
#endif
  memcpy((void *)(msg_data + 4), &msg_cnt, sizeof(msg_cnt));
  CanMsg const msg(CanStandardId(CAN_ID), sizeof(msg_data), msg_data);

  /* Transmit the CAN message, capture and display an
   * error core in case of failure.
   */
  if (int const rc = CAN.write(msg); rc < 0)
  {
    Serial.print  ("CAN.write(...) failed with error code ");
    Serial.println(rc);
   digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    for (;;) { }
  }

  unsigned long cur = millis();

  /* Increase the message counter. */
  msg_cnt++;

  /* Only send one message per second. */
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(1);
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW

  if (CAN.available())
  {
    // for (int ii=0; ii<5; ii++) {
    //   delay(2);
    //   digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    //   delay(3);
    //   digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
    // }

    CanMsg const msg = CAN.read();
    // Serial.println(msg);
  }

  // unsigned long elapsed = millis() - cur;
  // delay(250 - elapsed);                      // wait for a second
}
