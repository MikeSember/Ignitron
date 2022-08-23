/*
 * SparkOTAServer.cpp
 *
 *  Created on: 10.10.2021
 *      Author: steffen
 */

#include "SparkOTAServer.h"

const char *SparkOTAServer::serverIndex =
		"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
				"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
				"<input type='file' name='update'>"
				"<input type='submit' value='Update'>"
				"</form>"
				"<div id='prg'>progress: 0%</div>"
				"<script>"
				"$('form').submit(function(e){"
				"e.preventDefault();"
				"var form = $('#upload_form')[0];"
				"var data = new FormData(form);"
				" $.ajax({"
				"url: '/update',"
				"type: 'POST',"
				"data: data,"
				"contentType: false,"
				"processData:false,"
				"xhr: function() {"
				"var xhr = new window.XMLHttpRequest();"
				"xhr.upload.addEventListener('progress', function(evt) {"
				"if (evt.lengthComputable) {"
				"var per = evt.loaded / evt.total;"
				"$('#prg').html('progress: ' + Math.round(per*100) + '%');"
				"}"
				"}, false);"
				"return xhr;"
				"},"
				"success:function(d, s) {"
				"console.log('success!')"
				"},"
				"error: function (a, b, c) {"
				"}"
				"});"
				"});"
				"</script>";

WebServer SparkOTAServer::server(80);

SparkOTAServer::SparkOTAServer() {
	// TODO Auto-generated constructor stub

}

SparkOTAServer::~SparkOTAServer() {
	// TODO Auto-generated destructor stub
}


bool SparkOTAServer::init() {
	// Connect to WiFi network
	Serial.println("Starting WiFI");
	WiFi.begin(ssid, password);
	Serial.println("");

	// Wait for connection
	unsigned int startScanTime = millis();
	while (WiFi.status() != WL_CONNECTED) {
		unsigned int currentTime = millis();
		if (currentTime - startScanTime > connectTimeout) {
			Serial.println("WiFi connect timeout, continuing without WiFi");
			return false;
		}
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	/*use mdns for host name resolution*/
	if (!MDNS.begin(host)) { //http://ignitron.local
		Serial.println("Error setting up MDNS responder!");
		while (1) {
			delay(500);
		}
	}
	Serial.println("mDNS responder started");
	/*return index page which is stored in serverIndex */
	server.on("/", HTTP_GET, []() {
		server.sendHeader("Connection", "close");
		server.send(200, "text/html", serverIndex);
	});
	/*handling uploading firmware file */
	server.on("/update", HTTP_POST, []() {
		server.sendHeader("Connection", "close");
		server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
		ESP.restart();
	}
	, []() {
		HTTPUpload &upload = server.upload();
		if (upload.status == UPLOAD_FILE_START) {
			Serial.printf("Update: %s\n", upload.filename.c_str());
			if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
				Update.printError(Serial);
			}
				} else if (upload.status == UPLOAD_FILE_WRITE) {

					// flashing firmware to ESP
					if (Update.write(upload.buf, upload.currentSize)
							!= upload.currentSize) {
						Update.printError(Serial);
					}
				} else if (upload.status == UPLOAD_FILE_END) {
					if (Update.end(true)) { //true to set the size to the current progress
						Serial.printf("Update Success: %u\nRebooting...\n",
								upload.totalSize);
					} else {
						Update.printError(Serial);
					}
				}
	}
	);

	server.begin();
	return true;
}

void SparkOTAServer::handleClient() {
	server.handleClient();
}
