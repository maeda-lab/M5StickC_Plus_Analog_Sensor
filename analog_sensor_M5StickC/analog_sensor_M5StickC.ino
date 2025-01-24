#include <M5StickCPlus.h>  // CPlus用のライブラリをインクルード

// グローバル変数
const float expFilterSmoothingFactor = 0.05;
const float scaleFactor = 4095;
float sensorVal = 0;
float displayVal = 0;
int previousBarWidth = 0;
SemaphoreHandle_t xMutex;

// センサー読み取りタスク
void readSensorTask(void *pvParameters) {
  while (true) {
    int sensorValueInt = analogRead(33);
    
    // ローパスフィルタ処理
    float newVal = sensorVal * (1.0 - expFilterSmoothingFactor) + (float)sensorValueInt * expFilterSmoothingFactor;

    // ローバスフィルタ未適用
    // float newVal = (float)sensorValueInt;// * (1.0 - expFilterSmoothingFactor) + (float)sensorValueInt * expFilterSmoothingFactor;
    
    // ミューテックスを取得してセンサー値を更新
    if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
      sensorVal = newVal;
      xSemaphoreGive(xMutex);
    }
    
    delay(1); // 読み取り間隔
  }
}

// 描画タスク
void displayGraphTask(void *pvParameters) {

  while (true) {
    // ミューテックスを取得してsensorValを表示用に取得
    if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
      displayVal = sensorVal;  // センサー値のコピーを取得
      xSemaphoreGive(xMutex);
    }

    // 棒グラフの長さを画面の全幅に合わせて描画
    int barWidth = map(displayVal, 0, 4095, 0, 240); // 横方向240ピクセルにマップ

    // 差分領域のみに限定して前の棒グラフを消去
    if (barWidth < previousBarWidth) {
      // 新しい幅が前回より小さい場合、余剰部分を黒で消去
      M5.Lcd.fillRect(barWidth, 60, previousBarWidth - barWidth, 20, BLACK);
    }

    // 新しい棒グラフを描画
    M5.Lcd.fillRect(0, 60, barWidth, 20, GREEN);

    // 前回の幅を更新
    previousBarWidth = barWidth;

    // sensorValの値を画面下部に描画
    M5.Lcd.setCursor(10, 100);
    M5.Lcd.printf("%4.3f", displayVal / scaleFactor);  // 小数点以下2桁まで表示
    Serial.printf("%4.3f\n", displayVal / scaleFactor);
    
    delay(10); // 描画間隔
  }
}

void setup() {
  M5.begin();
  Serial.begin(115200); // シリアル通信を初期化
  pinMode(33, INPUT);   // GPIO 33 はアナログ入力に対応

  // 初期値
  sensorVal = (float)analogRead(33);

  // ミューテックスを作成
  xMutex = xSemaphoreCreateMutex();
  if (xMutex == NULL) {
    Serial.println("Mutex creation failed.");
    while (1);
  }

  M5.Lcd.setRotation(1); 
  M5.Lcd.fillScreen(BLACK); 
  M5.Lcd.setTextColor(WHITE, BLACK);  // 文字色を白、背景を黒に設定
  
  M5.Lcd.setTextSize(1);   
  M5.Lcd.setCursor(10, 20);  M5.Lcd.printf("Nov 3, 2024 / Masahiro Furukawa"); 
  M5.Lcd.setCursor(10, 40);  M5.Lcd.printf("SmoothFactor %4.2f", expFilterSmoothingFactor); 
  M5.Lcd.setTextSize(3);
     
  // FreeRTOSタスクの作成
  xTaskCreate(readSensorTask, "Read Sensor Task", 1024, NULL, 1, NULL);
  xTaskCreate(displayGraphTask, "Display Graph Task", 2048, NULL, 1, NULL);
}

void loop() {
  // メインループは空で、全ての処理はタスクに任せる
}
