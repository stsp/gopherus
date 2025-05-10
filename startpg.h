/*
 * This file is part of the Gopherus project.
 * Copyright (C) 2013-2022 Mateusz Viste
 */

#ifndef startpg_h_sentinel
  #define startpg_h_sentinel

  /* loads the embedded start page into a memory buffer and returns */
  size_t loadembeddedstartpage(char *buffer, size_t buffer_max, const char *selector, const char *bookmarksfile);

#endif
