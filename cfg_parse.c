#include "cfg_parse.h"

/* for malloc, EXIT_SUCCESS and _FAILURE, exit */
#include <stdlib.h>
/* for FILE*, fgets, fputs */
#include <stdio.h>
/* for memset, strlen, strchr etc */
#include <string.h>
/* for tolower, isspace */
#include <ctype.h>

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

/* implementation details of (opaque) config structures */
struct cfg_node
{
  char* key;
  char* value;

  struct cfg_node* next;
};

struct cfg_struct
{
  struct cfg_node* head;
};

/* Helper functions
    A malloc() wrapper which handles null return values */
static void* cfg_malloc(const size_t size)
{
  void* ptr;

  if (size == 0) return NULL;

  ptr = malloc(size);
  if (ptr == NULL)
  {
    perror("cfg_parse: ERROR: malloc() returned NULL");
    exit(EXIT_FAILURE);
  }

  return ptr;
}

/* Returns a duplicate of input str, without leading / trailing whitespace
    Input str *MUST* be null-terminated, or disaster will result */
static char* cfg_trim(const char* str)
{
  size_t tlen;
  char* tstr;

  /* check for null input first */
  if (str == NULL) return NULL;

  /* advance start pointer to first non-whitespace char */
  while (isspace(*str))
    str ++;

  /* roll back length until we run out of whitespace */
  tlen = strlen(str);
  while (tlen > 0 && isspace(str[tlen - 1]))
    tlen --;

  /* copy portion of string to new string */
  tstr = (char*)cfg_malloc(tlen + 1);
  tstr[tlen] = '\0';
  if (tlen > 0) memcpy(tstr, str, tlen);

  return tstr;
}

/* Returns a duplicate of input str, without leading / trailing whitespace
    Also lowercases the string, AND returns NULL instead of empty str */
static char* cfg_norm_key(const char* key)
{
  size_t i, len;
  char* tkey;

  if (key == NULL) return NULL;

  /* trim input key */
  tkey = cfg_trim(key);
  /* Exclude empty key */
  len = strlen(tkey);
  if (len == 0)
  {
    free(tkey);
    return NULL;
  }

  /* Lowercase key */
  for (i = 0; i < len; i++)
    tkey[i] = tolower(tkey[i]);

  return tkey;
}

/**
 * This function initializes a cfg_struct, and must be called before
 * performing any further operations.
 * @return Pointer to newly initialized cfg_struct object.
 */
struct cfg_struct* cfg_init()
{
  struct cfg_struct* cfg;

  cfg = (struct cfg_struct*)cfg_malloc(sizeof(struct cfg_struct));
  cfg->head = NULL;

  return cfg;
}

/**
 * This function deletes an entire cfg_struct, clearing any memory
 * previously held by the structure.
 * @param cfg Pointer to cfg_struct to delete.
 */
void cfg_free(struct cfg_struct* cfg)
{
  struct cfg_node* temp;

  if (cfg == NULL) return;

  while ((temp = cfg->head) != NULL)
  {
    cfg->head = temp->next;

    free(temp->key);
    free(temp->value);
    free(temp);
  }

  free(cfg);
}

/**
 * This function loads data from a file, and inserts / updates the specified cfg_struct.
 * New keys will be inserted.  Existing keys will have values overwritten by those read from the file.
 * The format of config-files is "key=value", with any amount of whitespace.
 * Comments can be included by using a # character: processing ends at that point.
 * The maximum line size is CFG_MAX_LINE bytes (see cfg_parse.h)
 * @param cfg Pointer to cfg_struct to update.
 * @param filename String containing filename to open and parse.
 * @return EXIT_SUCCESS (0) on success, or EXIT_FAILURE if file could not be opened.
 */
int cfg_load(struct cfg_struct* cfg, const char* filename)
{
  FILE* fp;
  char* delim;
  char buffer[CFG_MAX_LINE + 1];

  /* safety check: null input */
  if (cfg == NULL || filename == NULL) return EXIT_FAILURE;

  /* open file for reading */
  fp = fopen(filename, "r");
  if (fp == NULL) return EXIT_FAILURE;

  while (!feof(fp))
  {
    if (fgets(buffer, CFG_MAX_LINE + 1, fp) != NULL)
    {
      /* locate first # sign and terminate string there (comment) */
      delim = strchr(buffer, '#');
      if (delim != NULL) *delim = '\0';

      /* locate first = sign and prepare to split */
      delim = strchr(buffer, '=');
      if (delim != NULL)
      {
        *delim = '\0';
        delim ++;

        cfg_set(cfg, buffer, delim);
      }
      /* else: seems to be an invalid line */
    }
    /* else: read error */
  }

  fclose(fp);
  return EXIT_SUCCESS;
}

/**
 * This function saves a complete cfg_struct to a file.
 * Comments are not preserved.
 * @param cfg Pointer to cfg_struct to save.
 * @param filename String containing filename to open and parse.
 * @return EXIT_SUCCESS (0) on success, or EXIT_FAILURE if file could not be opened or a write error occurred.
 */
int cfg_save(const struct cfg_struct* cfg, const char* filename)
{
  FILE* fp;
  struct cfg_node* cur;

  /* safety check: null input */
  if (cfg == NULL || filename == NULL) return EXIT_FAILURE;

  /* open output file for writing */
  fp = fopen(filename, "w");
  if (fp == NULL) return EXIT_FAILURE;

  /* point at first item in list */
  cur = cfg->head;

  /* step through the list, dumping each key-value pair to disk */
  while (cur != NULL)
  {
    if (fputs(cur->key, fp) == EOF ||
        fputc('=', fp) == EOF ||
        fputs(cur->value, fp) == EOF ||
        fputc('\n', fp) == EOF)
    {
      fclose(fp);
      return EXIT_FAILURE;
    }

    cur = cur->next;
  }

  fclose(fp);
  return EXIT_SUCCESS;
}

/**
 * This function performs a key-lookup on a cfg_struct, and returns the associated value.
 * @param cfg Pointer to cfg_struct to search.
 * @param key String containing key to search for.
 * @return String containing associated value, or NULL if key was not found.
 */
const char* cfg_get(const struct cfg_struct* cfg, const char* key)
{
  char* tkey;
  struct cfg_node* cur;

  /* safety check: null input */
  if (cfg == NULL || key == NULL) return NULL;

  /* Trim input search key */
  tkey = cfg_norm_key(key);
  /* Exclude empty key */
  if (tkey == NULL) return NULL;

  /* set up pointer to start of list */
  cur = cfg->head;

  /* loop through linked list looking for match on key
    if found, free curkey, return the value */
  while (cur != NULL)
  {
    if (strcmp(tkey, cur->key) == 0)
    {
      free(tkey);
      return cur->value;
    }
    cur = cur->next;
  }

  free(tkey);
  return NULL;
}

/**
 * This function sets a single key-value pair in a cfg_struct.
 * If the key already exists, its value will be updated.
 * If not, a new item is added to the cfg_struct list.
 * There is a commented-out option to treat blank value as a delete operation:
 *  uncomment if your project needs this feature.
 * @param cfg Pointer to cfg_struct to search.
 * @param key String containing key to search for.
 * @param value String containing new value to assign to key.
 */
void cfg_set(struct cfg_struct* cfg, const char* key, const char* value)
{
  char* tkey;
  char* tvalue;

  struct cfg_node* cur;

  /* safety check: null input */
  if (cfg == NULL || key == NULL || value == NULL) return;

  /* Trim input search key */
  tkey = cfg_norm_key(key);
  /* Exclude empty key */
  if (tkey == NULL) return;

  /* Trim value. */
  tvalue = cfg_trim(value);

  /* Depending on implementation, you may wish to treat blank value
     as a "delete" operation */
  /* if (strlen(tvalue) == 0) { free(tvalue); cfg_delete(cfg, tkey); free(tkey); return; } */

  /* point at first item in list */
  cur = cfg->head;

  /* search list for existing key */
  while (cur != NULL)
  {
    if (strcmp(tkey, cur->key) == 0)
    {
      /* found a match: no longer need cur key */
      free(tkey);

      /* update value */
      free(cur->value);
      cur->value = tvalue;
      return;
    }
    cur = cur->next;
  }

  /* not found: create new element */
  cur = (struct cfg_node*)cfg_malloc(sizeof(struct cfg_node));

  /* assign key, value */
  cur->key = tkey;
  cur->value = tvalue;

  /* prepend */
  cur->next = cfg->head;
  cfg->head = cur;
}

/**
 * This function sets multiple key-value pairs in a cfg_struct.
 * @param cfg Pointer to cfg_struct to search.
 * @param keys Array of strings containing key to search for.
 * @param values Array of strings containing new value to assign to key.
 * @param count Length of keys / values arrays
 */
void cfg_set_array(struct cfg_struct* cfg, const char* keys[], const char* values[], const size_t count)
{
  size_t i;

  /* safety check: null input */
  if (cfg == NULL || keys == NULL || values == NULL) return;

  /* Call cfg_set on every item in the lists */
  for (i = 0; i < count; i ++)
    cfg_set(cfg, keys[i], values[i]);
}

/**
 * This function deletes a key-value pair from a cfg_struct.
 * If the key does not exist, the function does nothing.
 * @param cfg Pointer to cfg_struct to search.
 * @param key String containing key to search for.
 */
void cfg_delete(struct cfg_struct* cfg, const char* key)
{
  char* tkey;
  struct cfg_node* cur;
  struct cfg_node* prev;

  /* safety check: null input */
  if (cfg == NULL || key == NULL) return;

  /* Trim input search key */
  tkey = cfg_norm_key(key);
  /* Exclude empty key */
  if (tkey == NULL) return;

  /* set pointer to start of list */
  cur = cfg->head;

  /* search list for existing key */
  while (cur != NULL)
  {
    if (strcmp(tkey, cur->key) == 0)
    {
      /* found it - cleanup trimmed key */
      free(tkey);

      if (cur == cfg->head)
      {
        /* first element */
        cfg->head = cur->next;
      } else {
        /* splice out element */
        prev->next = cur->next;
      }

      /* delete element */
      free(cur->value);
      free(cur->key);
      free(cur);

      return;
    }

    prev = cur;
    cur = cur->next;
  }

  /* not found */
  /* cleanup trimmed key */
  free(tkey);
}

/**
 * This function deletes multiple key-value pairs from a cfg_struct.
 * @param cfg Pointer to cfg_struct to search.
 * @param keys Array of strings containing key to search for.
 * @param count Length of keys array
 */
void cfg_delete_array(struct cfg_struct* cfg, const char* keys[], const size_t count)
{
  size_t i;

  /* safety check: null input */
  if (cfg == NULL || keys == NULL) return;

  /* Call cfg_delete on every item in the list */
  for (i = 0; i < count; i ++)
    cfg_delete(cfg, keys[i]);
}

/**
 * This function performs the inverse of cfg_delete_array().
 * Instead of deleting entries from cfg which match keys[],
 *  this will KEEP only those entries that match keys[].
 * It can be used to keep a config file tidy between versions or
 *  after user edits.
 * @param cfg Pointer to cfg_struct to search.
 * @param keys Array of strings containing keys to keep
 * @param count Length of keys array
 */
void cfg_prune(struct cfg_struct* cfg, const char* keys[], const size_t count)
{
  char** tkeys;
  size_t i;

  struct cfg_node* cur;
  struct cfg_node* prev;

  /* safety check: null input */
  if (cfg == NULL || keys == NULL || SIZE_MAX / sizeof(char*) < count) return;

  /* First we must prep every key in keys[] using the normalize function. */
  tkeys = (char**)cfg_malloc(count * sizeof(char*));
  for (i = 0; i < count; i ++)
    tkeys[i] = cfg_norm_key(keys[i]);

  /* Now iterate through the cfg struct and test every entry */
  /* set pointer to start of list */
  cur = cfg->head;

  /* search list for existing key */
  while (cur != NULL)
  {
    for (i = 0; i < count; i ++)
      if (tkeys[i] != NULL && strcmp(tkeys[i], cur->key) == 0)
        break;

    if (i == count)
    {
      /* Didn't find a key match - delete this */
      free(cur->value);
      free(cur->key);

      if (cur == cfg->head)
      {
        /* first element */
        cfg->head = cur->next;
        free(cur);
        cur = cfg->head;
      } else {
        /* splice out element */
        prev->next = cur->next;
        free(cur);
        cur = prev->next;
      }
    } else {
      /* matched, advance list element */
      prev = cur;
      cur = cur->next;
    }
  }

  /* Cleanup all our trimmed keys */
  for (i = 0; i < count; i ++)
    free(tkeys[i]);
  free(tkeys);
}
