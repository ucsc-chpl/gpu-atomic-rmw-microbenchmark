#define PARTICIPATING 1
#define NON_PARTICIPATING 0

// Ticket lock 
static void lock(__global atomic_uint* next_ticket, __global atomic_uint* now_serving) {
    uint my_ticket;
    atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_relaxed, memory_scope_device);
    my_ticket = atomic_fetch_add(next_ticket, 1);
    while (atomic_load(now_serving) != my_ticket) {}
}

static void unlock(__global atomic_uint* now_serving) {
    atomic_store(now_serving, atomic_load(now_serving) + 1);
}

static uint get_occupancy(__global uint *count, 
                          __global uint *poll_open,
                          __global uint *M,
                          __global atomic_uint *now_serving,
                          __global atomic_uint *next_ticket) {
    lock(next_ticket, now_serving);
    // Polling Phase
    if (*poll_open) { 
        M[get_group_id(0)] = *count;
        *count = *count + 1;
        unlock(now_serving);
    } else {
        // Poll is no longer open. Workgroup is not participating.
        unlock(now_serving);
        return NON_PARTICIPATING;
    }

    // Closing Phase
    lock(next_ticket, now_serving);
    if (*poll_open) {
        // First workgroup to reach this point closes the poll.
        *poll_open = 0;  
    }
    unlock(now_serving);
    return PARTICIPATING;
}

__kernel void occupancy_discovery(__global atomic_uint* res,
                                  __global uint* iters,
                                  __global uint* mapping,
                                  __global uint *count, 
                                  __global uint *poll_open,
                                  __global uint *M,
                                  __global atomic_uint *now_serving,
                                  __global atomic_uint *next_ticket) {
    // Single represesentative thread from each workgroups runs the occupancy_discovery protocol
    uint index;
    if (get_local_id(0) == 0) {
        get_occupancy(count, poll_open, M, now_serving, next_ticket);
    }
    index = mapping[get_global_id(0)];
    for (uint i = 0; i < *iters; i++) {
        atomic_fetch_add_explicit(&res[index], 1, memory_order_relaxed);
    }
    return;
}
