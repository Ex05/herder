#ifndef CONSTANTS_H
#define CONSTANTS_H

#define CONSTANTS_PROPERTY_FILE_SECTION_SERVER "Server"
#define CONSTANTS_PROPERTY_FILE_SECTION_SECURITY "Security"
#define CONSTANTS_PORT_PROPERTY_NAME "port"
#define CONSTANTS_NUM_WORKER_THREADS_PROPERTY_NAME "num_worker_threads"
#define CONSTANTS_EPOLL_EVENT_BUFFER_SIZE_PROPERTY_NAME "epoll_event_buffer_size"
#define CONSTANTS_HTTP_READ_BUFFER_SIZE_PROPERTY_NAME "http_read_buffer_size"
#define CONSTANTS_SYSLOG_ID_PROPERTY_NAME "system_log_id"
#define CONSTANTS_WORK_DIRECTORY_PROPERTY_NAME "work_directory"
#define CONSTANTS_HTTP_ROOT_DIRECTORY_PROPERTY_NAME "http_root_directory"
#define CONSTANTS_HTTP_CACHE_SIZE_PROPERTY_NAME "http_cache_size"
#define CONSTANTS_ERROR_PAGE_CACHE_SIZE_PROPERTY_NAME "error_page_cache_size"
#define CONSTANTS_LOGFILE_DIRECTORY_PROPERTY_NAME "logfile_directory"
#define CONSTANTS_CUSTOM_ERROR_PAGES_DIRECTORY_PROPERTY_NAME "custom_error_pages_directoory"
#define CONSTANTS_SSL_CERTIFICATE_LOCATION_PROPERTY_NAME "ssl_certificate"
#define CONSTANTS_SSL_PRIVATE_KEY_FILE_PROPERTY_NAME "ssl_privateKeyFile"

#define CONSTANTS_DAEMONIZE_PROPERTY_NAME "daemonize"
#define CONSTANTS_DAEMONIZE_PROPERTY_DEFAULT_VALUE "false"

#define CONSTANTS_MEDIA_LIBRARY_LOCATION_PROPERTY_NAME "library_location"

#define CONSTANTS_HTTP_MAX_HEADER_FIELDS 32

#define CONSTANTS_HTTP_VERSION_1_0 "HTTP/1.0"
#define CONSTANTS_HTTP_VERSION_1_1 "HTTP/1.1"
#define CONSTANTS_HTTP_VERSION_2_0 "HTTP/2.0"

// NOTE: Find/Replace with a propper definition for this in the c standard library. (jan - 2022.09.29)
#define CONSTANTS_FILE_PATH_DIRECTORY_DELIMITER '/'

#define CONSTANTS_MINIMAL_HTTP_ERROR_PAGE "<!DOCTYPE html><html> <head> <meta charset=\"utf-8\"> <title>$errorCode</title> </head> <body> <h1>$errorCode</h1> <p>$errorMessage</p> <hr> <address>'Herder' HTTP-Web Server at <a href=\"http://$address\">$address</a> Port $port</address> </body></html>"

#define CONSTANTS_ERROR_PAGE_SEARCH_STRING_ERROR_CODE "$errorCode"
#define CONSTANTS_ERROR_PAGE_SEARCH_STRING_ERROR_MESSAGE "$errorMessage"
#define CONSTANTS_ERROR_PAGE_SEARCH_STRING_ADDRESS "$address"
#define CONSTANTS_ERROR_PAGE_SEARCH_STRING_PORT "$port"

#define CONSTANTS_HTTP_HEADER_FIELD_HOST_NAME "Host"

#define CONSTANTS_HTTP_HEADER_FIELD_SERVER_VALUE "Herder Server"

#define CONSTANTS_SERVER_USAGE_ARGUMENT_SHOW_SETTINGS_NAME "--showSettings"
#define CONSTANTS_SERVER_USAGE_ARGUMENT_SHOW_SETTINGS_DESCRIPTION "Shows a quick overview of all user definable settings."
#define CONSTANTS_SERVER_USAGE_ARGUMENT_HELP_NAME "-?, -h, --help"
#define CONSTANTS_SERVER_USAGE_ARGUMENT_HELP_DESCRIPTION "Displays this help message."

#define CONSTANTS_CHARACTER_LINE_FEED 0x0a

#define CONSTANTS_LOCK_FILE_FILE_EXTENSION ".lock"

#define CONSTANTS_TMP_SYSTEM_LOG_ID "herder_server"

#endif