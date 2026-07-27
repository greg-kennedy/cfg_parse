/* driver test program for cfg_parse */

#include "cfg_parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_keys(const struct cfg_struct* cfg)
{
  char** keys;
  unsigned int i, count;

  keys = cfg_get_keys(cfg, &count);

  printf("Keys (%lu total):\n", (unsigned long)count);
  for (i = 0; i < count; i ++)
  {
    printf(" [%02lu] '%s'\n", (unsigned long)i, keys[i]);
    free(keys[i]);
  }
  free(keys);
}

static char* get_local_config(void)
{
  char* homeconfig;
  const char* homedir = getenv("HOME");
  if (homedir == NULL) return NULL;

  homeconfig = malloc(strlen(homedir) + 9);
  strcpy(homeconfig, homedir);
  strcat(homeconfig, "/.config");
  return homeconfig;
}

int main(int argc, char* argv[])
{
  int i;

  /* Pointer to a cfg_struct structure */
  struct cfg_struct* cfg;

  /* some data */
  const char* arrayKeys[] = {"ARRAY_KEY_1", NULL, "ARRAY_KEY_2"};
  const char* arrayValues[] = {"ARRAY_VALUE_1", NULL, "ARRAY_VALUE_2"};

  /* path to config file in user's home */
  char* homeconfig = get_local_config();
  
  /* Initialize config struct */
  cfg = cfg_init();

  /* Specifying some defaults */
  cfg_set(cfg, "KEY", "VALUE");
  cfg_set(cfg, "KEY_A", "DEFAULT_VALUE_A");

  /* "Required" file */
  if (cfg_load(cfg, "config.ini") < 0)
  {
    fprintf(stderr, "Unable to load cfg.ini\n");
    return EXIT_FAILURE;
  }

  /* Several "optional" files can be added as well
      Each subsequent call upserts values already in
      the cfg structure. */
  cfg_load(cfg, "/usr/local/etc/config.ini");
  if (homeconfig)
  {
    cfg_load(cfg, homeconfig);
    free(homeconfig);
  }

  /* Reading anything from CLI args works too */
  for (i = 1; i < argc; i++)
    cfg_load(cfg, argv[i]);

  /* Retrieve the value for key INFINITY, and print */
  printf("INFINITY = %s\n", cfg_get(cfg, "INFINITY"));

  /* Retrieve the value for key "KEY", and print */
  printf("KEY = %s\n", cfg_get(cfg, "KEY"));

  /* Try the array functions */
  cfg_set_array(cfg, arrayKeys, arrayValues, 3);
  print_keys(cfg);

  cfg_delete_array(cfg, arrayKeys, 3);
  /* cfg_prune(cfg, arrayKeys, 3); */
  print_keys(cfg);

  /* Delete the key-value pair for "DeLeTe_Me" */
  cfg_delete(cfg, "DeLeTe_Me");

  /* Dump cfg-struct to disk. */
  cfg_save(cfg, "config_new.ini");

  /* All done, clean up. */
  cfg_free(cfg);

  return EXIT_SUCCESS;
}
