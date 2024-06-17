#include <LiquidCrystal_I2C.h>  // Biblioteca para LCD I2C
#include <RTClib.h> // Biblioteca para Relógio em Tempo Real
#include <Wire.h>   // Biblioteca para comunicação I2C
#include <EEPROM.h> // Biblioteca para EEPROM
#include <stdlib.h>

#define PIRPIN 2 // Pino onde o sensor PIR está conectado
#define LOG_OPTION 1 // Opção para ativar a leitura do log
#define SERIAL_OPTION 1 // Opção de comunicação serial: 0 para desligado, 1 para ligado
#define UTC_OFFSET 0 // Ajuste de fuso horário para UTC-3

// Configuração do Display I2C (endereço 0x27 pode variar, verifique o seu display)
LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço do LCD I2C, colunas e linhas

// Configuração do RTC
RTC_DS1307 rtc;

// Marca o tempo de início
unsigned long startTime = millis(); 

// Variável da contagem de voltas
int voltaAtual = 0; 
int buttonState = LOW;      // Estado atual do pino do sensor
int lastButtonState = LOW;  // Estado anterior do pino do sensor

// Variáveis para debounce
unsigned long lastDebounceTime = 0; 
unsigned long debounceDelay = 50; 

// Lista de carros e siglas
const char* carros[] = {"Mahindra", "Porsche", "Jaguar", "Nissan"};
const char* siglas[] = {"MH", "PS", "JG", "NS"};
int voltas[4]; // Contador de voltas para cada carro

// Lista temporária de carros para evitar repetições
int carrosDisponiveis[4];
int numCarrosDisponiveis = 4;

// Endereços de memória EEPROM para armazenar os contadores de voltas
int eepromAddresses[] = {0, sizeof(int), 2*sizeof(int), 3*sizeof(int)};

// Função para inicializar a EEPROM
void inicializarEEPROM() {
  for (int i = 0; i < 4; i++) {
    int valor;
    EEPROM.get(eepromAddresses[i], valor);
    if (valor < 0 || valor > 1000) { // Verifica se o valor lido não é válido
      EEPROM.put(eepromAddresses[i], 0);
      voltas[i] = 0;
    } else {
      voltas[i] = valor;
    }
  }
}

void resetarCarrosDisponiveis() {
  for (int i = 0; i < 4; i++) {
    carrosDisponiveis[i] = i;
  }
  numCarrosDisponiveis = 4;
}

void setup() {
  Serial.begin(9600); 

  lcd.init(); // Inicializa o LCD
  lcd.backlight(); // Liga a luz de fundo do LCD

  while(millis() - startTime < 5000){
    lcd.setCursor(0, 0); 
    lcd.print("INICIANDO"); 
    delay(2000); 

    if (!rtc.begin()) {
      lcd.setCursor(0, 0); 
      lcd.print("RTC nao iniciado"); 
      while(1); 
    } else {
      lcd.setCursor(0, 1); 
      lcd.print("RTC iniciado"); 
      delay(2000); 
    }
  }
  pinMode(PIRPIN, INPUT); 
  resetarCarrosDisponiveis(); // Inicializa a lista de carros disponíveis
  
  // Inicializa a EEPROM e carrega os contadores de voltas
  inicializarEEPROM();

  // Mensagem de depuração para verificar os valores iniciais carregados da EEPROM
  for (int i = 0; i < 4; i++) {
    Serial.print("Carro: ");
    Serial.print(carros[i]);
    Serial.print(", Voltas iniciais: ");
    Serial.println(voltas[i]);
  }
}

void loop() {
  int reading = digitalRead(PIRPIN);
  unsigned long currentTime = millis();

  DateTime now = rtc.now();
  
  // Calculando o deslocamento do fuso horário
  int offsetSeconds = UTC_OFFSET * 3600; // Convertendo horas para segundos
  now = now.unixtime() + offsetSeconds; // Adicionando o deslocamento ao tempo atual

  DateTime adjustedTime = DateTime(now); 

  // Se o estado do sensor mudou
  if (reading != lastButtonState) {
    lastDebounceTime = currentTime;
  }

  // Se o tempo de debounce passou
  if ((currentTime - lastDebounceTime) > debounceDelay) {
    // Se o estado mudou
    if (reading != buttonState) {
      buttonState = reading;

      // Apenas conta o sensor ativado (LOW to HIGH)
      if (buttonState == HIGH) {
        if (numCarrosDisponiveis == 0) {
          resetarCarrosDisponiveis();
        }

        // Escolhe um carro aleatoriamente da lista de carros disponíveis
        int indiceEscolhido = rand() % numCarrosDisponiveis;
        int carroIndice = carrosDisponiveis[indiceEscolhido];
        
        // Remove o carro escolhido da lista de carros disponíveis
        for (int i = indiceEscolhido; i < numCarrosDisponiveis - 1; i++) {
          carrosDisponiveis[i] = carrosDisponiveis[i + 1];
        }
        numCarrosDisponiveis--;

        const char* carroAtual = carros[carroIndice];
        const char* carroAtualSigla = siglas[carroIndice];
        voltas[carroIndice]++;
        voltaAtual = voltas[carroIndice];

        // Armazena o contador de voltas na EEPROM
        EEPROM.put(eepromAddresses[carroIndice], voltas[carroIndice]);

        lcd.clear(); // Limpa o display antes de exibir novas informações
        lcd.setCursor(0, 0); 
        lcd.print("Volta: "); 
        lcd.print(voltaAtual); 
        lcd.print(" "); 
        lcd.print(carroAtualSigla);

        lcd.setCursor(0, 1); 
        lcd.print(adjustedTime.hour() < 10 ? "0" : ""); 
        lcd.print(adjustedTime.hour()); 
        lcd.print(":"); 
        lcd.print(adjustedTime.minute() < 10 ? "0" : ""); 
        lcd.print(adjustedTime.minute()); 
        lcd.print(":"); 
        lcd.print(adjustedTime.second() < 10 ? "0" : ""); 
        lcd.print(adjustedTime.second()); 

        // Mensagem de depuração com horário
        Serial.print("Volta detectada: ");
        Serial.print(voltaAtual);
        Serial.print(" - Carro: ");
        Serial.print(carroAtual);
        Serial.print(" - Hora: ");
        Serial.print(adjustedTime.hour() < 10 ? "0" : "");
        Serial.print(adjustedTime.hour());
        Serial.print(":");
        Serial.print(adjustedTime.minute() < 10 ? "0" : "");
        Serial.print(adjustedTime.minute());
        Serial.print(":");
        Serial.print(adjustedTime.second() < 10 ? "0" : "");
        Serial.println(adjustedTime.second());
      }
    }
  }

  // Atualiza o estado anterior
  lastButtonState = reading;
}
