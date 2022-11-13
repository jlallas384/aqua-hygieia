#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"             // disable brownout problems
#include "soc/rtc_cntl_reg.h"    // disable brownout problems
#include "esp_http_server.h"
#include "SerialTransfer.h"

// Replace with your network credentials
const char* ssid = "CD-R KING_AF2E0C";
const char* password = "Baldonheng1617";

// const char* ssid = "AquaHygeia";
// const char* password = "walangpass";

// const char* ssid1 = "ESP32-CAM Robot";
// const char* password1 = "1234567890";

#define PART_BOUNDARY "123456789000000000000987654321"
// lfg
#define CAMERA_MODEL_AI_THINKER

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>S.O.L.A.R Controller Web Interface</title>
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.2/dist/leaflet.css" integrity="sha256-sA+zWATbFveLLNqWO2gtiw3HL/lh1giY/Inf1BJ0z14=" crossorigin="" />
    <script src="https://unpkg.com/leaflet@1.9.2/dist/leaflet.js" integrity="sha256-o9N1jGDZrf5tS+Ft4gbIK7mYMipq9lqpVJ91xHSyKhg=" crossorigin=""></script>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.2.2/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-Zenh87qX5JnK2Jl0vWa8Ck2rdkQ2Bzep5IDxbcnCeuOxjzrPF/et3URy9Bv1WTRi" crossorigin="anonymous">
    <script src="https://code.jquery.com/jquery-3.6.1.js" integrity="sha256-3zlB5s2uwoUzrXK3BT7AX3FyvojsraNFxCc2vC/7pNI=" crossorigin="anonymous"></script>
    <style type="text/css">
      .loading:after {
        overflow: hidden;
        display: inline-block;
        vertical-align: bottom;
        -webkit-animation: ellipsis steps(4, end) 900ms infinite;
        animation: ellipsis steps(4, end) 900ms infinite;
        content: "\2026";
        /* ascii code for the ellipsis character */
        width: 0px;
      }

      @keyframes ellipsis {
        to {
          width: 1.25em;
        }
      }

      @-webkit-keyframes ellipsis {
        to {
          width: 1.25em;
        }
      }
    </style>
  </head>
  <body>
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.2.2/dist/js/bootstrap.bundle.min.js" integrity="sha384-OERcA2EqjJCMA+/3y+gxIOqMEjwtxJY7qPCqsdltbNJuaOe923+mo//f6V8Qbsw3" crossorigin="anonymous"></script>
    <nav class="navbar navbar-dark bg-dark ">
      <div class="container-fluid">
        <a class="navbar-brand"> Aqua Hygeia </a>
      </div>
    </nav>
    <div class="container">
      <div class="row justify-content-between">
        <div class="col">
          <div id="map" style="width: 100%; height: 200px; float: left">
            <p class="text-dark text-center loading">Finding GPS Satellites to connect</p>
          </div>
        </div>
      </div>
      <div class="row justify-content-between">
        <div class="col text-center">
          <img style="width: 75%; height: 200px; float: center;" id="cam">
        </div>
      </div>
      <div class="row">
        <p class="text-dark fs-5">pH Level: <span id="ph"></span>
        </p>
      </div>
      <div class="row justify-content-center">
        <div class="col-9">
          <div class="row justify-content-center">
            <div class="col-3" style="padding:0">
              <button class="btn btn-dark" ontouchstart="move('U')" ontouchend="move('S')" style="width:100%">U</button>
            </div>
          </div>
          <div class="row justify-content-center">
            <div class="col-3" style="padding:0">
              <button class="btn btn-dark" ontouchstart="move('L')" ontouchend="move('S')" style="width:100%;">L</button>
            </div>
            <div class="col-3 offset-3" style="padding:0">
              <button class="btn btn-dark" ontouchstart="move('R')" ontouchend="move('S')" style="width:100%">R</button>
            </div>
          </div>
          <div class="row justify-content-center">
            <div class="col-3" style="padding:0">
              <button class="btn btn-dark" ontouchstart="move('D')" ontouchend="move('S')" style="width:100%">D</button>
            </div>
          </div>
        </div>
      </div>
    </div>
    <script type="application/javascript">
      function move(e){var n=new XMLHttpRequest;n.open("GET","action?move="+e,!0),n.send()}function go(e){var n=new XMLHttpRequest;n.onload=function(){e(f=JSON.parse(obj=this.responseText))},n.open("GET","status",!0),n.send()}var map,cur,flg=0;setInterval(function(){go(function(e){document.getElementById("ph").innerHTML=e.ph,-1!=e.lat&&(flg?(map.removeLayer(cur),cur=new L.marker([e.lat,e.lng]),map.addLayer(cur)):(mapOptions={center:[e.lat,e.lng],zoom:18,zoomControl:!1,boxZoom:!1,dragging:!1,touchZoom:!1},map=new L.map("map",mapOptions),layer=new L.TileLayer("http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"),cur=new L.Marker([e.lat,e.lng]),map.addLayer(layer),map.addLayer(cur),flg=1))})},1e4),window.onload=document.getElementById("cam").src=window.location.href.slice(0,-1)+":81/stream";
    </script>
  </body>
</html>
)rawliteral";

static esp_err_t index_handler(httpd_req_t *req){
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}

static esp_err_t stream_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK){
    return res;
  }

  while(true){
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      if(fb->width > 400){
        if(fb->format != PIXFORMAT_JPEG){
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if(!jpeg_converted){
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if(res == ESP_OK){
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if(fb){
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if(res != ESP_OK){
      break;
    }
    //Serial.printf("MJPG: %uB\n",(uint32_t)(_jpg_buf_len));
  }
  return res;
}
int global = 1;
static esp_err_t status_handler(httpd_req_t *req) {
  static char json_response[1024];
 
  sensor_t * s = esp_camera_sensor_get();
  char * p = json_response;
  *p++ = '{';
 
  p += sprintf(p, "\"lat\":%d,", -1);
  p += sprintf(p, "\"lng\":%d,", -1);
  p += sprintf(p, "\"ph\":%u", global);
  *p++ = '}';
  *p++ = 0;
  global++;
  if(global == 13) global = 0;
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, json_response, strlen(json_response));
}

enum __attribute__((__packed__)) ACTION{
  UP,
  DOWN,
  LEFT,
  RIGHT,
  STOP
} act;

SerialTransfer st;

static esp_err_t cmd_handler(httpd_req_t *req){
  char*  buf;
  size_t buf_len;
  char variable[32] = {0,};
  
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = (char*)malloc(buf_len);
    if(!buf){
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "move", variable, sizeof(variable)) == ESP_OK) {
      } else {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }
    } else {
      free(buf);
      httpd_resp_send_404(req);
      return ESP_FAIL;
    }
    free(buf);
  } else {
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  sensor_t * s = esp_camera_sensor_get();
  int res = 0;
  
  if(!strcmp(variable, "U")) {
    act = UP;
    Serial.println("DOING UP");
  }
  else if(!strcmp(variable, "D")) {
    act = DOWN;
    Serial.println("DOING DOWN");
  }
  else if(!strcmp(variable, "L")) {
    act = LEFT;
    Serial.println("DOING LEFT");
  }
  else if(!strcmp(variable, "R")) {
    act = RIGHT;
    Serial.println("DOING RIGHT");
  }
  else if(!strcmp(variable, "S")) {
    act = STOP;
    Serial.println("DOING STOP");
  }
  else {
    res = -1;
  }
  st.sendDatum(act);
  if(res){
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

void startCameraServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
  };
  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };
  httpd_uri_t status_uri = {
    .uri       = "/status",
    .method    = HTTP_GET,
    .handler   = status_handler,
    .user_ctx  = NULL
  };
  httpd_uri_t cmd_uri = {
    .uri       = "/action",
    .method    = HTTP_GET,
    .handler   = cmd_handler,
    .user_ctx  = NULL
  };
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &status_uri);
    httpd_register_uri_handler(camera_httpd, &cmd_uri);
  }
  config.server_port += 1;
  config.ctrl_port += 1;
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

unsigned long sm, cm;
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  pinMode(4,OUTPUT);
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial1.begin(76800, SERIAL_8N1, 14, 15);
  st.begin(Serial1);
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
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 30;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // WiFi.softAP(ssid1, password1);
  // IPAddress myIP = WiFi.softAPIP();
  // Serial.print("AP IP address: ");
  // Serial.println(myIP);
  // Wi-Fi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");    
  }
  WiFi.setSleep(false);
  digitalWrite(4,HIGH);
  delay(1000);
  digitalWrite(4,LOW);
  Serial.println("");
  Serial.println("WiFi connected");
  
  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.println(WiFi.localIP());
  Serial.print("SIGNAL ");
  Serial.println(WiFi.RSSI());
  //Start streaming web server
  startCameraServer();
  sm = millis();
}

void loop() {
  cm = millis();
  if(cm - sm >= 5000){
    //Serial.println(WiFi.RSSI());
    sm = cm;
  }
}