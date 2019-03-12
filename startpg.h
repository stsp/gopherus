/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2019 Mateusz Viste
 */

#ifndef startpg_h_sentinel
  #define startpg_h_sentinel

  /* loads the embedded start page into a memory buffer and returns */
  int loadembeddedstartpage(char *buffer, unsigned long buffer_max, const char *selector, const char *bookmarksfile);

#endif
