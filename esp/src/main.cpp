#define LOG_LOCAL_LEVEL ESP_LOG_ERROR

#include "Arduino.h"
#include "BLEDevice.h"

// The remote service we wish to connect to.
static BLEUUID serviceUUID(uint32_t(0x180D)); // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.heart_rate.xml
static BLEUUID    charUUID(uint32_t(0x2A37)); //https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.heart_rate_measurement.xml

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = true;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("i: Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("i: data: ");

    uint8_t firstData = pData[0];
    
    String format = "uint 8";
    bool is16Bit = bitRead(firstData, 0) == 1;
    if (is16Bit) {
      format = "uint 16";
    }

    Serial.print("i: Data format: "); Serial.println(format);

    bool contactSupported = bitRead(firstData, 2) == 1;
    if (contactSupported) {
      bool connected = bitRead(firstData, 1);
      Serial.printf("d: connected: %d\n", connected);
    } 

    uint16_t rate;
    if (is16Bit) {
      uint8_t major = pData[1];
      uint8_t minor = pData[2];

      rate = uint16_t(major << 8) & minor;
    } else {
      rate = uint16_t(pData[1]);
    }
    
    Serial.print("d: rate: ");
    Serial.println(rate);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("i: onConnect");
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("i: onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("i: Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println("i: - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice); 
    Serial.println("i: - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("i: Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println("i: - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("i: Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println("i: - Found our characteristic");

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;

    return true;
}

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("i: BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    }
  }
};


void setup() {
  Serial.begin(115200);
  Serial.println("i: Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}


void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("i: We are now connected to the BLE Server.");
    } else {
      Serial.println("i: We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  if(!connected && doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }
  
  delay(1000);
}