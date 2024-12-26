#include "http_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define OUTPUT_RESPONSE_NAME "response"
static FILE* pOutFile = NULL;

static void generateOutputFileName(char* buffer, size_t bufferSize) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buffer, bufferSize, OUTPUT_RESPONSE_NAME "_%Y%m%d%H%M%S.bin", t);
}

static int httpParserOnMessageBeginCallback(http_parser* pHttpParser) {
    unsigned int type = pHttpParser->type;
    printf("++++++++++++++++ Parser %s Start ++++++++++++++++\n", 
                (type == HTTP_REQUEST) ? "REQUEST" : (type == HTTP_RESPONSE) ? "RESPONSE" : "BOTH");
    return 0;
}

/*-----------------------------------------------------------*/

static int httpParserOnUrlCallback(http_parser* pHttpParser,
                                   const char*  pLoc,
                                   size_t       length) {
    (void)pHttpParser;
    printf("Parsing: url=%.*s\n", (int)length, pLoc);
    return 0;
}

static int httpParserOnStatusCallback(http_parser* pHttpParser,
                                      const char*  pLoc,
                                      size_t       length) {
    (void)pHttpParser;
    printf("Parsing: status=%.*s\n",
           (int)length, pLoc);
    return 0;
}

/*-----------------------------------------------------------*/

static int httpParserOnHeaderFieldCallback(http_parser* pHttpParser,
                                           const char*  pLoc,
                                           size_t       length) {
    (void)pHttpParser;
    printf("Parsing: header field=%.*s\n",
           (int)length, pLoc);
    return 0;
}

/*-----------------------------------------------------------*/

static int httpParserOnHeaderValueCallback(http_parser* pHttpParser,
                                           const char*  pLoc,
                                           size_t       length) {
    (void)pHttpParser;
    printf("Parsing: header value=%.*s\n", (int)length, pLoc);
    return 0;
}

/*-----------------------------------------------------------*/

static int httpParserOnHeadersCompleteCallback(http_parser* pHttpParser) {
    (void)pHttpParser;
    printf("Parsing: end of the headers\n");
    return 0;
}

/*-----------------------------------------------------------*/

static int httpParserOnBodyCallback(http_parser* pHttpParser,
                                    const char*  pLoc,
                                    size_t       length) {
    (void)pHttpParser;
    printf("Parsing: response body=%lu\n", (unsigned long)length);

    if (!pOutFile) {
        char outputFileName[256];
        generateOutputFileName(outputFileName, sizeof(outputFileName));
        printf("Output file name: %s\n", outputFileName);
        pOutFile = fopen(outputFileName, "wb");
        if (pOutFile == NULL) {
            printf("Open output file fail\n");
            return -1;
        }
    }

    fwrite(pLoc, 1, length, pOutFile);
    return 0;
}

static int httpParserOnMessageCompleteCallback(http_parser* pHttpParser) {
    unsigned int type = pHttpParser->type;
    printf("++++++++++++++++ Parser %s End ++++++++++++++++\n\n", 
                (type == HTTP_REQUEST) ? "REQUEST" : (type == HTTP_RESPONSE) ? "RESPONSE" : "BOTH");
    if (pOutFile) {
        fclose(pOutFile);
    }
    return 0;
}

static int httpParserOnChunkHeaderCallback(http_parser* pHttpParser) {
    (void)pHttpParser;
    printf("Parsing: chunk header\n");
    return 0;
}

static int httpParserOnChunkCompleteCallback(http_parser* pHttpParser) {
    (void)pHttpParser;
    printf("Parsing: chunk complete\n");
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Useage: %s <File Path>\n", argv[0]);
        return -1;
    }

    char*                file_path = argv[1];
    http_parser          parser;
    http_parser_settings settings;
    FILE*                pInFile      = NULL;
    long                 file_size    = 0;
    char*                file_content = NULL;
    size_t               read_size    = 0;
    size_t               parsed       = 0;

    if (access(file_path, F_OK) != 0) {
        printf("file does not exist: %s\n", file_path);
        return -1;
    }

    pInFile = fopen(file_path, "rb");
    if (pInFile == NULL) {
        printf("open file: %s fail\n", file_path);
        return -1;
    }

    fseek(pInFile, 0, SEEK_END);
    file_size = ftell(pInFile);
    fseek(pInFile, 0, SEEK_SET);

    printf("File size: %ld\n", file_size);

    file_content = (char*)malloc(file_size);
    if (file_content == NULL) {
        printf("Malloc content memory %ld fail\n", file_size);
        fclose(pInFile);
        return -1;
    }

    settings.on_message_begin    = httpParserOnMessageBeginCallback;
    settings.on_url              = httpParserOnUrlCallback;
    settings.on_status           = httpParserOnStatusCallback;
    settings.on_header_field     = httpParserOnHeaderFieldCallback;
    settings.on_header_value     = httpParserOnHeaderValueCallback;
    settings.on_headers_complete = httpParserOnHeadersCompleteCallback;
    settings.on_body             = httpParserOnBodyCallback;
    settings.on_message_complete = httpParserOnMessageCompleteCallback;
    settings.on_chunk_header     = httpParserOnChunkHeaderCallback;
    settings.on_chunk_complete   = httpParserOnChunkCompleteCallback;

    read_size = fread(file_content, 1, file_size, pInFile);
    fclose(pInFile);
    if (read_size != file_size) {
        printf("Read file content fail\n");
        free(file_content);
        return -1;
    }

    http_parser_init(&parser, HTTP_BOTH);

    parsed = http_parser_execute(&parser, &settings, file_content, file_size);
    if ((parsed > 0 && parsed < file_size) &&
        parser.type == HTTP_REQUEST &&
        parser.http_errno == HPE_INVALID_METHOD) {
        printf("Parse Request End, try to Parse Response\n\n");

        http_parser_init(&parser, HTTP_RESPONSE);

        int cost_size = parsed - 2;

        parsed = http_parser_execute(&parser, &settings, file_content + cost_size, file_size - cost_size);

        printf("Parsed size: %ld, file size: %ld, errno: %d, %s\n",
                parsed, file_size, parser.http_errno, http_errno_description(parser.http_errno));
    } else {
        printf("Parsed size: %ld, file size: %ld, errno: %d, %s\n",
                parsed, file_size, parser.http_errno, http_errno_description(parser.http_errno));
    }

    free(file_content);
    return 0;
}
