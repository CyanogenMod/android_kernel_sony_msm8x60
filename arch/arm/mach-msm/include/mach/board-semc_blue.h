/* /kernel/arch/arm/mach-msm/include/mach/board-semc_blue.h
 *
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef _BOARD_SEMC_BLUE_H
#define _BOARD_SEMC_BLUE_H

#ifdef CONFIG_INPUT_MBHC_MIC_BIAS_SWITCHING_GPIO
int msm8960_read_is_pmic_used(void);
#endif /* CONFIG_INPUT_MBHC_MIC_BIAS_SWITCHING_GPIO */

#endif
