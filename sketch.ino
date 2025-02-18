#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TM1637Display.h>
#include "pitches.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define  SPEAKER_PIN 10
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUTTON_UP_PIN 2
#define BUTTON_DOWN_PIN 3
#define BUTTON_SELECT_PIN 4
#define POT_PIN A1
#define CLK_PIN 6
#define DIO_PIN 5
#define LED_LIFE1 11
#define LED_LIFE2 12
#define LED_LIFE3 13
#define LDR_PIN A0
TM1637Display displaySegment(CLK_PIN, DIO_PIN);

bool isGameActive = false;
int selectedOption = 0;
int score = 0;
int lives = 3;
float rewardPosX, rewardPosY;
bool isRewardFalling = false;

int paddleWidth = 30;
int paddleHeight = 5;
int ballSize = 4;
int level = 1;
float ballPosX, ballPosY;
float ballVelX, ballVelY;
int paddlePosX;
uint16_t textColor = SSD1306_WHITE;
uint16_t backgroundColor = SSD1306_BLACK;

const int bricksPerRow = 8;
const int brickRows = 3;
const int brickWidth = SCREEN_WIDTH / bricksPerRow;
const int brickHeight = 8;
bool bricks[brickRows][bricksPerRow];
bool rewards[brickRows][bricksPerRow];

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP);
  pinMode(LED_LIFE1, OUTPUT);
  pinMode(LED_LIFE2, OUTPUT);
  pinMode(LED_LIFE3, OUTPUT);

  pinMode(LDR_PIN, INPUT);
  pinMode(SPEAKER_PIN, OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.display();
  delay(2000);
  display.clearDisplay();

  displaySegment.setBrightness(0x0f);
  displaySegment.clear();

  showMainMenu();
}

void loop() {
  static bool mainMenuSelected = false; // Ana menü seçimi yapıldı mı?
  
  if (!mainMenuSelected) { // Ana menü seçimi yapılmadıysa
    if (!isGameActive) {
      handleMenuNavigation();
    } else {
      updateGame();
      delay(10);
    }
  }

  if (isGameActive || mainMenuSelected) { // Oyun aktif veya ana menü seçimi yapıldıysa
    if (digitalRead(BUTTON_SELECT_PIN) == LOW && !isGameActive) {
      mainMenuSelected = true; // Ana menü seçimini yeniden başlat
    }
    return; // Ana menü seçimi yapıldığı veya oyun aktif olduğu için döngüyü sonlandır
  }
}

void showMainMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  updateDisplayColors();
  
  // Arka plan rengi ayarı
  if (backgroundColor == SSD1306_WHITE) {
    display.fillRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
  }

  
  // Başlıktan sonra başlamak için yükseklik
  int optionsYStart = 20;
  
  // Seçenekleri ortala
  int optionWidth = display.width(); // Ekran genişliği
  int optionX = (optionWidth - 6 * 6) / 4; // 6 karakter genişliği varsayımı
  display.setTextSize(1); // Normal metin boyutuna dönün
  display.setCursor(optionX, optionsYStart);
  display.println(selectedOption == 0 ? "-> Baslat" : "   Baslat");
  
  optionX = (optionWidth - 5 * 6) / 4; // 5 karakter genişliği varsayımı
  display.setCursor(optionX, optionsYStart + 8); // 8 piksel boşluk bırak
  display.println(selectedOption == 1 ? "-> Cikis" : "   Cikis");
  
  display.display(); // Ekranı güncelleyin
}

void handleMenuNavigation() {
  if (digitalRead(BUTTON_UP_PIN) == LOW || digitalRead(BUTTON_DOWN_PIN) == LOW) {
    selectedOption = 1 - selectedOption;
    showMainMenu();
    delay(200); // Buton gecikmesi
  }
  updateDisplayColors();
  if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
    if (selectedOption == 0) {
      initializeBricks();
      initializeGame();
      updateLivesDisplay();
      isGameActive = true;
    } else {
      display.clearDisplay();
      display.setTextSize(1);
      if (backgroundColor == SSD1306_WHITE) {
        display.fillRect(0, 0, display.width(), display.height(), SSD1306_WHITE); // Ekranın tamamını kaplayan bir beyaz dikdörtgen çizin
      }
      display.setCursor(0, 0);
      display.println("Oyunumuza gosterdiginiz ilgi icin tesekkurler");
      display.display();
      delay(2000);
      showMainMenu();
    }
  }
}

void showEndScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  updateDisplayColors();
  if (backgroundColor == SSD1306_WHITE) {
    display.fillRect(0, 0, display.width(), display.height(), SSD1306_WHITE); // Ekranın tamamını kaplayan bir beyaz dikdörtgen çizin
  }
  display.setCursor(0, 0);
  display.println("Oyun Bitti!");
  display.print("Skor: ");
  display.println(score);
  display.display();
  delay(2000); // 2 saniye beklet
}

void initializeGame() {
  score = 0;
  lives = 3;
  ballPosX = SCREEN_WIDTH / 2;
  ballPosY = SCREEN_HEIGHT - paddleHeight - 10;
  ballVelX = 2;
  ballVelY = -2;
  initializeBricks();
  display.clearDisplay();
  displaySegment.showNumberDec(score);
}

void updateGame() {
  paddlePosX = map(analogRead(POT_PIN), 0, 1023, 0, SCREEN_WIDTH - paddleWidth);

  float nextBallPosX = ballPosX + ballVelX;
  float nextBallPosY = ballPosY + ballVelY;

  if (nextBallPosX <= 0 || nextBallPosX >= SCREEN_WIDTH - ballSize) {
    ballVelX *= -1;
    nextBallPosX = max(0, min(nextBallPosX, SCREEN_WIDTH - ballSize));
  }

  if (nextBallPosY <= 0) {
    ballVelY *= -1;
    nextBallPosY = 0;
  } else if (nextBallPosY >= SCREEN_HEIGHT - ballSize &&
             !(nextBallPosX >= paddlePosX && nextBallPosX <= paddlePosX + paddleWidth &&
               nextBallPosY >= SCREEN_HEIGHT - paddleHeight - ballSize)) {
    lives--;
    updateLivesDisplay();
    if (lives <= 0) {
      isGameActive = false;
       // Play a Wah-Wah-Wah-Wah sound
      tone(SPEAKER_PIN, NOTE_DS5);
      delay(300);
      tone(SPEAKER_PIN, NOTE_D5);
      delay(300);
      tone(SPEAKER_PIN, NOTE_CS5);
      delay(300);
      for (byte i = 0; i < 10; i++) {
        for (int pitch = -10; pitch <= 10; pitch++) {
          tone(SPEAKER_PIN, NOTE_C5 + pitch);
          delay(5);
        }
      }
      noTone(SPEAKER_PIN);
      delay(500);
      showEndScreen();
      delay(2500); // 3 saniye beklet
      showMainMenu(); // Ana menüye dön

      return;
    } else {
      ballPosX = SCREEN_WIDTH / 2;
      ballPosY = SCREEN_HEIGHT - paddleHeight - ballSize - 10;
      ballVelY = -ballVelY;
      return;
    }
  }

  if (nextBallPosY >= SCREEN_HEIGHT - paddleHeight - ballSize &&
      nextBallPosX >= paddlePosX && nextBallPosX <= paddlePosX + paddleWidth) {
    ballVelY *= -1;
    nextBallPosY = SCREEN_HEIGHT - paddleHeight - ballSize - 1;
  }

  ballPosX = nextBallPosX;
  ballPosY = nextBallPosY;

  bool allBricksDestroyed = true; // Tüm tuğlalar yok edildi mi?

  for (int i = 0; i < brickRows; i++) {
    for (int j = 0; j < bricksPerRow; j++) {
      if (bricks[i][j]) {
        allBricksDestroyed = false; // Henüz tüm tuğlalar yok edilmedi
        int brickX = j * brickWidth;
        int brickY = i * brickHeight;
        if (ballPosX + ballSize > brickX && ballPosX < brickX + brickWidth &&
            ballPosY + ballSize > brickY && ballPosY < brickY + brickHeight) {
          bricks[i][j] = false;
          ballVelY *= -1;
          score++;
          displaySegment.showNumberDec(score, false);
          if (rewards[i][j]) {
            rewardPosX = brickX + brickWidth / 2;
            rewardPosY = brickY + brickHeight;
            isRewardFalling = true;
          }
        }
      }
    }
  }

  if (allBricksDestroyed) { // Tüm tuğlalar yok edildi ise
    level++;
    dashboard(level);
    playLevelUpSound();
    delay(1000);
    initializeBricks(); // Yeni seviyeye geçmek için tuğlaları yeniden başlat
    ballVelX *= 1.2; // Topun hızını %20 arttır
    ballVelY = abs(ballVelY) * 1.2;
    return;
  } 

  if (!isRewardFalling) {
    for (int i = 0; i < brickRows; i++) {
      for (int j = 0; j < bricksPerRow; j++) {
        if (bricks[i][j]) {
          int brickX = j * brickWidth;
          int brickY = i * brickHeight;
          if (ballPosX + ballSize > brickX && ballPosX < brickX + brickWidth &&
              ballPosY + ballSize > brickY && ballPosY < brickY + brickHeight) {
            bricks[i][j] = false;
            ballVelY *= -1;
            score++;
            displaySegment.showNumberDec(score, false);
            if (rewards[i][j]) {
              rewardPosX = brickX + brickWidth / 2;
              rewardPosY = brickY + brickHeight;
              isRewardFalling = true;
            }
          }
        }
      }
    }
  } else {
    rewardPosY += 1;
    if (rewardPosX >= paddlePosX && rewardPosX <= paddlePosX + paddleWidth &&
        rewardPosY >= SCREEN_HEIGHT - paddleHeight && rewardPosY <= SCREEN_HEIGHT) {
      isRewardFalling = false;
      if (lives < 3) {
        lives++;
        updateLivesDisplay();
      }
    } else if (rewardPosY > SCREEN_HEIGHT) {
      isRewardFalling = false;
    }
  }

  drawGame();
  
}

void drawGame() {
  updateDisplayColors(); // Ekran rengini güncelle
  display.clearDisplay();
  
  // Işık seviyesine göre arka planı ayarla
  if (backgroundColor == SSD1306_WHITE) {
    display.fillRect(0, 0, display.width(), display.height(), SSD1306_WHITE); // Ekranın tamamını kaplayan bir beyaz dikdörtgen çizin
  }

  display.drawRect(paddlePosX, SCREEN_HEIGHT - paddleHeight, paddleWidth, paddleHeight, textColor);
  display.fillCircle(ballPosX, ballPosY, ballSize / 2, textColor);
  for (int i = 0; i < brickRows; i++) {
    for (int j = 0; j < bricksPerRow; j++) {
      if (bricks[i][j]) {
        display.fillRect(j * brickWidth, i * brickHeight, brickWidth - 1, brickHeight - 1, textColor);
      }
    }
  }
  if (isRewardFalling) {
    drawHeart(rewardPosX, rewardPosY);
  }
  display.display();
}

void initializeBricks() {
  // Tüm tuğlaları başlangıçta dolu olarak işaretle
  for (int i = 0; i < brickRows; i++) {
    for (int j = 0; j < bricksPerRow; j++) {
      bricks[i][j] = true;
      rewards[i][j] = false;
    }
  }

  // Seviyeye bağlı olarak belirli sayıda boşluk oluştur
  int gapCount = level * 2; // Örneğin, seviye sayısı ile çarpılmış bir şekilde daha fazla boşluk oluşturabiliriz
  while (gapCount > 0) {
    int row = random(brickRows);
    int col = random(bricksPerRow);
    // Eğer bu tuğla zaten boş değilse, boş olarak işaretle ve gapCount'u azalt
    if (bricks[row][col]) {
      bricks[row][col] = false;
      gapCount--;
    }
  }

  // Ödül tuğlalarını yerleştir
  int rewardsCount = 2; // Ödül tuğlası sayısı
  while (rewardsCount > 0) {
    int row = random(brickRows);
    int col = random(bricksPerRow);
    // Eğer tuğla dolu ve ödül yoksa, ödülü yerleştir
    if (bricks[row][col] && !rewards[row][col]) {
      rewards[row][col] = true;
      rewardsCount--;
    }
  }

  // Topun başlangıç pozisyonunu ayarla
  ballPosX = SCREEN_WIDTH / 2;
  ballPosY = SCREEN_HEIGHT - paddleHeight - 10; // abs kullanımına gerek yok, çünkü SCREEN_HEIGHT zaten pozitif bir değer
}

void updateLivesDisplay() {
  digitalWrite(LED_LIFE1, lives >= 1 ? HIGH : LOW);
  digitalWrite(LED_LIFE2, lives >= 2 ? HIGH : LOW);
  digitalWrite(LED_LIFE3, lives >= 3 ? HIGH : LOW);
}

void drawHeart(float x, float y) {
  int size = 3;
  int radius = size / 2;

  display.fillCircle(x - radius, y - radius, radius, textColor);
  display.fillCircle(x + radius, y - radius, radius, textColor);
  display.fillTriangle(x - size, y - radius, x + size, y - radius, x, y + size, textColor);
}

void updateDisplayColors() {
  int lightValue = analogRead(LDR_PIN);
  if (lightValue > 512) { // Sensör değerine göre ayar yapın, bu değer donanımınıza göre değişebilir
    textColor = SSD1306_BLACK;
    display.setTextColor(SSD1306_BLACK);
    backgroundColor = SSD1306_WHITE;
  } else {
    textColor = SSD1306_WHITE;
    display.setTextColor(SSD1306_WHITE);
    backgroundColor = SSD1306_BLACK;
  }
}

void dashboard(int level){
  display.clearDisplay();
  display.setTextSize(2);
  updateDisplayColors();
    if (backgroundColor == SSD1306_WHITE) {
    display.fillRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
  }

  // Başlıktan sonra başlamak için yükseklik
  int optionsYStart = 20;
  
  // Seçenekleri ortala
  int optionWidth = display.width(); // Ekran genişliği
  int optionX = (optionWidth - 6 * 6) / 4; // 6 karakter genişliği varsayımı
  display.setTextSize(2); 
  display.setCursor(optionX, optionsYStart);
  display.print("Level ");
  display.println(level);
  display.display(); // Ekranı güncelleyin

}

void playLevelUpSound() {
  tone(SPEAKER_PIN, NOTE_E4);
  delay(150);
  tone(SPEAKER_PIN, NOTE_G4);
  delay(150);
  tone(SPEAKER_PIN, NOTE_E5);
  delay(150);
  tone(SPEAKER_PIN, NOTE_C5);
  delay(150);
  tone(SPEAKER_PIN, NOTE_D5);
  delay(150);
  tone(SPEAKER_PIN, NOTE_G5);
  delay(150);
  noTone(SPEAKER_PIN);
}