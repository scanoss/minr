#ifndef __MINR_MZ_H
    #define __MINR_MZ_H

void mz_cat(struct mz_job *job, char *key);

typedef enum
{
    MZ_OPTIMISE_ALL = 0,
    MZ_OPTIMISE_DUP = 1,
    MZ_OPTIMISE_ORPHAN = 2,
    MZ_OPTIMISE_SQUARENESS = 4,
    MZ_OPTIMISE_UNWANTED_HEADER = 16
}mz_optimise_mode_t;

void mz_optimise(struct mz_job *job, mz_optimise_mode_t mode);
void mz_extract(struct mz_job *job);

#endif