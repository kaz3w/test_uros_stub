#include <micro_ros_arduino.h>


/**************************************************************************************
 * INCLUDE
 **************************************************************************************/

#include <Arduino_CAN.h>
#include <stdio.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <std_msgs/msg/int32.h>

rcl_publisher_t publisher;
std_msgs__msg__Int32 msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
rcl_timer_t timer;

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}

void error_loop(){
  while(1){
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(100);
  }
}

void timer_callback(rcl_timer_t * timer, int64_t last_call_time)
{  
  RCLC_UNUSED(last_call_time);
  if (timer != NULL) {
    RCSOFTCHECK(rcl_publish(&publisher, &msg, NULL));
    msg.data++;
  }
}  

unsigned long prev = millis();


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
  set_microros_transports();


  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  while (!Serial) { }

  allocator = rcl_get_default_allocator();

  //create init_options
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // create node
  RCCHECK(rclc_node_init_default(&node, "micro_ros_arduino_node", "", &support));

  // create publisher
  RCCHECK(rclc_publisher_init_default(
    &publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "micro_ros_arduino_node_publisher"));

  // create timer,
  const unsigned int timer_timeout = 1000;
  RCCHECK(rclc_timer_init_default(
    &timer,
    &support,
    RCL_MS_TO_NS(timer_timeout),
    timer_callback));

  // create executor
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));

  msg.data = 0;
  prev = millis();


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

  RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100)));


}
