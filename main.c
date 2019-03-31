/* driver test program for cfg_parse */

#include "cfg_parse.h"

#include <stdio.h>

int main()
{
  /* Pointer to a cfg_struct structure */
  struct cfg_struct* cfg;

  /* some data */
  const char* arrayKeys[] = {"ARRAY_KEY_1", "ARRAY_KEY_2"};
  const char* arrayValues[] = {"ARRAY_VALUE_1", "ARRAY_VALUE_2"};

  /* Initialize config struct */
  cfg = cfg_init();

  /* Specifying some defaults */
  cfg_set(cfg,"KEY","VALUE");
  cfg_set(cfg,"KEY_A","DEFAULT_VALUE_A");

  /* "Required" file */
  if (cfg_load(cfg,"config.ini") < 0)
  {
    fprintf(stderr,"Unable to load cfg.ini\n");
    return -1;
  }

  /* Several "optional" files can be added as well
      Each subsequent call upserts values already in
      the cfg structure. */
  cfg_load(cfg,"/usr/local/etc/config.ini");
  cfg_load(cfg,"~/.config");

  /* Retrieve the value for key INFINITY, and print */
  printf("INFINITY = %s\n",cfg_get(cfg,"INFINITY"));

  /* Retrieve the value for key "KEY", and print */
  printf("KEY = %s\n",cfg_get(cfg,"KEY"));

  /* Try the array functions */
  cfg_set_array(cfg, arrayKeys, arrayValues, 2);
  cfg_delete_array(cfg, arrayKeys, 2);

  /* Delete the key-value pair for "DeLeTe_Me" */
  cfg_delete(cfg,"DeLeTe_Me");

  /* Dump cfg-struct to disk. */
  cfg_save(cfg,"config_new.ini");

  /* All done, clean up. */
  cfg_free(cfg);

  return 0;
}
