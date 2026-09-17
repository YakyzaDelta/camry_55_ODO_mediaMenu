
#include <SPI.h>
#include <mcp_can.h>
#include "can_frames.h"

// ----------------- НАСТРОЙКИ ПОД ВАШЕ ЖЕЛЕЗО -----------------
#define CS_PIN      15
#define CAN_SPEED   CAN_500KBPS   // <-- сверьте со своим сниффером!
#define MCP_CLOCK   MCP_8MHZ      // <-- сверьте со своим сниффером!
// ---------------------------------------------------------------

MCP_CAN CAN(CS_PIN);

#define LOOP_START_INDEX 0

bool verbose = false;

uint32_t sentOk = 0;
uint32_t sentFail = 0;
uint32_t lastStatsPrint = 0;

const char* errText(byte code) {
  switch (code) {
    case CAN_OK:               return "OK";
    case CAN_FAILINIT:         return "FAILINIT (не удалось инициализировать MCP2515 - проверьте SPI/CS/питание)";
    case CAN_FAILTX:           return "FAILTX (передача не удалась)";
    case CAN_MSGAVAIL:         return "MSGAVAIL";
    case CAN_NOMSG:            return "NOMSG";
    case CAN_CTRLERROR:        return "CTRLERROR (контроллер в состоянии ошибки - см. EFLG/bus-off)";
    case CAN_GETTXBFTIMEOUT:   return "GETTXBFTIMEOUT (все TX-буферы заняты)";
    case CAN_SENDMSGTIMEOUT:   return "SENDMSGTIMEOUT (кадр НЕ подтверждён - нет ACK: неверная скорость, нет терминации 120R, или нет второго живого узла на шине)";
    default:                   return "UNKNOWN";
  }
}

void printFrame(const char* prefix, uint32_t id, uint8_t dlc, uint8_t* data) {
  Serial.print(prefix);
  Serial.print(" ID=0x");
  Serial.print(id, HEX);
  Serial.print(" DLC=");
  Serial.print(dlc);
  Serial.print(" Data=");
  for (int i = 0; i < dlc; i++) {
    if (data[i] < 0x10) Serial.print('0');
    Serial.print(data[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
}

void sendFrame(const CanFrame &f) {
  uint8_t buf[8];
  memcpy_P(buf, f.data, 8);

  byte res = CAN.sendMsgBuf(f.id, 1 /*ext*/, f.dlc, buf);

  if (res == CAN_OK) {
    sentOk++;
  } else {
    sentFail++;
  }

  if (verbose || res != CAN_OK) {
    printFrame(res == CAN_OK ? "[TX OK]  " : "[TX ERR] ", f.id, f.dlc, buf);
    if (res != CAN_OK) {
      Serial.print("         -> ");
      Serial.println(errText(res));
    }
  }
}

void printStatsIfDue() {
  uint32_t now = millis();
  if (now - lastStatsPrint >= 2000) {
    lastStatsPrint = now;
    Serial.print("[STATS] sentOK=");
    Serial.print(sentOk);
    Serial.print(" sentFAIL=");
    Serial.print(sentFail);
    if (sentFail > 0 && sentOk == 0) {
      Serial.print("  <-- ни один кадр не подтверждён! Проверьте: скорость шины, "
                    "терминаторы 120R, запитана ли и включена ли настоящая приборка, "
                    "полярность CAN-H/CAN-L.");
    }
    Serial.println();
  }
}

void sniffMode() {
  Serial.println(">>> SNIFF режим: слушаю шину 5 секунд, ничего не передаю...");
  unsigned long start = millis();
  uint32_t rxCount = 0;
  while (millis() - start < 5000) {
    if (CAN.checkReceive() == CAN_MSGAVAIL) {
      long unsigned int rxId;
      uint8_t len;
      uint8_t buf[8];
      CAN.readMsgBuf(&rxId, &len, buf);
      rxCount++;
      printFrame("[RX]", rxId, len, buf);
    }
  }
  Serial.print(">>> SNIFF завершён. Принято кадров: ");
  Serial.println(rxCount);
  if (rxCount == 0) {
    Serial.println(">>> ВНИМАНИЕ: за 5 секунд не принято НИ ОДНОГО кадра.");
    Serial.println("    Это значит, что либо неверная скорость (CAN_SPEED),");
    Serial.println("    либо неверная проводка CAN-H/CAN-L, либо шина мертва");
    Serial.println("    (приборка не запитана). Сначала добейтесь, чтобы SNIFF");
    Serial.println("    видел трафик (как в вашем Silent Listener) - и только");
    Serial.println("    потом имеет смысл пытаться передавать.");
  } else {
    Serial.println(">>> Трафик виден - скорость и проводка в порядке.");
    Serial.println("    Если передача (TX) всё равно не проходит (SENDMSGTIMEOUT),");
    Serial.println("    дело в отсутствии ACK: либо нет терминации 120R, либо");
    Serial.println("    приборка не отвечает на эти ID (например, ждёт другой");
    Serial.println("    порядок/набор кадров рукопожатия перед тем как их принять).");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("=== HU emulator (debug build) ===");
  Serial.print("CS_PIN=");
  Serial.println(CS_PIN);
  Serial.print("Кадров в таблице HU_FRAMES: ");
  Serial.println(HU_FRAME_COUNT);

  byte res;
  uint8_t attempts = 0;
  do {
    res = CAN.begin(MCP_ANY, CAN_SPEED, MCP_CLOCK);
    if (res != CAN_OK) {
      attempts++;
      Serial.print("MCP2515 init FAIL, код=");
      Serial.print(res);
      Serial.print(" (");
      Serial.print(errText(res));
      Serial.println("), повтор через 500мс...");
      Serial.println("  Если это повторяется бесконечно - проверьте: провода SPI "
                      "(SCK/MOSI/MISO/CS), питание 5В/3.3В на модуле MCP2515, "
                      "и что CS_PIN не занят чем-то ещё.");
      delay(500);
    }
  } while (res != CAN_OK && attempts < 20);

  if (res != CAN_OK) {
    Serial.println("!!! MCP2515 так и не инициализировался за 20 попыток. "
                    "Дальше эмулятор работать не будет, проверьте железо.");
    while (true) delay(1000);
  }

  CAN.setMode(MCP_NORMAL);
  Serial.println("MCP2515 инициализирован, режим NORMAL.");
  Serial.println();
  Serial.println("Команды в Serial-мониторе:");
  Serial.println("  s - запустить SNIFF (5 сек слушать шину, ничего не слать)");
  Serial.println("  v - включить/выключить подробный вывод каждого TX-кадра");
  Serial.println();
  Serial.println("Начинаю (по умолчанию) сразу с SNIFF, чтобы проверить, что шина живая...");
  sniffMode();
  Serial.println("Запускаю передачу таблицы HU_FRAMES...");
}

void loop() {
  static uint32_t idx = 0;

  if (Serial.available()) {
    char c = Serial.read();
    if (c == 's') sniffMode();
    if (c == 'v') {
      verbose = !verbose;
      Serial.print("verbose = ");
      Serial.println(verbose ? "ON" : "OFF");
    }
  }

  CanFrame f;
  memcpy_P(&f, &HU_FRAMES[idx], sizeof(CanFrame));

  if (f.dt_ms > 0) {
    delay(f.dt_ms);
  }
  sendFrame(f);
  printStatsIfDue();

  idx++;
  if (idx >= HU_FRAME_COUNT) {
    idx = LOOP_START_INDEX;
    Serial.println("[LOOP] Таблица кадров закончилась, начинаю заново.");
  }
}

/* ============================================================
   Отправка своих метаданных "сейчас играет" в найденном формате.
   (без изменений по логике, только теперь тоже с проверкой результата)
   ============================================================ */
void sendMetadataText(const char *text) {
  size_t len = strlen(text);

  uint8_t semaphoreOn[1]  = { 0xAA };
  uint8_t semaphoreOff[1] = { 0x00 };

  uint8_t header[8] = { 0x7C, 0x91, 0x93, 0x00, (uint8_t)(len * 2), 0x0D, 0x00, 0x00 };
  byte r = CAN.sendMsgBuf(0x1F240016, 1, 8, header);
  Serial.print("header -> "); Serial.println(errText(r));
  CAN.sendMsgBuf(0xC00161, 1, 1, semaphoreOn);

  uint8_t sub = 1;
  size_t i = 0;
  while (i < len) {
    uint8_t payload[8] = {0,0,0,0,0,0,0,0};
    for (int k = 0; k < 4 && i < len; k++, i++) {
      payload[k * 2]     = (uint8_t)text[i];
      payload[k * 2 + 1] = 0x00;
    }
    uint32_t id = 0x1F240016 | (sub << 8);
    r = CAN.sendMsgBuf(id, 1, 8, payload);
    Serial.print("sub "); Serial.print(sub); Serial.print(" -> "); Serial.println(errText(r));
    sub++;
    delay(5);
  }

  uint8_t term[8] = {0,0,0,0,0,0,0,0};
  CAN.sendMsgBuf(0x1F24FF16, 1, 8, term);
  CAN.sendMsgBuf(0xC00161, 1, 1, semaphoreOff);
}
