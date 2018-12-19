#ifndef __GET_REQUESTER_H
#define __GET_REQUEESTER_H

#include <string>

#include "curl/curl.h"

class GetRequester {
public:
	GetRequester();
	~GetRequester();
	std::string sendRequest(std::string url);

	/* representing the response codes from the request (html response codes from 100-599)
	and from 600 and above used to set custom error codes to get information about the request
	619 : Timeout while connecting
	690 : Timeout while transfering
	*/
	long getResponseCode();

private:
	CURL *curl;
	CURLcode res;
	long responseCode;
};

#endif