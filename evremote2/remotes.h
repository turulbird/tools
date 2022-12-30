
#ifndef REMOTES_H_
#define REMOTES_H_

#include <stdio.h>
#include <stdlib.h>
#include "global.h"

extern RemoteControl_t Ufs910_1W_RC;
extern RemoteControl_t Ufs910_14W_RC;
extern RemoteControl_t Tf7700_RC;
extern RemoteControl_t Hl101_RC;
extern RemoteControl_t Vip2_RC;
extern RemoteControl_t UFS922_RC;
extern RemoteControl_t UFC960_RC;
extern RemoteControl_t Fortis_RC;
extern RemoteControl_t Fortis_4G_RC;
extern RemoteControl_t UFS912_RC;
extern RemoteControl_t UFS913_RC;
extern RemoteControl_t Spark_RC;
extern RemoteControl_t Adb_Box_RC;
extern RemoteControl_t Cuberevo_RC;
extern RemoteControl_t Ipbox_RC;
extern RemoteControl_t CNBOX_RC;
extern RemoteControl_t VitaminHD5000_RC;
extern RemoteControl_t Pace7241_RC;
extern RemoteControl_t Hchs8100_RC;
extern RemoteControl_t Hchs9000_RC;
extern RemoteControl_t LircdName_RC;

extern BoxRoutines_t Ufs910_1W_BR;
extern BoxRoutines_t Ufs910_14W_BR;
extern BoxRoutines_t Tf7700_BR;
extern BoxRoutines_t Hl101_BR;
extern BoxRoutines_t Vip2_BR;
extern BoxRoutines_t UFS922_BR;
extern BoxRoutines_t UFC960_BR;
extern BoxRoutines_t Fortis_BR;
extern BoxRoutines_t Fortis_4G_BR;
extern BoxRoutines_t UFS912_BR;
extern BoxRoutines_t UFS913_BR;
extern BoxRoutines_t Spark_BR;
extern BoxRoutines_t Adb_Box_BR;
extern BoxRoutines_t Cuberevo_BR;
extern BoxRoutines_t Ipbox_BR;
extern BoxRoutines_t CNBOX_BR;
extern BoxRoutines_t VitaminHD5000_BR;
extern BoxRoutines_t Pace7241_BR;
extern BoxRoutines_t Hchs8100_BR;
extern BoxRoutines_t Hchs9000_BR;
extern BoxRoutines_t LircdName_BR;

int selectRemote(Context_r_t *context_r, Context_t *context, eBoxType type);

#endif  // REMOTES_H_
// vim:ts=4
