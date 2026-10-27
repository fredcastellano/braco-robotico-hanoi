#include <Servo.h>

// ==== SERVOS ====
Servo baseServo;      // gira esquerda / centro / direita
Servo ombroServo;     // sobe/desce braço principal
Servo cotoveloServo;  // dobra o braço
Servo garraServo;     // abre/fecha garra

// ==== PINOS ====
const int PIN_BASE = 10;
const int PIN_OMBRO = 8;
const int PIN_COTOVELO = 9;
const int PIN_GARRA = 12;

// ==== CONFIG GARRA ====
const int GARRA_ABERTA = 0;
const int GARRA_FECHADA = 45;

// ==== CONFIGURAÇÕES DE MOVIMENTO ====
const int STEP_DELAY = 15;   // ms entre passos (movimento suave)
const int STEP_SIZE = 1;     // graus por passo
const int MOVE_DELAY = 300;  // ms após movimentos principais

// ==== POSIÇÕES CALIBRADAS ====
struct Pose {
  int base;
  int ombro;
  int cotovelo;
};

// Haste A (esquerda)
Pose A_UP = {168, 130, 125};  // base - altura - frente
Pose A_LOW_BIG = {168, 43, 180};
Pose A_LOW_SMALL = {168, 70, 170};

// Haste B (meio)
Pose B_UP = {98, 130, 172};
Pose B_LOW_BIG = {98, 61, 180};
Pose B_LOW_SMALL = {98, 60, 180};

// Haste C (direita)
Pose C_UP = {15, 130, 137};
Pose C_LOW_BIG = {15, 90, 131};
Pose C_LOW_SMALL = {15, 80, 137};

// ==== VARIÁVEIS DE POSIÇÃO ATUAL ====
int basePos = 90;
int ombroPos = 80;
int cotoveloPos = 100;
int garraPos = GARRA_ABERTA;

// ==== FUNÇÕES OTIMIZADAS ====

// Move servo suavemente mantendo controle de posição
void moveServoSmoothTracked(Servo &s, int target, int &shadowVar) {
  while (shadowVar != target) {
    if (shadowVar < target) shadowVar += STEP_SIZE;
    else if (shadowVar > target) shadowVar -= STEP_SIZE;
    s.write(shadowVar);
    delay(STEP_DELAY);
  }
}

// Configuração imediata de servo
void setServoImmediate(Servo &s, int angle, int &shadowVar) {
  shadowVar = angle;
  s.write(angle);
}

// Movimento otimizado para pose - versão corrigida
void gotoPose(Pose p) {
  // Se já está na pose, não faz nada
  if (basePos == p.base && ombroPos == p.ombro && cotoveloPos == p.cotovelo) {
    return;
  }

  // Movimento seguro: ajusta altura antes de girar apenas se necessário
  int baseDiff = abs(basePos - p.base);

  // Sobe antes de girar apenas se:
  // 1. Precisa girar bastante E
  // 2. Não está já em uma posição segura E
  // 3. Não está indo para uma posição baixa
  if (baseDiff > 30 &&
      (ombroPos < 70 || cotoveloPos < 110) &&
      p.ombro > 70) {  // Só levanta se o destino não for baixo
    // Levanta para posição segura antes de girar
    moveServoSmoothTracked(ombroServo, 80, ombroPos);
    moveServoSmoothTracked(cotoveloServo, 120, cotoveloPos);
    delay(100);
  }

  // Executa movimentos
  moveServoSmoothTracked(baseServo, p.base, basePos);
  moveServoSmoothTracked(ombroServo, p.ombro, ombroPos);
  moveServoSmoothTracked(cotoveloServo, p.cotovelo, cotoveloPos);
  delay(MOVE_DELAY);
}

void openGripper() {
  Serial.println("Abrindo garra...");
  moveServoSmoothTracked(garraServo, GARRA_ABERTA, garraPos);
  delay(MOVE_DELAY);
}

void closeGripper() {
  Serial.println("Fechando garra...");
  moveServoSmoothTracked(garraServo, GARRA_FECHADA, garraPos);
  delay(MOVE_DELAY);
}

// Função unificada para pegar disco
void pickFrom(Pose upPose, Pose lowPose) {
  Serial.println("Pegando disco...");
  gotoPose(upPose);
  gotoPose(lowPose);
  closeGripper();
  delay(500);  // Delay extra para garantir que fechou
  gotoPose(upPose);
}

// Função unificada para colocar disco
void placeOn(Pose upPose, Pose lowPose) {
  Serial.println("Colocando disco...");
  gotoPose(upPose);
  gotoPose(lowPose);
  openGripper();
  delay(500);  // Delay extra para garantir que abriu
  gotoPose(upPose);
}

// ==== ALGORITMO TORRE DE HANÓI ====

// Função auxiliar para determinar pose baseada no tamanho do disco e haste
Pose getPose(char haste, int tamanhoDisco, bool isUp) {
  if (isUp) {
    switch (haste) {
      case 'A': return A_UP;
      case 'B': return B_UP;
      case 'C': return C_UP;
    }
  } else {
    switch (haste) {
      case 'A':
        return (tamanhoDisco == 1) ? A_LOW_BIG : A_LOW_SMALL;
      case 'B':
        return (tamanhoDisco == 1) ? B_LOW_BIG : B_LOW_SMALL;
      case 'C':
        return (tamanhoDisco == 1) ? C_LOW_BIG : C_LOW_SMALL;
    }
  }
  return A_UP;  // fallback
}

// Move disco entre hastes
void moveDisco(char origem, char destino, int tamanhoDisco) {
  Serial.print("Movendo disco ");
  Serial.print(tamanhoDisco);
  Serial.print(" de ");
  Serial.print(origem);
  Serial.print(" para ");
  Serial.println(destino);

  Pose upOrigem = getPose(origem, tamanhoDisco, true);
  Pose lowOrigem = getPose(origem, tamanhoDisco, false);
  Pose upDestino = getPose(destino, tamanhoDisco, true);
  Pose lowDestino = getPose(destino, tamanhoDisco, false);

  pickFrom(upOrigem, lowOrigem);
  placeOn(upDestino, lowDestino);
}

// Versão específica para 2 discos
void hanoi2() {
  Serial.println("Iniciando Torre de Hanoi com 2 discos");
  moveDisco('A', 'B', 2);  // Pequeno A->B
  moveDisco('A', 'C', 1);  // Grande A->C
  moveDisco('B', 'C', 2);  // Pequeno B->C
  Serial.println("Torre de Hanoi concluída!");
}

// ==== SETUP E LOOP ====
void setup() {
  Serial.begin(9600);
  Serial.println("Inicializando braço robótico...");

  // Inicializa servos
  baseServo.attach(PIN_BASE);
  ombroServo.attach(PIN_OMBRO);
  cotoveloServo.attach(PIN_COTOVELO);
  garraServo.attach(PIN_GARRA);

  // Posição inicial segura
  basePos = 90;
  ombroPos = 80;
  cotoveloPos = 100;
  garraPos = GARRA_ABERTA;

  setServoImmediate(baseServo, basePos, basePos);
  setServoImmediate(ombroServo, ombroPos, ombroPos);
  setServoImmediate(cotoveloServo, cotoveloPos, cotoveloPos);
  setServoImmediate(garraServo, garraPos, garraPos);

  Serial.println("Posição inicial definida");
  delay(2000);  // Aguarda inicialização

  // Executa Torre de Hanói para 2 discos
  hanoi2();
}

void loop() {
  // Para executar repetidamente, descomente:
  // delay(5000);
  // hanoi2();
}
