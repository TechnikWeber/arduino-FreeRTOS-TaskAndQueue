#include <Arduino.h>

// Queue Handles
QueueHandle_t inputToProcessQueue;
QueueHandle_t processToOutputQueue;

// Struct für die zwei analogen Werte
struct AnalogValues {
  uint16_t value1;
  uint16_t value2;
};

// Pins (ESP32 ADC1-Pins, z.B. GPIO34 und GPIO35 – sicher für analogRead)
const int analogPin1 = 34;
const int analogPin2 = 35;

// Task-Funktionen
void InputTask(void *pvParameters) {
  AnalogValues values;
  for (;;) {
    values.value1 = analogRead(analogPin1);
    values.value2 = analogRead(analogPin2);

    // Sende Struct in Queue (blockiert bei voller Queue)
    xQueueSend(inputToProcessQueue, &values, portMAX_DELAY);

    // Alle 500ms neu lesen
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void ProcessingTask(void *pvParameters) {
  AnalogValues received;
  uint32_t sum;
  for (;;) {
    // Warte auf Daten vom InputTask
    if (xQueueReceive(inputToProcessQueue, &received, portMAX_DELAY) == pdPASS) {
      sum = received.value1 + received.value2;

      // Sende Summe weiter
      xQueueSend(processToOutputQueue, &sum, portMAX_DELAY);
    }
  }
}

void OutputTask(void *pvParameters) {
  uint32_t sum;
  for (;;) {
    // Warte auf Summe vom ProcessingTask
    if (xQueueReceive(processToOutputQueue, &sum, portMAX_DELAY) == pdPASS) {
      Serial.print("Summe der analogen Werte: ");
      Serial.println(sum);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Queues erstellen (Länge 10, um Puffer zu haben)
  inputToProcessQueue = xQueueCreate(10, sizeof(AnalogValues));
  processToOutputQueue = xQueueCreate(10, sizeof(uint32_t));

  if (inputToProcessQueue == NULL || processToOutputQueue == NULL) {
    Serial.println("Fehler beim Erstellen der Queues!");
    while (1);
  }

  // Tasks erstellen (Stack-Größe in Words, Priorität 1-3)
  xTaskCreate(InputTask, "Input", 2048, NULL, 2, NULL);
  xTaskCreate(ProcessingTask, "Processing", 2048, NULL, 2, NULL);
  xTaskCreate(OutputTask, "Output", 2048, NULL, 1, NULL);

  // Scheduler startet automatisch auf ESP32 – nichts weiter nötig
}

void loop() {
  // Leer – alles läuft in Tasks
}
