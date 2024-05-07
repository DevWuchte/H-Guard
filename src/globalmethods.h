#ifndef TUERSPION_GLOBALMETHODS_H
#define TUERSPION_GLOBALMETHODS_H

#include "libraries.h"
#include "firebase.h"
#include "camera.h"

// LivPic
typedef std::function<void(void)> AsyncCallback;
AsyncCallback asyncUploadCallback = nullptr;

//declare Methods
void testscrolltext(void);
void alarm(void);
void motion(void);
void displayNotifications(String line1, String line2, int line1x, int line1y, int line2x, int line2y);
void uploadPics(void);
void captureAndSavePhotos(void);
void deletePhotosFromFlash(void);
String timeStamp();
String dateStamp();
void sendFCMNotification(const String &title, const String &body);
void uploadLivePic(void);
void uploadLivePicAsync();
void setAsyncUploadCallback(AsyncCallback callback);
void uploadLivePicAsync(void);
void mainFunction();
void CoreTask0(void *parameter);
void uploadLivePicToFirebase();
String getDeviceToken();

#endif
