#include <stdio.h>
#include <unistd.h>
#include <regex.h>
#include "ap_config.h"
#include "ap_provider.h"
#include "httpd.h"
#include "http_core.h"
#include "http_config.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"

//Nothing will be logged in the DEBUG file if this is undefined:
#define MANIFEST_DEBUG_MODE

//There shall be no Log of Damnation if you remove this definition:
#define DAMN_ABUSERS

#define LINE_CACHE_BUCKET_SIZE 37
#define LINE_CACHE_CLEAR_FREQ 3600
#define LINE_CACHE_THRESHOLD 300
#define STATIC_FILE_CLEAR_FREQ 3600
#define FILE_TOTAL_CACHE_THRESHOLD 1000
#define FILE_PTR_CACHE_THRESHOLD 600
#define BLOCK_SIZE 100
#define MANIFEST_LINE 1000
#define FORMAT_RESULT_SIZE 1000
#define PATH_DESCRIPTOR_LENGTH 500
#define BLACKLIST_THRESHOLD 50
#define BLACKLIST_CLEAR_FREQUENCY 300
#define DEFAULT_DOS_BUCKET_SIZE 30

/*
  Copyright (c) 2013 Anthony Bau and Exeter Computing Club.
*/

//Debugging file:
FILE* DEBUG;

//Access log:
FILE* ACCESS_LOG;

//Error log:
FILE* ERROR_LOG;

/*
  THE LOG OF DAMNATNION
    Those who make this log shall forever be condemned to an afterlife of wandering this barren earth, lusting after the contentment and fulfillment that once could have been theirs, thin wisps in the air, insubstantial as they gaze listlessly at the happy faces on this gray planet, their minds gutted and filled with glop, never resting but floating horribly about the burning ether.

    Basically for IP addresses who try to abuse our HTTP server.
*/
FILE* LOG_OF_DAMNATION;

/*========================
 * HIT LIST FUNCTIONS for caching and DOS evasion
 *========================*/

struct hit_list_node;

typedef struct hit_list_node hit_list_node;

struct hit_list_node {
  char* key;
  int value;
  hit_list_node* next;
};

int hash(const char* string) {
  int r = 0;
  for (int i = 0; string[i] != '\0'; ++i) r = r * 31 + string[i];
  return r;
}

typedef struct {
  hit_list_node** buckets;
  int size;
  time_t last_cleared;
} hit_list;

static void hit_list_clear(hit_list* map) {
#ifdef MANIFEST_DEBUG_MODE
  fputs("Clearing a map.\n", DEBUG);
  fflush(DEBUG);
#endif

  int size = map->size;
  for (int i = 0; i < size; ++i) {
    hit_list_node* head = map->buckets[i];
    if (head == NULL) continue;
    else {
      //Free every element here:
      hit_list_node* foot = head->next;
      while (foot != NULL) {
        free(head->key);
        free(head);
        head = foot;
        foot = head->next;
      }
    }
  }
}

static int hit_list_increment(hit_list* map, const char* key, int clear_freq) {
  //If it's time to refresh, do so and automatically clear this requester:
  if (difftime(time(NULL), map->last_cleared) > clear_freq) {
    hit_list_clear(map);
    return 0;
  }

  int incorrect = 0, index;
  hit_list_node* head = map->buckets[(index = hash(key) % map->size)];

  //If there is nothing in this bucket yet, put something there:
  if (head == 0) {
    //Make a new element:
    hit_list_node* new_element = (hit_list_node*) malloc (sizeof(hit_list_node));
    
    //Fill the new element in with the correct values:
    new_element->key = strdup(key);
    new_element->value = 1;
    new_element->next = NULL;
    
    //Install the new element:
    map->buckets[index] = new_element;

#ifdef MANIFEST_DEBUG_MODE
    fprintf(DEBUG, "This is %s's first request this clear period. Storing him at index %d.\n", key, index);
    fflush(DEBUG);
#endif

    return 1;
  }

  //Otherwise, search the list at that bucket:
  while (head->next != NULL & (incorrect = strcmp(head->key, key))) {
    head = head->next;
  }

  if (incorrect) {
    //We have reached the end of the list and found no fit:
    head->next = (hit_list_node*) malloc (sizeof(hit_list_node));
    
    //Make a new element with the correct values:
    head->next->key = strdup(key);
    head->next->value = 1;
    head->next->next = NULL;

#ifdef MANIFEST_DEBUG_MODE
    fprintf(DEBUG, "This is %s's first request this clear period.\n", key);
#endif

    return 1;
  }
  else {
#ifdef MANIFEST_DEBUG_MODE
    fprintf(DEBUG, "%s has requested %d times this period.\n", key, head->value);
#endif
    //Otherwise, this element already exists, so we can increment it.
    return (head->value += 1);
  }

  //If we've gotten here, the requester is benign.
  return 1;
}

/*==================
 *  HASHMAP FUNCTIONS for caching
 *==================*/

struct hashmap_node;

typedef struct hashmap_node hashmap_node;

struct hashmap_node {
  char* key;
  void* value;
  hashmap_node* next;
};

typedef struct {
  hashmap_node** buckets;
  int size;
  time_t last_cleared;
} hashmap;

static void hashmap_clear(hashmap* map) {
#ifdef MANIFEST_DEBUG_MODE
  fputs("Clearing a hashmap.\n", DEBUG);
  fflush(DEBUG);
#endif

  int size = map->size;
  for (int i = 0; i < size; ++i) {
    hashmap_node* head = map->buckets[i];
    if (head == NULL) continue;
    else {
      //Free every element here:
      hashmap_node* foot = head->next;
      while (foot != NULL) {
        free(head->key);
        free(head);
        head = foot;
        foot = head->next;
      }
    }
  }
}

static int hashmap_contains(hashmap* map, char* key) {
  hashmap_node* head = map->buckets[hash(key) % map->size];
  while (head != NULL) {
    if (strcmp(head->key, key) == 0) return 1;
    head = head->next;
  }
  return 0;
}

static void* hashmap_get(hashmap* map, const char* key) {
#ifdef MANIFEST_DEBUG_MODE
  fputs("Starting hashmap_get.\n", DEBUG);
  fflush(DEBUG);
#endif
  hashmap_node* head = map->buckets[hash(key) % map->size];
  while (head != NULL && strcmp(head->key, key)) head = head->next;
#ifdef MANIFEST_DEBUG_MODE
  fputs("Found the proper head value.\n", DEBUG);
  fflush(DEBUG);
#endif
if (head == NULL) return NULL;
  else return head->value;
}

static void hashmap_set(hashmap* map, const char* key, void* value) {
  int index, incorrect = 1;
  hashmap_node* head = map->buckets[(index = hash(key) % map->size)];
  if (head != NULL) while (head->next != NULL && (incorrect = strcmp(head->key, key))) head = head->next;
  if (incorrect) {
    //Create and a new node.
    hashmap_node* new_el = (hashmap_node*) malloc (sizeof(hashmap_node));
    new_el->key = strdup(key);
    new_el->value = value;
    new_el->next = NULL;

    //Install it.
    if (head == NULL) map->buckets[index] = new_el;
    else head->next = new_el;
  }
  else {
    //Set the value of the matching element.
    head->value = value;
  }
}

/*================
 * SPECIAL CACHING TYPES
 *================*/

//Represents something to execute:
typedef struct {
  int disposition;
  char* file;
} manifest_command;

//Represents a static file for reading:
typedef struct {
  FILE* file;
  char* mimetype;
  int size;
} static_file_record;

typedef struct {
  char* text;
  char* mimetype;
  int size;
} static_file_total;

/*=================
 * GLOBAL VARIABLES
 *=================*/

//A hit list for DOS evasion:
static hit_list * server_hit_list;

//A hit list for manifest line caching:
static hit_list * line_cache_record;

//A hit list for file name caching:
static hit_list * file_access_record;

//A hashmap for manifest line caching:
static hashmap * line_cache; //ALL VALUE TYPES SHOULD BE (manifest_command*)

//A hashmap for file name to file pointer caching:
static hashmap * file_ptr_cache; //ALL VALUE TYPES SHOULD BE (static_file_record*)

//A hashmap for file name to full file string caching:
static hashmap * file_text_cache; //ALL VALUE TYPES SHOULD BE (char*)

/*=================
 * SERVER TOOLS
 *=================*/

char* findMime(const char* ext) {
  FILE* mimetypes = fopen("/srv/http/conf/mime.types", "r");
  if (mimetypes != NULL) {
    int nbytes = 100;
    char* line = (char*) malloc (100 * sizeof(char));

    while (getline(&line, (size_t*) &nbytes, mimetypes) > 0) {
      //Parse a single line.
      
      //If it's commented out, we're done.
      if (line[0] == '#') continue;

      //Otherwise, continue until we see whitespace:
      char* mimetype = (char*) malloc (100 * sizeof(char));
      int i = 0;
      for (; line[i] != '\t' && line[i] != '\n'; ++i) {
        mimetype[i] = line[i];
      }
      mimetype[i] = '\0';
      
      //If we're at the end of line, we're done.
      if (line[i] == '\n') continue;
      
      //Otherwise, continue until the whitespace ends.
      for (; line[i] == '\t'; ++i);
      
      int s = 0;
      int bad = 0;
      while (line[i] != '\0') {
        if (line[i] == ' ' || line[i] == '\n') {
          //If we have found the correct mimetype, return it
          if (ext[s] == '\0' && !bad) {
            free(line);
#ifdef MANIFEST_DEBUG_MODE
            fprintf(DEBUG, "Malloced a mimetype that I'll return. It's %u.\n", mimetype);
            fflush(DEBUG);
#endif
            return mimetype;
          }
          else {
            //Otherwise, reset all our counters
            s = 0;
            bad = 0;
            ++i;
          }
        }
        else {
          bad |= (ext[s] != line[i]);
          ++s;
          ++i;
        }
      }
      
      free(mimetype);
    }
    free(line);
    return strdup("text/plain");
  }
  else {
    return strdup("text/plain");
  }
}

static int util_read (request_rec* r, const char** rbuf, apr_off_t* size) {
  int rc = OK;

  if ((rc = ap_setup_client_block(r, REQUEST_CHUNKED_ERROR)) != OK) {
    //Setup reading from POST and if we have an error, return it.
    return (rc); 
  }

  if (ap_should_client_block(r)) { //If we're okay to keep reading,
    
    char buffer[BLOCK_SIZE];
    apr_off_t rsize, len_read, rpos = 0;
    apr_off_t length = r->remaining;

    *rbuf = (const char*) apr_pcalloc (r -> pool, (apr_size_t) (length + 1)); //Allocate memory for our return value
    *size = length; //Tell the caller how much  memory we've just allocated

    while ((len_read = ap_get_client_block(r, buffer, sizeof(buffer))) > 0) {
      if (rpos + len_read > length) {
        //If we've apparently gone past the end of the stream, set our size to the remaining bytes that used to be left in th stream
        rsize = length - rpos;
      }
      else {
        //Otherwise, trust len_read.
        rsize = len_read;
      }
      memcpy((char*) *rbuf + rpos, buffer, (size_t) rsize); //Put what we just read into the return memory.
      rpos += rsize; //Move forward.
    }
  }

  return (rc);
}

/*===============
 * FILE RUNNING FUNCTIONS
 *===============*/

static int run_dynamic(request_rec* r, const char* file) {
#ifdef MANIFEST_DEBUG_MODE
  fprintf(DEBUG, "Running %s dynamically.\n", file);
  fflush(DEBUG);
#endif

  //Declare pipes:
  int stdin_pipe[2];
  int stdout_pipe[2];
  int stderr_pipe[2];

  //Make pipes:
  if (pipe2(stdin_pipe, O_NONBLOCK) < 0) return HTTP_INTERNAL_SERVER_ERROR;
  if (pipe2(stdout_pipe, O_NONBLOCK) < 0) return HTTP_INTERNAL_SERVER_ERROR;
  if (pipe2(stderr_pipe, O_NONBLOCK) < 0) return HTTP_INTERNAL_SERVER_ERROR;

  //Fork ourselves:
  int pid = fork();
  if (pid < 0) {
    //We have an error:
    return HTTP_INTERNAL_SERVER_ERROR;
  }
  else if (pid == 0) {
    //We are the client:
    close(stdin_pipe[1]);
    close(stdout_pipe[0]);

    //Redirect pipe ends:
    if (dup2(stdin_pipe[0], STDIN_FILENO) == -1) {
      return HTTP_INTERNAL_SERVER_ERROR;
    }
    if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1) {
      return HTTP_INTERNAL_SERVER_ERROR;
    }
    if (dup2(stderr_pipe[1], STDERR_FILENO) == -1) {
      return HTTP_INTERNAL_SERVER_ERROR;
    }
    
    //Set environment variables:
    setenv("PATH_INFO", r->uri, 1);
    setenv("QUERY_STRING", r->args, 1);
    setenv("REQUEST_METHOD", r->method, 1);

    //Run the needed script.
    char* nargs[] = {(char*) file, NULL};
    execvp(nargs[0], nargs);
  }
  else {
    //We are the parent:
    int status;
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    const apr_array_header_t* fields;
    apr_table_entry_t* e = 0;

    //Put out all the headers we got.
    fields = apr_table_elts(r->headers_in);
    e = (apr_table_entry_t*) fields->elts;
    for (int i = 0; i < fields->nelts; ++i) {
      if (write(stdin_pipe[1], e[i].key, strlen(e[i].key)) < 0) return HTTP_INTERNAL_SERVER_ERROR;
      if (write(stdin_pipe[1], ":", 1) < 0) return HTTP_INTERNAL_SERVER_ERROR;
      if (write(stdin_pipe[1], e[i].val, strlen(e[i].val)) < 0) return HTTP_INTERNAL_SERVER_ERROR;
      if (write(stdin_pipe[1], "\n", 1) < 0) return HTTP_INTERNAL_SERVER_ERROR;
    }
    if (write(stdin_pipe[1], "\n", 1) < 0) return HTTP_INTERNAL_SERVER_ERROR;
    
    if (strcmp("POST", r->method) == 0) {
      //Read the post data:
      apr_off_t size; 
      const char* buf;
      util_read (r, &buf, &size);
      
      //Write it to the child:
      if (write(stdin_pipe[1], buf, (size_t) size) < 0) return HTTP_INTERNAL_SERVER_ERROR;
    }

    //Wait for the child to finish:
    waitpid(pid, &status, 0);

    //Parse headers:
    char c, l;
    char header_name[100];
    char header_value[100];
    int modifying_header_name = 1, after_colon = 0, after_newline = 0, headers_ended_properly = 0, s = 0;
    while (read(stdout_pipe[0], &c, 1) > 0) {
      if (after_colon) {
        //Skip the whitespace after colons.
        if (c == ' ' || c == '\t') continue;
        else after_colon = 0;
      }
      if (c == ':') {
        //Finalize header name and reset s:
        header_name[s] = '\0';
        s = 0;

        //Set flags to skip whitespace and move on to modifying the header value:
        after_colon = 1;
        modifying_header_name = 0;
      }
      else if (c == '\n') {
        //If we have \n\n, stop our header parsing and move on:
        if (after_newline) {
          headers_ended_properly = 1;
          break;
        }
        else {
          //Otherwise, finalize and set the preceeding header value:
          header_value[s] = '\0';
          apr_table_set(r->headers_out, header_name, header_value);

          //Then reset our flags and parse the next line.
          s = 0;
          modifying_header_name = 1;
          after_newline = 1;
        }
      }
      else if (modifying_header_name) {
        header_name[s] = c;
        ++s;
      }
      else {
        header_value[s] = c;
        ++s;
      }
      
      //Unset the newline flag if we're not after a newline:
      if (c != '\n') after_newline = 0;
    }

    //If we don't get to the end of the headers, say so:
    if (!headers_ended_properly) {
#ifdef MANIFEST_DEBUG_MODE
      fputs("End of script before headers.", DEBUG);
      fflush(DEBUG);
#endif
      return HTTP_INTERNAL_SERVER_ERROR;
    }
    
    //Then read out the entire file.
    while (read(stdout_pipe[0], &c, 1) > 0) {
      ap_rputc(c, r);
    }

    return OK;
  }
}

static int run_static(request_rec* r, const char* filename) {
#ifdef MANIFEST_DEBUG_MODE
  fprintf(DEBUG, "Running static file %s.\n", filename);
  fflush(DEBUG);
#endif
  
  //See if we are cached:
  int hit_list_result;
  static_file_record* finfo;
  static_file_total* t_finfo;
  if ((hit_list_result = hit_list_increment(file_access_record, filename, STATIC_FILE_CLEAR_FREQ)) >= FILE_PTR_CACHE_THRESHOLD) {
    if (hit_list_result >= FILE_TOTAL_CACHE_THRESHOLD && (t_finfo = (static_file_total*) hashmap_get(file_text_cache, filename)) != NULL) {
#ifdef MANIFEST_DEBUG_MODE
      fputs("Supercache hit!\n", DEBUG);
      fflush(DEBUG);
#endif
      //Total file cache hit.
      ap_set_content_type(r, t_finfo->mimetype);
      ap_set_content_length(r, t_finfo->size);
      ap_rwrite(t_finfo->text, t_finfo->size, r);
      return OK;
    }
    else if ((finfo = (static_file_record*) hashmap_get(file_ptr_cache, filename)) != NULL) {
#ifdef MANIFEST_DEBUG_MODE
      fputs("Static file cache hit (not super)!\n", DEBUG);
      fflush(DEBUG);
#endif
      //File descriptor cache hit.
      ap_set_content_type(r, finfo->mimetype);
      ap_set_content_length(r, finfo->size);
      char* buf = (char*) malloc (finfo->size * sizeof(char));
      int bytes_read;
      if (bytes_read = fread(buf, finfo->size, 1, finfo->file) < finfo->size) finfo->size = bytes_read;
      rewind(finfo->file);
      ap_rwrite(buf, bytes_read, r);
      
      //If we're supposed to be supercached right now, supercache us:
      if (hit_list_result >= FILE_TOTAL_CACHE_THRESHOLD) {
        t_finfo = (static_file_total*) malloc (sizeof(static_file_total));
        t_finfo->size = bytes_read;
        t_finfo->text = buf;
        t_finfo->mimetype = finfo->mimetype;
        hashmap_set(file_text_cache, filename, t_finfo);
      }

      return OK;
    }
  }
  
  fputs("Apparently no cache hit... :(\n", DEBUG);
  fflush(DEBUG);
  
  //If we get here, we are apparently not cached.

  //Find the extension:
  char extension[20];
  char* dot_ptr = strrchr(filename, '.') + 1;
  int ext_len = filename + strlen(filename) - dot_ptr;
  memcpy(extension, dot_ptr, ext_len);
  extension[ext_len] = 0;
  char* mimetype = findMime(extension);

#ifdef MANIFEST_DEBUG_MODE
  fprintf(DEBUG, "File extension is %s. Thus mimeType is %s.\n", extension, mimetype);
  fflush(DEBUG);
#endif

  //Set the mimetype to the proper value for this extension:
  ap_set_content_type(r, mimetype);
  
/*
  //TODO Get this gorram thing working.
  //Open the file and send it:
  apr_file_t* file = 0;
  int* sent;
  char buf[30];
  int result = 0;
  result = apr_file_open(&file, filename, APR_READ | APR_BINARY, APR_OS_DEFAULT, r->pool);

#ifdef MANIFEST_DEBUG_MODE
  fprintf(DEBUG, "apr_file_open came back with result code %s (%s).\n", apr_strerror(result, buf, 30), buf, file);
  fflush(DEBUG);
#endif

  //TODO Get this gorram thing working.

  apr_finfo_t file_info;
  result = apr_file_info_get(&file_info, APR_FINFO_SIZE, file);

#ifdef MANIFEST_DEBUG_MODE
  fprintf(DEBUG, "apr_file_info_get came back with result code %s (%s) (file size looks like %d; rfinfo says %d).\n", apr_strerror(result, buf, 30), buf, file_info.size, r->finfo.size);
  fflush(DEBUG);
#endif

  ap_set_content_length(r, file_info.size);
  ap_send_fd(file, r, 0, 3964, (apr_size_t*) sent);
*/

  FILE* file = fopen(filename, "rb");
  
  //Get file length
  fseek(file, 0L, SEEK_END);
  int size = ftell(file);
  rewind(file);
  
  char* buffer = (char*) malloc (size * sizeof(char));
  int true_size = fread(buffer, size, 1, file);

  //If we should be cached right now, cache us:
  if (hit_list_result >= FILE_PTR_CACHE_THRESHOLD) {
#ifdef MANIFEST_DEBUG_MODE
    fputs("It's about time we cached this thing.\n", DEBUG);
    fflush(DEBUG);
#endif

    finfo = (static_file_record*) malloc (sizeof(static_file_record*));
    finfo->file = file;
    finfo->size = size;
    finfo->mimetype = strdup(mimetype);
    hashmap_set(file_ptr_cache, filename, finfo);
  }

#ifdef MANIFEST_DEBUG_MODE
  fprintf(DEBUG, "Almost done! Going to write %d bytes of \n%s\nto the user.\n", size, buffer);
  fflush(DEBUG);
#endif

  ap_rwrite(buffer, size, r);

  free(buffer);
  free(mimetype);

  return OK;
}

/*================
 * THE MANIFEST PARSER
 *================*/

int matches(regmatch_t** backrefs, char* form, char* path, char* match) {
  regex_t compiled;

  //Count the number of backreferences needed
  int nbackrefs = 0;
  for (int i = 0; form[i] != '\0'; ++i) {
    if (form[i] == '$') {
      char n[3];
      ++i;
      for (int s = 0; form[i] != '$'; ++i & ++s) n[s] = form[i];
      ++i;
      n[i] = '\0';
      int m;
      if ((m = atoi(n)) > nbackrefs) {
        nbackrefs = m;
      }
    }
  }

  //Make our backrefs array
  *backrefs = (regmatch_t*) malloc ((nbackrefs + 1) * sizeof(regmatch_t));
  
  int rc;
  if ((rc = regcomp(&compiled, match, REG_EXTENDED))) {
    //If we have an error, return it.
    return (rc);
  }
  else {
    return (regexec(&compiled, path, nbackrefs + 1, *backrefs, 0) == 0);
  }
}

const char* format(regmatch_t* backref, char* format, char* path) {
  //Allocate data for the return value:
  char* result = (char*) malloc (FORMAT_RESULT_SIZE * sizeof(char));
  int mark = 0;

  for (int i = 0; format[i] != '\0'; ++i & ++mark) {
    if (format[i] == '$') {
      //Get the requested backref index:
      
      char n[3];
      ++i;
      for (int s = 0; format[i] != '$'; ++i & ++s) n[s] = format[i];
      ++i;
      n[i] = '\0';

      int which = atoi(n);
      char* beg_ptr = path + backref[which].rm_so;
      int size = backref[which].rm_eo - backref[which].rm_so;
      
      //Get the request backref index:
      memcpy(result + mark, beg_ptr, size);
      mark += backref[which].rm_eo - backref[which].rm_so;
      
      //Skip the following dollar sign.
      ++i;
    }
    else {
      result[mark] = format[i];
    }
  }

  //Add terminating null character and return.
  result[mark] = '\0';
  return result;
}

static int run_manifest(request_rec* r, const char* filename) {
#ifdef MANIFEST_DEBUG_MODE
  fprintf(DEBUG, "Running manifest file %s.\n", filename);
  fflush(DEBUG);
#endif
  
  FILE* f = fopen(filename, "r");
  char* path = r->uri;
  long unsigned int manifest_line = MANIFEST_LINE;
  char* line = (char*) malloc (MANIFEST_LINE * sizeof(char));

  while (getline(&line, &manifest_line, f) > 0) {
    //'#' is the comment character.
    if (line[0] == '#') continue;

    //'--CRON--' is the marker for the start of cron jobs:
    if (strcmp(line, "--CRON--\n") == 0) break;

    //Set up our marker:
    int i = 0;

    //Get the path descriptor:
    char match[PATH_DESCRIPTOR_LENGTH];
    for (; line[i] != ' ' && line[i] != '\n'; ++i) match[i] = line[i];
    match[i] = '\0';

    //If we have a misformatted line, say so:
    if (line[i] == '\0') return HTTP_INTERNAL_SERVER_ERROR;

    //Advance past the space:
    ++i;
    
    //Declare stuff
    int s = 0;
    regmatch_t* backref;
    char* new_file;

    //Assemble the format string:
    char form[100];
    for (; line[i] != ' ' && line[i] != '\0'; ++i & ++s) form[s] = line[i];
    form[s] = '\0';

    //Again, if the line is misformatted, say so:
    if (line == '\0') return HTTP_INTERNAL_SERVER_ERROR;

    //Check if we match
    if (matches(&backref, form, r->uri, match)) {
      //If we do, format our path
      new_file = (char*) format(backref, form, r->uri);
      free(backref);
    }
    else {
      free(backref);
      continue;
    }
    
    //Then run the formatted file with the appropriate disposition
    int rc;
    switch (line[i+1]) {
      case 'M':
        rc = run_manifest(r, new_file);
        break;
      case 'D':
        //Cache this line:
        if (!hashmap_contains(line_cache, r->uri) && hit_list_increment(line_cache_record, r->uri, LINE_CACHE_CLEAR_FREQ)) {
          manifest_command * cached_command;
          cached_command->disposition = 1;
          cached_command->file = strdup(new_file);
          hashmap_set(line_cache, r->uri, new_file);
        }
        rc = run_dynamic(r, new_file);
        break;
      default:
        //Cache this line:
        if (!hashmap_contains(line_cache, r->uri) && hit_list_increment(line_cache_record, r->uri, LINE_CACHE_CLEAR_FREQ)) {
          manifest_command * cached_command;
          cached_command->disposition = 0;
          cached_command->file = strdup(new_file);
          hashmap_set(line_cache, r->uri, new_file);
        }
        rc = run_static(r, new_file);
        break;
    }

    free (new_file);

    return rc;
  }
  
  //If there is no such manifest line, says so:
  return HTTP_NOT_FOUND;
}

inline hit_list* create_hit_list(int size) {
  hit_list* new_hit_list = (hit_list *) malloc (sizeof(hit_list));
  new_hit_list->buckets = (hit_list_node **) calloc (DEFAULT_DOS_BUCKET_SIZE, sizeof(hit_list_node*));
  new_hit_list->size = DEFAULT_DOS_BUCKET_SIZE;
  new_hit_list->last_cleared = time(NULL);
  return new_hit_list;
}

inline hashmap* create_hashmap(int size) {
  hashmap* new_hashmap = (hashmap *) malloc (sizeof(hashmap));
  new_hashmap->buckets = (hashmap_node**) calloc (size, sizeof(hashmap_node*));
  new_hashmap->size = size;
  new_hashmap->last_cleared = time(NULL);
  return new_hashmap;
}

static void* setup_req_densities(apr_pool_t *p, server_rec *s) {
  
  //Create the hit lists
  server_hit_list = create_hit_list(DEFAULT_DOS_BUCKET_SIZE);
  line_cache_record = create_hit_list(DEFAULT_DOS_BUCKET_SIZE);
  file_access_record = create_hit_list(DEFAULT_DOS_BUCKET_SIZE);
  
  //Create the caches
  line_cache = create_hashmap(LINE_CACHE_BUCKET_SIZE);
  file_ptr_cache = create_hashmap(LINE_CACHE_BUCKET_SIZE);
  file_text_cache = create_hashmap(LINE_CACHE_BUCKET_SIZE);
}

/*=================
 * APACHE REGISTRATON STUFF
 *=================*/

static int manifester(request_rec* r) {
#ifdef MANIFEST_DEBUG_MODE
  fprintf(DEBUG, "\nPID %d (name %s) handled request %s from %s.\n", getpid(), r->server->process->argv[0], r->uri, r->connection->client_ip);
  fflush(DEBUG);
#endif


  //REQUEST HANDLING STEP 1: DOS EVASION

  if (hit_list_increment(server_hit_list, r->connection->client_ip, BLACKLIST_CLEAR_FREQUENCY) > BLACKLIST_THRESHOLD) {
    return HTTP_FORBIDDEN;
  }
  
  fputs("Benign requester.\n", DEBUG);
  fflush(DEBUG);

  //REQUEST HANDLING STEP 2: CHECK CACHE HIT
  manifest_command* cached_command;
  if ((cached_command = (manifest_command*) hashmap_get(line_cache, r->uri)) != NULL) return (cached_command->disposition == 0 ? run_static(r, cached_command->file) : (cached_command->disposition == 1 ? run_dynamic(r, cached_command->file) : HTTP_INTERNAL_SERVER_ERROR));

  fputs("No cache hit.\n", DEBUG);
  fflush(DEBUG);

  //REQUEST HANDLING STEP 3: RUN MANIFEST FILE
  return run_manifest(r, "/srv/http/manifest.txt");
}

static void register_hooks(apr_pool_t* pool) {
#ifdef MANIFEST_DEBUG_MODE
  DEBUG = fopen("/srv/http/logs/manifester.debug", "w");
#endif

#ifdef DAMN_ABUSERS
  LOG_OF_DAMNATION = fopen("/srv/http/logs/damnation.log", "w");
#endif
  ap_hook_handler(manifester, NULL, NULL, APR_HOOK_LAST);
}

module AP_MODULE_DECLARE_DATA manifester_module = {
  STANDARD20_MODULE_STUFF,
  NULL,
  NULL,
  setup_req_densities,
  NULL,
  NULL,
  register_hooks
};
