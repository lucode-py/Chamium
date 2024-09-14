#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#define CAMERA_MODEL_WROVER_KIT
#include "camera_pins.h"

// Paramètres Wi-Fi
const char* ssid = "Livebox-D770";
const char* password = "FamilleDUGARD2021";

// Initialisation des serveurs
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Fonction pour gérer les commandes des joysticks via WebSocket
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    String command = String((char*)payload);
    Serial.printf("Commande reçue: %s\n", command.c_str());
    // Traitement des commandes en fonction de la direction et de la vitesse envoyées
    // Par exemple, commande = {"direction":"forward", "speed":255}
  }
}

// Fonction pour servir la page HTML
void handleRoot() {
  server.send(200, "text/html", R"rawliteral(
   <!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Control Your Robot</title>
    <style>
        body, html {
            margin: 0;
            padding: 0;
            height: 100%;
            display: flex;
            justify-content: center;
            align-items: center;
            flex-direction: column;
            background-color: black;
        }
        
        #camera-feed {
            width: 100%;
            height: 100vh; /* Utilise 100% de la hauteur de l'écran */
            display: flex;
            justify-content: center;
            align-items: center;
            background-color: black;
        }
        
        #camera-feed img {
            width: 100%;  /* L'image prendra 100% de la largeur disponible */
            height: auto; /* L'image gardera son ratio d'aspect */
            object-fit: cover; /* L'image s'ajustera pour couvrir la div */
        }
        
        #joystick-container {
            position: absolute;
            bottom: 10px;
            width: 100%;
            display: flex;
            justify-content: space-between;
            padding: 0 10px;
        }

        #joystick-left, #joystick-right {
            width: 45%;
            height: 200px;
        }
    </style>
</head>
<body>
    <!-- Video Stream -->
      <div id="camera-feed">
          <img id="robot-stream" src="/stream" alt="Video Stream" />
      </div>

    <!-- Joysticks -->
    <div id="joystick-container">
        <div id="joystick-left"></div>
        <div id="joystick-right"></div>
    </div>

    <script src="https://cdnjs.cloudflare.com/ajax/libs/nipplejs/0.7.3/nipplejs.min.js"></script>
    <script>
        // Joystick setup
        const leftJoystick = nipplejs.create({
            zone: document.getElementById('joystick-left'),
            mode: 'static',
            position: { left: '50px', bottom: '50px' },
            color: 'blue'
        });

        const rightJoystick = nipplejs.create({
            zone: document.getElementById('joystick-right'),
            mode: 'static',
            position: { right: '50px', bottom: '50px' },
            color: 'red'
        });

        // WebSocket connection to Laravel (replace with your WebSocket URL)
        const ws = new WebSocket('ws://your-laravel-server/ws/control');

        // Send movement commands from joystick data
        leftJoystick.on('move', (evt, data) => {
            const direction = data.direction ? data.direction.angle : null;
            const distance = data.distance || 0;
            const speed = Math.min(Math.round(distance * 2.55), 255);  // Map distance to speed (0-255)
            
            if (direction === 'up') {
                sendCommand('forward', speed);
            } else if (direction === 'down') {
                sendCommand('backward', speed);
            }
        });

        rightJoystick.on('move', (evt, data) => {
            const direction = data.direction ? data.direction.angle : null;
            const distance = data.distance || 0;
            const speed = Math.min(Math.round(distance * 2.55), 255);

            if (direction === 'left') {
                sendCommand('left', speed);
            } else if (direction === 'right') {
                sendCommand('right', speed);
            }
        });

        // Function to send commands via WebSocket
        function sendCommand(direction, speed) {
            const command = JSON.stringify({ direction, speed });
            ws.send(command);
        }
    </script>
</body>
</html>
  )rawliteral");
}

// Fonction pour diffuser le flux vidéo de la caméra
void handleStream() {
  // On sert l'image de la caméra ici
  WiFiClient client = server.client();
  String response = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);

  while (client.connected()) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Erreur capture caméra");
      break;
    }

    response = "--frame\r\nContent-Type: image/jpeg\r\n\r\n";
    server.sendContent(response);
    client.write(fb->buf, fb->len);
    esp_camera_fb_return(fb);

    response = "\r\n";
    server.sendContent(response);
  }
}

// Configuration de la caméra
void startCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    // ... autres paramètres ...
    config.frame_size = FRAMESIZE_QVGA;  // Petite résolution
    config.jpeg_quality = 4;  // Compression minimale pour une transmission plus rapide
    config.fb_count = 1;  // Un seul tampon mémoire
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 4;
    config.fb_count = 1;
  }

  // Initialiser la caméra
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Erreur initialisation de la caméra: 0x%x", err);
    return;
  }
}

// Configuration du serveur
void startCameraServers() {
  server.on("/", HTTP_GET, handleRoot);   // Sert la page HTML
  server.on("/stream", HTTP_GET, handleStream); // Flux vidéo de la caméra

  // Démarrage du WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Démarrage du serveur HTTP
  server.begin();
  Serial.println("Serveur HTTP démarré");
}

void setup() {
  Serial.begin(115200);
  
  // Connexion au réseau Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connecté au Wi-Fi");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());

  // Initialiser la caméra
  startCamera();

  // Démarrer le serveur web
  startCameraServers();
}

void loop() {
  // Gérer les requêtes HTTP
  server.handleClient();
  // Gérer les événements WebSocket
  webSocket.loop();
}