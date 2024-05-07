#include "libraries.h"
#include "firebase.h"
#include "camera.h"
#include "globalmethods.h"
#include "display.h"

//Replace with your network credentials
const char *ssid = "***"
const char *password = "****";

//Multithreading
TaskHandle_t coreTask0;
camera_fb_t *motionPhoto = nullptr;

//Define Firebase Data objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;

void fcsUploadCallback(FCS_UploadStatusInfo info);

bool taskCompleted = false;

void initWiFi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
}

void initLittleFS() {
    if (!LittleFS.begin(true)) {
        Serial.println("An Error has occurred while mounting LittleFS");
        ESP.restart();
    } else {
        delay(500);
        Serial.println("LittleFS mounted successfully");
    }
}

void initCamera() {
    // OV2640 camera module
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
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

    // SVGA config
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 15;
    config.fb_count = 1;

    // Camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        ESP.restart();
    }
}

// IR-Sensor Pin
const byte sensorPin = 33;

// Taster Pin
const int buttonPin = 0;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int lastButtonState = HIGH;
unsigned long previousMillis = 0;
const long photoInterval = 1500;
bool method1Active = true;




void setup() {
    pinMode(buttonPin, INPUT_PULLUP);
    Serial.begin(115200);
    Wire.begin(2, 15);

//Multithreading
    motionPhoto = nullptr;
    xTaskCreatePinnedToCore(CoreTask0, "CPU_0", 1000, NULL, 1, &coreTask0, 0);

// Date&Time Sync
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");


//Display Setup

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }

    display.clearDisplay();
    display.display();
    delay(2000);
    display.invertDisplay(true);
    delay(1000);
    display.invertDisplay(false);
    delay(1000);


// Serial port for debugging purposes

    initWiFi();
    initLittleFS();

// Turn-off the 'brownout detector'

    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    initCamera();

//Firebase

    configF.api_key = API_KEY;
    configF.database_url = DATABASE_URL;
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
    configF.token_status_callback = tokenStatusCallback;

    Firebase.begin(&configF, &auth);
    Firebase.reconnectWiFi(true);

    testscrolltext();

}

void loop() {

    mainFunction();

}


//Motion Detected

void motion() {
    displayNotifications("Motion", "Modus", 66, 35, 56, 2);
    delay(2000);
    while (method1Active) {
        unsigned long currentMillis = millis();
        int currentButtonState = digitalRead(buttonPin);

        if (currentButtonState == LOW && lastButtonState == HIGH &&
            (currentMillis - lastDebounceTime) > debounceDelay) {
            lastDebounceTime = currentMillis;
            Serial.println("BUTTON PRESS");
            method1Active = !method1Active;
        }
        int distance = analogRead(sensorPin);
        Serial.println(distance);

        if (currentMillis - previousMillis >= photoInterval) {
            previousMillis = currentMillis;

            if (motionPhoto == nullptr) {
                motionPhoto = esp_camera_fb_get();
                if (motionPhoto != nullptr) {
                    xTaskCreatePinnedToCore(CoreTask0, "UploadTask", 10000, NULL, 1, NULL, 0);
                }
            }
        }
        if (distance >= 500) {
            displayNotifications("Motion", "Detected", 66, 35, 86, 2);
        } else {
            displayNotifications("System", "Ready", 66, 35, 56, 2);
        }
        delay(500);
    }
}


// Notification

void sendFCMNotification(const String &title, const String &body, const String &deviceToken) {
    HTTPClient http;

    http.begin("https://fcm.googleapis.com/fcm/send");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization",
                   "***");

    String jsonPayload =
            "{\"to\":\"" + deviceToken + "\",\"notification\":{\"title\":\"" + title + "\",\"body\":\"" + body + "\"}}";

    Serial.println("Sending FCM Notification with the following payload:");
    Serial.println(jsonPayload);

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
        Serial.print("FCM Notification sent with response code: ");
        Serial.println(httpResponseCode);

        // Antwort vom Server ausgeben
        String response = http.getString();
        Serial.println("Response from FCM:");
        Serial.println(response);
    } else {
        Serial.print("Error sending FCM Notification. HTTP Response code: ");
        Serial.println(httpResponseCode);
    }

    http.end();
}

//Alarm Detected

void alarm() {
    displayNotifications("Alarm", "Modus", 56, 35, 56, 2);
    delay(2000);

    unsigned long currentMillis = millis();

    while (!method1Active) {
        int currentButtonState = digitalRead(buttonPin);
        if (currentButtonState == LOW && lastButtonState == HIGH &&
            (currentMillis - lastDebounceTime) > debounceDelay) {
            lastDebounceTime = currentMillis;
            Serial.println("BUTTON PRESS");
            method1Active = !method1Active;
        }
        int distance = analogRead(sensorPin);
        Serial.println(distance);
        delay(300);

        if (distance >= 500) {
            displayNotifications("Alarm", "!!!", 66, 35, 40, 2);
            captureAndSavePhotos();
            uploadPics();

            // Send FCM Notification an alle Geräte
            sendFCMNotification("Türspion", "Alarm wurde ausgelöst!", getDeviceToken());
        } else {
            displayNotifications("System", "Ready", 66, 35, 56, 2);
        }
    }
}


//Scrolltext

void testscrolltext(void) {

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);

    int16_t yPos = display.height();

    for (int repeat = 0; repeat < 2; repeat++) {
        while (yPos >= -16) {
            display.clearDisplay();
            display.setCursor(10, yPos);
            display.println(F("HomeWatch"));
            display.display();
            yPos -= 2;
            delay(30);
        }

        yPos = display.height();
    }
    delay(1000);
    displayNotifications("System", "Ready", 66, 35, 56, 2);
}

//Upload Callback/Info

// Initialisieren Sie den Zähler im globalen Bereich
int urlCounter = 1;

void fcsUploadCallback(FCS_UploadStatusInfo info) {
    String currentDate = dateStamp();
    String currentTime = timeStamp();
    if (info.status == firebase_fcs_upload_status_init) {
        Serial.printf("Uploading file %s (%d) to %s\n", info.localFileName.c_str(), info.fileSize,
                      info.remoteFileName.c_str());
    } else if (info.status == firebase_fcs_upload_status_upload) {
        Serial.printf("Uploaded %d%s, Elapsed time %d ms\n", (int) info.progress, "%", info.elapsedTime);
    } else if (info.status == firebase_fcs_upload_status_complete) {
        Serial.println("Upload completed\n");
        FileMetaInfo meta = fbdo.metaData();
        Serial.printf("Name: %s\n", meta.name.c_str());
        Serial.printf("Bucket: %s\n", meta.bucket.c_str());
        Serial.printf("contentType: %s\n", meta.contentType.c_str());
        Serial.printf("Size: %d\n", meta.size);
        Serial.printf("Generation: %lu\n", meta.generation);
        Serial.printf("Metageneration: %lu\n", meta.metageneration);
        Serial.printf("ETag: %s\n", meta.etag.c_str());
        Serial.printf("CRC32: %s\n", meta.crc32.c_str());
        Serial.printf("Tokens: %s\n", meta.downloadTokens.c_str());
        Serial.printf("Download URL: %s\n\n", fbdo.downloadURL().c_str());

        String photoURL = fbdo.downloadURL().c_str();

        String urlPath =
                "/users/user/Tuerspion/H-Guard/" + currentDate + "/" + currentTime + "/URL" + String(urlCounter);

        if (Firebase.RTDB.setString(&fbdo, urlPath.c_str(), photoURL.c_str())) {
            Serial.println("Download URL written to the database");
        } else {
            Serial.println("Failed to write data to Firebase");
        }
        urlCounter++;
    } else if (info.status == firebase_fcs_upload_status_error) {
        Serial.printf("Upload failed, %s\n", info.errorMsg.c_str());
    }
}

//Upload Pics

void uploadPics() {
    if (Firebase.ready() && !taskCompleted) {
        taskCompleted = true;
        Serial.print("Uploading pictures... ");
        for (int i = 0; i < 3; i++) {
            String timestamp = timeStamp();
            String datestamp = dateStamp();
            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb) {
                Serial.println("Camera capture failed");
                delay(1000);
                ESP.restart();
            }
            String timestampedFileName = "/Alarm-Pics/photo-";
            timestampedFileName += timestamp;
            timestampedFileName += "-";
            timestampedFileName += datestamp;
            timestampedFileName += ".jpg";
            if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID, fb->buf, fb->len, timestampedFileName.c_str(),
                                        "image/jpeg", fcsUploadCallback)) {
                Serial.printf("Uploaded photo: %s\n", timestampedFileName.c_str());
            } else {
                Serial.println(fbdo.errorReason());
            }

            esp_camera_fb_return(fb);
        }
    }
    deletePhotosFromFlash();
    taskCompleted = false;
}

// Capture Pics

void captureAndSavePhotos() {
    for (int i = 0; i < 3; i++) {

        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            delay(1000);
            ESP.restart();
        }
        String photoFileName = "/photo";
        photoFileName += String(i);
        photoFileName += ".jpg";
        File file = LittleFS.open(photoFileName, "w");

        if (!file) {
            Serial.println("Failed to open file in writing mode");
        } else {
            file.write(fb->buf, fb->len);
            Serial.print("Photo ");
            Serial.print(i + 1);
            Serial.print(" saved as: ");
            Serial.println(photoFileName);
        }

        file.close();
        esp_camera_fb_return(fb);
    }
}

// Flash Cleaner

void deletePhotosFromFlash() {
    for (int i = 0; i < 3; i++) {
        String photoFileName = "/photo";
        photoFileName += String(i);
        photoFileName += ".jpg";
        if (LittleFS.remove(photoFileName)) {
            Serial.print("Photo ");
            Serial.print(i + 1);
            Serial.println(" deleted from Flash");
        } else {
            Serial.println("Failed to delete photo from Flash");
        }
    }
}

// Display Notification

void displayNotifications(String line1, String line2, int line1x, int line1y, int line2x, int line2y) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor((display.width() - line1x) / 2, (display.height() - line1y) / 2);  // Move "System" up
    display.println(line1);
    display.setCursor((display.width() - line2x) / 2, (display.height() - line2y) / 2);  // Move "Ready" up
    display.println(line2);
    display.display();
}


// Callback-Function

void setAsyncUploadCallback(AsyncCallback callback) {
    asyncUploadCallback = callback;
}

// Timestamp

String timeStamp() {
    time_t now = time(nullptr);
    struct tm *timeinfo;
    timeinfo = localtime(&now);
    char buffer[9];
    sprintf(buffer, "%02d:%02d:%02d",
            timeinfo->tm_hour + 1,
            timeinfo->tm_min,
            timeinfo->tm_sec);

    String timestamp = String(buffer);
    return timestamp;
}


// DateStamp

String dateStamp() {
    time_t now = time(nullptr);
    struct tm *timeinfo;
    timeinfo = localtime(&now);
    String day = (timeinfo->tm_mday < 10) ? "0" + String(timeinfo->tm_mday) : String(timeinfo->tm_mday);
    String month = (timeinfo->tm_mon + 1 < 10) ? "0" + String(timeinfo->tm_mon + 1) : String(timeinfo->tm_mon + 1);
    String year = String(timeinfo->tm_year + 1900);
    String datestamp = day + month + year;
    return datestamp;
}


//MainFunction

void mainFunction() {
    int reading = digitalRead(buttonPin);

    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }
    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading != lastButtonState) {
            lastButtonState = reading;

            if (lastButtonState == LOW) {
                method1Active = !method1Active;
            }
        }
    }
    if (method1Active) {
        motion();
    } else {
        alarm();
    }
}

//Threading Task

void CoreTask0(void *parameter) {
    if (motionPhoto != nullptr) {
        if (Firebase.ready()) {
            String livePhotoPath = "/Live-Pics/Live-Pic.jpg";
            if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID, motionPhoto->buf, motionPhoto->len,livePhotoPath.c_str(), "image/jpeg", nullptr)) {
                Serial.println("Live photo uploaded to Firebase");
            } else {
                Serial.println(fbdo.errorReason());
            }
        }
        esp_camera_fb_return(motionPhoto);
        motionPhoto = nullptr;
    }
    vTaskDelete(NULL);
}

//Device Token

String getDeviceToken() {
    HTTPClient http;
    String firebaseUrl = "https://homewatch-e022f-default-rtdb.europe-west1.firebasedatabase.app/users/user/deviceToken/deviceTokenApp/token.json";
    String token = "";

    http.begin(firebaseUrl);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
        String response = http.getString();
        token = response.substring(1, response.length() - 1);
    } else {
        Serial.print("Fehler beim Abrufen des Tokens: ");
        Serial.println(httpResponseCode);
    }
    http.end();
    return token;
}
