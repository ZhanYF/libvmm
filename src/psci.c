/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include "psci.h"
#include "smc.h"
#include "fault.h"
#include "util/util.h"
#include "vmm.h"

bool handle_psci(uint64_t vcpu_id, seL4_UserContext *regs, uint64_t fn_number, uint32_t hsr)
{
    // @ivanv: write a note about what convention we assume, should we be checking
    // the convention?
    switch (fn_number) {
        case PSCI_VERSION: {
            // We support PSCI version 1.2
            uint32_t version = PSCI_MAJOR_VERSION(1) | PSCI_MINOR_VERSION(2);
            smc_set_return_value(regs, version);
            break;
        }
        case PSCI_CPU_ON: {
            uintptr_t target_cpu = smc_get_arg(regs, 1);
            // Right now we only have one vCPU and so any fault for a target vCPU
            // that isn't the one that's already on we consider an error on the
            // guest's side.
            // @ivanv: adapt for starting other vCPUs
            if (target_cpu == vcpu_id) {
                smc_set_return_value(regs, PSCI_ALREADY_ON);
            } else {
                // The guest has requested to turn on a virtual CPU that does
                // not exist.
                smc_set_return_value(regs, PSCI_INVALID_PARAMETERS);
            }
            break;
        }
        case PSCI_MIGRATE_INFO_TYPE:
            /* trusted OS does not require migration */
            // @ivanv: why the is this "2"
            smc_set_return_value(regs, 2);
            break;
        case PSCI_FEATURES:
            smc_set_return_value(regs, PSCI_NOT_SUPPORTED);
            break;
        case PSCI_SYSTEM_RESET: {
            bool success = guest_restart();
            if (success) {
                LOG_VMM_ERR("Failed to restart guest\n");
                smc_set_return_value(regs, PSCI_INTERNAL_FAILURE);
            } else {
                smc_set_return_value(regs, PSCI_SUCCESS);
            }
            break;
        }
        default:
            LOG_VMM_ERR("Unhandled PSCI function ID 0x%lx\n", fn_number);
            return false;
    }

    int err = fault_advance_vcpu(regs);
    assert(!err);

    return err == 0;
}
