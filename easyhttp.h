#ifndef EASY_HTTP_H
#define EASY_HTTP_H

// Including Libraries
#include "easyfile.h"
#include "easysocket.h"
#include "easystring.h"

// Defs

// EasyCSSEmbedder Structure
typedef struct {
    EasyString* path;
    EasyString* html;
    int startIndex;
    int endIndex;
}EasyCSSEmbedder;

// EasyHTTPRequest Structure
typedef struct {
    EasyString* requestLine;
    EasyStringSet* headers;
    EasyString* body;
}EasyHTTPRequest;

// EasyHTTPResponse Structure
typedef struct {
    EasyString* statusLine;
    EasyStringSet* headers;
    EasyString* body;
}EasyHTTPResponse;

// Functions Pre-Declaration: EasyCSSEmbedder
int http_html_header_css_check_integrity(const EasyCSSEmbedder* cssEmbedder);
EasyCSSEmbedder* http_html_header_css_init(const EasyString* html, const EasyString* path);
EasyString* http_html_header_css_embed(const EasyCSSEmbedder* cssEmbedder);
EasyString* http_html_header_css_direct_embed(const EasyString* html, const EasyString* path);
void http_html_header_css_delete(EasyCSSEmbedder** cssEmbedder);

// Functions Pre-Declaration: EasyHTTPRequest
int http_get_request_check_integrity(const EasyHTTPRequest* request);
int http_get_request_check_full_integrity(const EasyHTTPRequest* request);
EasyHTTPRequest* http_get_request_parse(const EasyString* request);
EasyString* http_get_request_parse_path(const EasyHTTPRequest* request);
EasyStringArray* http_get_request_parse_query(const EasyHTTPRequest* request);
EasyStringSet* http_get_request_headers(const EasyHTTPRequest* request);
EasyString* http_get_request_body(const EasyHTTPRequest* request);
void http_get_request_delete(EasyHTTPRequest** request);

// Functions Pre-Declaration: EasyHTTPResponse
int http_get_response_check_integrity(const EasyHTTPResponse* response);
int http_get_response_check_full_integrity(const EasyHTTPResponse* response);
EasyHTTPResponse* http_get_response_parse(const EasyString* response);

// Functions Declaration: EasyCSSEmbedder
int http_html_header_css_check_integrity(const EasyCSSEmbedder* cssEmbedder) {
    return (cssEmbedder == NULL || cssEmbedder->startIndex<0 || cssEmbedder->endIndex<cssEmbedder->startIndex || !string_check_integrity(cssEmbedder->path) || !string_check_integrity(cssEmbedder->html)) == 0;
}
EasyCSSEmbedder* http_html_header_css_init(const EasyString* html, const EasyString* path) {
    if(!string_check_integrity(path) || !file_exist(path) || !string_check_integrity(html)) return NULL;

    int pathStartPosition = string_first_occurrence_index(html, path);

    if(pathStartPosition == -1) return NULL;

    int startIndex;
    for(startIndex = pathStartPosition; startIndex>=0; startIndex--) {
        if(html->string[startIndex] == '<') break;
    }

    int endIndex;
    for(endIndex=pathStartPosition; endIndex<html->length; endIndex++) {
        if(html->string[endIndex]=='>') break;
    }

    EasyCSSEmbedder* cssEmbedder = (EasyCSSEmbedder*) malloc(sizeof(EasyCSSEmbedder));
    cssEmbedder->startIndex = startIndex;
    cssEmbedder->endIndex = endIndex;
    cssEmbedder->path = string_clone(path);
    cssEmbedder->html = string_clone(html);

    return cssEmbedder;    

}
EasyString* http_html_header_css_embed(const EasyCSSEmbedder* cssEmbedder) {
    if(!http_html_header_css_check_integrity(cssEmbedder)) return NULL;
    EasyString* oldSequence = string_substring(cssEmbedder->html, cssEmbedder->startIndex, cssEmbedder->endIndex+1);
    EasyString* newSequence = string_init();
    string_append_c_str(newSequence, "<style>");
    EasyFile* cssFile = file_text_open(cssEmbedder->path, EASY_FILE_MODE_TEXT_READ_ONLY);
    file_text_read_all(cssFile, newSequence);
    file_close(&cssFile);
    string_append_c_str(newSequence, "</style>");
    EasyString* newHTML = string_replace_first(cssEmbedder->html, oldSequence, newSequence);
    string_delete(&oldSequence);
    string_delete(&newSequence);
    return newHTML;
}
EasyString* http_html_header_css_direct_embed(const EasyString* html, const EasyString* path) {
    EasyCSSEmbedder* embedder = http_html_header_css_init(html, path);
    if(embedder == NULL) return NULL;
    EasyString* result = http_html_header_css_embed(embedder);
    http_html_header_css_delete(&embedder);
    return result;
}
void http_html_header_css_delete(EasyCSSEmbedder** cssEmbedder) {
    if(cssEmbedder == NULL) return;
    if(!http_html_header_css_check_integrity(*cssEmbedder)) return;
    (*cssEmbedder)->endIndex = -1;
    (*cssEmbedder)->startIndex = -1;
    string_delete(&((*cssEmbedder)->path));
    string_delete(&((*cssEmbedder)->html));
    *cssEmbedder = NULL;
}

// Functions Declaration: EasyHTTPRequest
int http_get_request_check_integrity(const EasyHTTPRequest* request) {
    return (request == NULL || !string_check_integrity(request->body) || !string_set_check_integrity(request->headers) || !string_check_integrity(request->requestLine)) == 0;
}
int http_get_request_check_full_integrity(const EasyHTTPRequest* request) {
    return (request == NULL || !string_check_integrity(request->body) || !string_set_check_full_integrity(request->headers) || !string_check_integrity(request->requestLine)) == 0;
}
EasyHTTPRequest* http_get_request_parse(const EasyString* request) {
    if(!string_check_integrity(request)) return NULL;

    int i;

    EasyHTTPRequest* requestPacket = (EasyHTTPRequest*) malloc(sizeof(EasyHTTPRequest));

    EasyStringArray* splitRequest = string_split_c_str(string_c_str(request), "\r\n");

    requestPacket->requestLine = string_init_with_string(string_c_str(string_array_get(splitRequest, 0)));
    requestPacket->headers = string_set_init();
    requestPacket->body = string_init_with_string("");

    for(i=1;i<splitRequest->length;i++) {
        if(string_equals_c_str(string_c_str(string_array_get(splitRequest, i)), "")) {
            break;
        }
        string_set_add(requestPacket->headers, string_array_get(splitRequest, i));
    }

    string_array_delete(&splitRequest);

    return requestPacket;
}
EasyString* http_get_request_parse_path(const EasyHTTPRequest* request) {
    if(!http_get_request_check_integrity(request)) return NULL;
    EasyStringArray* splitRequestLine = string_split_c_str(string_c_str(request->requestLine), " ");
    EasyString* resAndQuery = string_array_get(splitRequestLine, 1);
    string_array_delete(&splitRequestLine);
    EasyString* path;
    if(string_contains_c_str(string_c_str(resAndQuery), "?")) {
        EasyStringArray* tempSplit = string_split_c_str(string_c_str(resAndQuery), "?");
        path = string_array_get(tempSplit, 0);
        string_array_delete(&tempSplit);
        string_delete(&resAndQuery);
    }else{
        path = resAndQuery;
    }
    return path;
}
EasyStringArray* http_get_request_parse_query(const EasyHTTPRequest* request) {
    if(!http_get_request_check_integrity(request)) return NULL;

    int i;

    EasyStringArray* splitRequestLine = string_split_c_str(string_c_str(request->requestLine), " ");

    EasyString* resAndQuery = string_array_get(splitRequestLine, 1);
    string_array_delete(&splitRequestLine);

    EasyStringArray* splitResAndQuery = string_split_c_str(string_c_str(resAndQuery), "?");
    EasyStringArray* query;
    string_delete(&resAndQuery);
    if(string_array_length(splitResAndQuery) <= 1) {
        string_array_delete(&splitResAndQuery);
        query = string_array_init(1);
        query->stringArray[0] = string_init_with_string("");
        return query;
    }else {
        EasyString* queryStr = string_array_get(splitResAndQuery, 1);
        string_array_delete(&splitResAndQuery);
        query = string_split_c_str(string_c_str(queryStr), "&");
        string_delete(&queryStr);
        return query;
    }
}
EasyStringSet* http_get_request_headers(const EasyHTTPRequest* request) {
    if(!http_get_request_check_integrity(request)) return NULL;
    return string_set_clone(request->headers);
}
EasyString* http_get_request_body(const EasyHTTPRequest* request) {
    if(!http_get_request_check_integrity(request)) return NULL;
    return string_clone(request->body);
}
void http_get_request_delete(EasyHTTPRequest** request) {
    if(request == NULL || !http_get_request_check_integrity(*request)) return;
    string_delete(&((*request)->body));
    string_delete(&((*request)->requestLine));
    string_set_delete(&((*request)->headers));
    *request = NULL;

}

// Functions Declaration: EasyHTTPResponse
int http_get_response_check_integrity(const EasyHTTPResponse* response) {
    return (response == NULL || !string_check_integrity(response->body) || !string_set_check_integrity(response->headers) || !string_check_integrity(response->statusLine)) == 0;
}
int http_get_response_check_full_integrity(const EasyHTTPResponse* response) {
    return (response == NULL || !string_check_integrity(response->body) || !string_set_check_full_integrity(response->headers) || !string_check_integrity(response->statusLine)) == 0;
}
EasyHTTPResponse* http_get_response_parse(const EasyString* response) {
    if(!string_check_integrity(response)) return NULL;

    int i;

    EasyHTTPResponse* responsePacket = (EasyHTTPResponse*) malloc(sizeof(EasyHTTPResponse));

    EasyStringArray* splitResponse = string_split_c_str(string_c_str(response), "\r\n");

    responsePacket->statusLine = string_init_with_string(string_c_str(string_array_get(splitResponse, 0)));
    responsePacket->headers = string_set_init();
    responsePacket->body = string_init_with_string("");

    for(i=1;i<splitResponse->length;i++) {
        if(string_equals_c_str(string_c_str(string_array_get(splitResponse, i)), "")) {
            break;
        }
        string_set_add(responsePacket->headers, string_array_get(splitResponse, i));
    }

    string_array_delete(&splitResponse);

    return responsePacket;
}

#endif