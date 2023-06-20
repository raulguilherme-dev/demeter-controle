#include <ESP8266WiFi.h> 
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>
#include <ArduinoJson.h>

int fluxo;
String resposta_req;
String resposta_get;
double calculoVazao; // 
double fluxoAtual;
float fluxoAcumulado = 0;
double litro;

const char* ssid = "[Nome da Rede]";
const char* password = "[Senha da Rede]";

double totalLitro(int fluxo) {
  calculoVazao = (fluxo * 2.25);
  fluxoAcumulado = (calculoVazao / 1000);
  return fluxoAcumulado;
}

void setup(void) {
  Serial.begin(9600);
  delay(10);
  Serial.println('\n');

  pinMode(14, INPUT);
  pinMode(16, OUTPUT);
  digitalWrite(16, HIGH);
  attachInterrupt(digitalPinToInterrupt(14), incInpulso, RISING);

  Serial.printf("Connecting to %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  
}

void loop() {

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;    //Declare object of class HTTPClient

    resposta_req = httpGETRequest("[ENDPOINT]");
    JSONVar objeto = JSON.parse(resposta_req);
    
    int last_req = objeto["last_req"];

    resposta_get = httpGETRequest("[ENDPOINT]");
    JSONVar obj = JSON.parse(resposta_get);

    int nova_req = obj["id_req"];
    String jString = objeto;

    Serial.print("AGUARDANDO NOVA REQUISIÇÃO.");
    while (nova_req <= last_req) {
      resposta_get = httpGETRequest("[ENDPOINT]");
      JSONVar obj = JSON.parse(resposta_get);

      nova_req = obj["id_valor"];
      Serial.println(nova_req);
      Serial.println(last_req);
      delay(10000);
    }
    fluxo = 0;

    double valor_recebido = obj["valor"];
    Serial.print("Valor Recebido: ");
    Serial.println(valor_recebido);
    digitalWrite(16, LOW);

    while (totalLitro(fluxo) < valor_recebido) {
      Serial.print(totalLitro(fluxo));
      Serial.print(" -> ");
      Serial.println(valor_recebido);
    }
    
    digitalWrite(16, HIGH);
    fluxo = 0;

    litro = totalLitro(fluxo);
    
    StaticJsonDocument<200> doc;
    doc["last_req"] = nova_req;
  
    String requestBody1;
    serializeJson(doc, requestBody1);
    Serial.println(requestBody1);

    httpPOSTRequestJSON("[ENDPOINT]", requestBody1);

    doc["total"] = valor_recebido;
  
    String requestBody2;
    serializeJson(doc, requestBody2);

    httpPOSTRequestJSON("[ENDPOINT]", requestBody2);
 
  } else {
    Serial.println("Desconectado");
  }
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your IP address with path or Domain name with URL path 
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void httpPOSTRequestJSON(String s, String r) {

  WiFiClient client;
  HTTPClient http;
  
  String site = s;
  String requestBody = r;
  Serial.println(requestBody);

  Serial.print("[HTTP] begin...\n");
  // configure traged server and url
  http.begin(client, site); //HTTP
  http.addHeader("Content-Type", "application/json");

  Serial.print("[HTTP] POST...\n");
  // start connection and send HTTP header and body
  int httpCode = http.POST(requestBody);

  // httpCode will be negative on error
  do { 
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        const String& payload = http.getString();
        Serial.println("received payload:\n<<");
        Serial.println(payload);
        Serial.println(">>");
      }
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
      delay(30000);
    }
  } while (httpCode != 200);
}
 
ICACHE_RAM_ATTR void incInpulso() {
  fluxo++;
}
