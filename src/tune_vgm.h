/* Tune selector. Picks one embedded VGM at compile time. Regenerate the
 * per-tune headers with scripts/vgm_to_header.py. */
#ifndef TUNE_VGM_H
#define TUNE_VGM_H

#if defined(TUNE_PM2)
#include "tune_pm2.h"
#else
#include "tune_crystal.h"
#endif

#endif /* TUNE_VGM_H */
