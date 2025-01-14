
#ifndef __VM_H__
#define __VM_H__

#include <aj_types.h>
#include "vcpu.h"
#include <hyper/vgic.h>

struct vm_t {
    uint32_t id;

    cpu_t *vcpus;
    uint32_t vcpu_num;

    struct vgic_t *vgic;
};

void vm_init();


#endif // __VM_H__