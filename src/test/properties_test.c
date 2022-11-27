#ifndef PROPERTIES_TEST_C
#define PROPERTIES_TEST_C

#include "../test.c"
#include <string.h>
#include <sys/syslog.h>

TEST_TEST_FUNCTION(properties_parse){
	char settingsString[] = "Version: 1.0.0\n\n# Server\nport = 1869\ndaemonize = true\nnum_worker_threads = 1\nhttp_root_directory = /home/ex05/herder/www\ncustom_error_pages_directoory = /home/ex05/herder/error_pages\nlogfile_directory = /home/ex05/herder/log\nepoll_event_buffer_size = 32\nhttp_cache_size = 256\nerror_page_cache_size = 4\n// Max architecture independant guaranteed size is 2pow(16) or 65_535 Bytes.\nhttp_read_buffer_size = 8096\n\n# Security\nssl_privateKeyFile = ./res/testCertificate.pem\nssl_certificate = ./res/testCertificate.pem\n\nwork_directory = /home/exo5/herderTEST\nsystem_log_id = herder_server";

	PropertyFile properties = {0};
	if(properties_parse(&properties, settingsString, strlen(settingsString)) != ERROR_NO_ERROR){
		return TEST_FAILURE("%s", "Failed to parse settings string.");
	}

	properties_free(&properties);

	return TEST_SUCCESS;
}

#endif