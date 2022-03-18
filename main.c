#include <string.h>
#include "myriota_user_api.h"

#define RECEIVE_TIMEOUT 100  // [ms]
#define ACK_STRING "\nOK\n"
#define OVERLOAD_STRING "\nOVERLOADED\n"
#define SENSOR_MESSAGE_SIZE 2

static void *leuart_handle = NULL;

static unsigned int sequence_number = 0;

// Format of the messages to be transmitted. Values are little endian
// 2+4+4+4+2=16bytes
typedef struct {
  uint16_t sequence_number; 
  int32_t latitude;   // scaled by 1e7, e.g. -891234567 (south 89.1234567)
  int32_t longitude;  // scaled by 1e7, e.g. 1791234567 (east 179.1234567)
  uint32_t time;      // epoch timestamp of last fix
  uint16_t sensor_message; 
} __attribute__((packed)) tracker_message;

// Read string from UART with timeout, return number of bytes read
int UARTReadStringWithTimeout(void *Handle, uint8_t *Rx, size_t MaxLength) {
  const uint32_t start = TickGet();
  int count = 0;
  while (TickGet() - start < RECEIVE_TIMEOUT) {
    uint8_t ch;
    if (UARTRead(Handle, &ch, 1) == 1) {
      if (count < MaxLength) {
        Rx[count] = ch;
      }
      count++;
    }
  }
  return count;
}

static time_t UartComm() {
  uint8_t Rx[SENSOR_MESSAGE_SIZE] = {0};
  int len = UARTReadStringWithTimeout(leuart_handle, Rx, MAX_MESSAGE_SIZE);
  if (len > SENSOR_MESSAGE_SIZE) {
    printf("LEUART RX buffer overloaded\n");
    UARTWrite(leuart_handle, (uint8_t *)OVERLOAD_STRING,
              strlen(OVERLOAD_STRING));
  } else if (len > 0) {
    UARTWrite(leuart_handle, (uint8_t *)Rx, len);
    UARTWrite(leuart_handle, (uint8_t *)ACK_STRING, strlen(ACK_STRING));
    
    //ScheduleMessage(Rx, len);
    printf("Scheduled message: 0x");
    for (int i = 0; i < len; i++) printf("%02x", Rx[i]);
    printf("\n");
  } else {
    printf("No data received\n");
  }

  if (GNSSFix()) printf("Failed to get GNSS Fix, using last known fix\n");

  // The timestamp and the coordinate are incorrect without valid GNSS fix
  if (HasValidGNSSFix()) {
    int32_t lat, lon;
    time_t timestamp;
    LocationGet(&lat, &lon, &timestamp);
    uint16_t sensor_message = (Rx[0] << 8) + (Rx[1]);     

    printf("sensor_message: %u\n", sensor_message);

    const tracker_message message = {sequence_number, lat, lon, timestamp, sensor_message};

    ScheduleMessage((void *)&message, sizeof(message));

    printf("Scheduled message: %u %f %f %u %u\n", sequence_number, lat * 1e-7,
           lon * 1e-7, (unsigned int)timestamp, sensor_message);

    sequence_number++;
  }

  return OnLeuartReceive();
}

void AppInit() {
  ScheduleJob(UartComm, OnLeuartReceive());
  leuart_handle = UARTInit(LEUART, 9600, 0);
  if (leuart_handle == NULL) {
    printf("Failed to initialise leuart\n");
    return;
  }
}
