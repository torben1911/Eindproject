#include "WiFi.h" // Bibliotheek nodig voor de wifi.
#include <PubSubClient.h> // Bibliotheek nodig voor MQTT.

#define KNOP 17
#define LED 16

// Declaratie van methoden
void mqttConnect();
void callback(char* topic, byte* message, unsigned int length);
void initWiFi();
void initMQTTbroker();
void ControleerDrukknop();
void IRAM_ATTR ChangeDrukknop();

// WiFi-instellingen
const char* ssid = "ICW"; // SSID van het te gebruiken wifi-netwerk.
const char* paswoord = "ICW6_ICW6"; // Paswoord van het te gebruiken wifi-netwerk

// MQTT-instellingen
WiFiClient WiFiClient1; // Een object wifiClient1 van de klasse wifiClient.
const char* mqttBroker = "broker.hivemq.com"; // De te gebruiken MQTT-broker.
const char* mqttClientName = "IoT_Torben_Plas"; // Naam van de MQTT-client
const char* topicToSub = "IoT/TorbenP/knop"; // Naam van de topic waarop geabonneerd wordt.
const char* topicToPub = "IoT/TorbenP/status"; // Naam van de topic waarop data verstuurd wordt.
PubSubClient mqttClient1(WiFiClient1);

// Variabelen voor knopcontrole
bool knopChange = false;
unsigned long startTijd;
unsigned long huidigeTijd;
const unsigned long DENDERTIJD = 40; // Dendervertraging
uint8_t Teller = 0;
bool knopEnabled = true;
bool ledEnabled = true;

// ****************** BEGIN WiFi-instellingen ******************
void initWiFi() {
    WiFi.begin(ssid, paswoord);
    Serial.println("Verbindt met WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(500);
    }
    Serial.println("\nVerbonden met WiFi.");
    Serial.print("IP-adres: ");
    Serial.println(WiFi.localIP());
}

// ****************** BEGIN MQTT-brokerinstellingen ******************
void initMQTTbroker() {
    mqttClient1.setServer(mqttBroker, 1883);
    mqttClient1.setCallback(callback);
}

void mqttConnect() {
    while (!mqttClient1.connected()) {
        Serial.print("Verbind met MQTT-broker...");
        if (mqttClient1.connect(mqttClientName)) {
            Serial.println("Verbonden");
            mqttClient1.subscribe(topicToSub);
        } else {
            Serial.print("Mislukt, rc=");
            Serial.print(mqttClient1.state());
            Serial.println(" Probeer opnieuw binnen 5 seconden");
            delay(5000);
        }
    }
}

// ****************** BEGIN Callback ******************
void callback(char* topic, byte* message, unsigned int length) {
    String ontvangenMsg;
    for (int i = 0; i < length; i++) {
        ontvangenMsg += (char)message[i];
    }
    Serial.print("Bericht ontvangen van topic: ");
    Serial.print(topic);
    Serial.print(". Bericht: ");
    Serial.println(ontvangenMsg);

    if (String(topic) == topicToSub) {
        if (ontvangenMsg.equalsIgnoreCase("enableknop")) {
            knopEnabled = true;
            Serial.println("Knop is ingeschakeld via MQTT.");
        } else if (ontvangenMsg.equalsIgnoreCase("disableknop")) {
            knopEnabled = false;
            Serial.println("Knop is uitgeschakeld via MQTT.");
        } else if (ontvangenMsg.equalsIgnoreCase("enableled")) {
            ledEnabled = true;
            digitalWrite(LED, HIGH);
            Serial.println("LED is ingeschakeld via MQTT.");
        } else if (ontvangenMsg.equalsIgnoreCase("disableled")) {
            ledEnabled = false;
            digitalWrite(LED, LOW);
            Serial.println("LED is uitgeschakeld via MQTT.");
        }
    }
}

// ****************** BEGIN Drukknopverwerking ******************
void IRAM_ATTR ChangeDrukknop() {
    if (knopEnabled) {
        knopChange = true;
        startTijd = millis();
    }
}

void ControleerDrukknop() {
    huidigeTijd = millis();
    if (huidigeTijd - startTijd >= DENDERTIJD) {
        knopChange = false;
        bool StatusKnop = digitalRead(KNOP);
        if (!StatusKnop) {
            Serial.print("Knop ingedrukt: ");
            Serial.println(Teller);
            Teller++;

            char msg[50];
            snprintf(msg, 50, "Knop ingedrukt: %u", Teller);
            mqttClient1.publish(topicToPub, msg);
        }
    }
}

// ****************** BEGIN Setup ******************
void setup() {
    Serial.begin(115200);
    initWiFi();
    initMQTTbroker();

    pinMode(LED, OUTPUT);
    pinMode(KNOP, INPUT);
    attachInterrupt(KNOP, ChangeDrukknop, CHANGE);

    Serial.println("Typ 'enableknop', 'disableknop', 'enableled', of 'disableled' in de Serial Monitor.");
}

// ****************** BEGIN Loop ******************
void loop() {
    if (!mqttClient1.connected()) {
        mqttConnect();
    }
    mqttClient1.loop();

    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();

        if (command.equalsIgnoreCase("enableknop")) {
            knopEnabled = true;
            Serial.println("Knop is ingeschakeld via Serial.");
        } else if (command.equalsIgnoreCase("disableknop")) {
            knopEnabled = false;
            Serial.println("Knop is uitgeschakeld via Serial.");
        } else if (command.equalsIgnoreCase("enableled")) {
            ledEnabled = true;
            digitalWrite(LED, HIGH);
            Serial.println("LED is ingeschakeld via Serial.");
        } else if (command.equalsIgnoreCase("disableled")) {
            ledEnabled = false;
            digitalWrite(LED, LOW);
            Serial.println("LED is uitgeschakeld via Serial.");
        } else {
            Serial.println("Ongeldig commando.");
        }
    }

    if (knopChange) {
        ControleerDrukknop();
    }
}