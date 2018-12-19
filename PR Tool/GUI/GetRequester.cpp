#include "GetRequester.h"

GetRequester::GetRequester() {
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
}

GetRequester::~GetRequester() {
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}

size_t saveResponse(char* ptr, size_t size, size_t nmemb, void* userdata) {
	((std::string*) userdata)->append(ptr, nmemb*size);

	return size * nmemb;
}

std::string GetRequester::sendRequest(std::string url) {
	if (curl) {
		responseCode = -1;

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		std::string responseBuffer;

		// Set buffer as pointer given to the saveResponse-method
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
		// Set saveResponse-method to the method called to handle the response-stream from the request
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &saveResponse);

		res = curl_easy_perform(curl);

		/* Check for errors */
		if (res != CURLE_OK) {
			switch (res) {
			case CURLE_COULDNT_CONNECT:
				responseCode = 619;
				break;
			case CURLE_OPERATION_TIMEDOUT:
				responseCode = 690;
			}
			return "";
		}

		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

		return responseBuffer;
	}

	return "";
}

long GetRequester::getResponseCode() {
	return responseCode;
}