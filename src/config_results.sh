#!/bin/bash

atomic_throughput() {
    local workgroups="$1"
    local device="$2"
    local contention="$3"
    local padding="$4"
    local iters="$5"

    result=$(./atomic_rmw_test.run -w "$workgroups" -d "$device" -c "$contention" -p "$padding" -i "$iters")
    throughput=$(echo "$result" | grep 'Throughput:' | grep -oE '[0-9]+([.][0-9]+)?')
    if [[ -n "$throughput" ]]; then
        echo "$throughput"
    else
        echo "0"
    fi
}

device_information() {
    local workgroups="$1"
    local device="$2"
    local iters="$3"

    result=$(./atomic_rmw_test.run -w "$workgroups" -d "$device" -c 1 -p 1 -i "$iters")
    device_info=$(echo "$result" | grep 'Device:')
    if [[ -n "$device_info" ]]; then
        workgroup_size=$(echo "$device_info" | cut -d '(' -f 2 | cut -d ',' -f 1)
        device_name=$(echo "$device_info" | cut -d ':' -f 2- | cut -d ',' -f 1)
        echo "${workgroup_size},${workgroups}:${device_name}"
    else
        echo ""
    fi
}

# Main script
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <workgroups> <device> <rmw_iterations>"
    exit 1
fi

workgroups="$1"
device="$2"
rmw_iterations="$3"

title=$(device_information "$workgroups" "$device" "$rmw_iterations")", contiguous_access: atomic_fa_relaxed"

output_file="result.txt"
> "$output_file"

echo "$title"
echo "$title" >> "$output_file"

coordinates=()
powers=(1 2 4 8 16 32 64 128 256 512 1024)
for power in "${powers[@]}"; do
    contention="$power"
    for power2 in "${powers[@]}"; do
        padding="$power2"
        value=$(atomic_throughput "$workgroups" "$device" "$contention" "$padding" "$rmw_iterations")
        coordinates+="($contention, $padding, $value)\n"
        echo -e "Contention: $contention,\tPadding: $padding,\tThroughput: $value"
        echo "($contention, $padding, $value)" >> "$output_file"
    done
done
